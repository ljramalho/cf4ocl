/*
 * This file is part of cf4ocl (C Framework for OpenCL).
 *
 * cf4ocl is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cf4ocl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cf4ocl. If not, see <http://www.gnu.org/licenses/>.
 * */

/**
 * @internal
 *
 * @file
 * Test the device wrapper class and its methods.
 *
 * @author Nuno Fachada
 * @date 2019
 * @copyright [GNU General Public License version 3 (GPLv3)](http://www.gnu.org/licenses/gpl.html)
 * */

#include <cf4ocl2.h>
#include "test.h"

/**
 * @internal
 *
 * @brief Tests obtaining device information.
 * */
static void info_test() {

    /* Test variables. */
    CCLContext * ctx = NULL;
    CCLDevice * dev = NULL;
    CCLErr * err = NULL;
    cl_ulong scalar;
    size_t * array;

    /* Get the test context with the pre-defined device. */
    ctx = ccl_test_context_new(0, &err);
    g_assert_no_error(err);

    /* Get device associated with context. */
    dev = ccl_context_get_device(ctx, 0, &err);
    g_assert_no_error(err);

    /* Get a scalar information. */
    scalar = ccl_device_get_info_scalar(
        dev, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, cl_ulong, &err);
    g_assert_no_error(err);
    g_assert_cmpuint(scalar, >, 0); // Actually min size should be 64KB

    /* Get an array information. */
    array = ccl_device_get_info_array(
        dev, CL_DEVICE_MAX_WORK_ITEM_SIZES, size_t, &err);
    g_assert_no_error(err);
    g_assert_cmpuint(array[0], >=, 1);
    g_assert_cmpuint(array[1], >=, 1);
    g_assert_cmpuint(array[2], >=, 1);

    /* Confirm that memory allocated by wrappers has not yet been freed. */
    g_assert_false(ccl_wrapper_memcheck());

    /* Destroy stuff. */
    ccl_context_destroy(ctx);

    /* Confirm that memory allocated by wrappers has been properly freed. */
    g_assert_true(ccl_wrapper_memcheck());

}

/**
 * @internal
 *
 * @brief Tests the creation of sub-devices.
 * */
static void sub_devices_test() {

#ifndef CL_VERSION_1_2

    g_test_skip("Test skipped due to lack of OpenCL 1.2 support.");

#else

    /* Test variables. */
    CCLContext * ctx = NULL;
    CCLDevice * pdev = NULL;
    CCLErr * err = NULL;
    CCLWrapperInfo * info = NULL;
    cl_device_partition_property * dpp;
    ccl_devquery_format format_func;
    cl_uint i;
    cl_bool supported;
    CCLDevice * const * subdevs;
    cl_uint num_subdevs;
    cl_uint max_subdevs;
    cl_uint cu;
    cl_uint subcu;
    cl_device_id parent_device;
    const int aux_len = 10;
    gchar out[CCL_TEST_DEVQUERY_MAXINFOLEN], aux[aux_len];

    /* Get the test context with the pre-defined device. */
    ctx = ccl_test_context_new(120, &err);
    g_assert_no_error(err);
    if (!ctx) return;

    /* Get parent device. */
    pdev = ccl_context_get_device(ctx, 0, &err);
    g_assert_no_error(err);

    /* Check if device has more than one compute unit. */
    cu = ccl_device_get_info_scalar(
        pdev, CL_DEVICE_MAX_COMPUTE_UNITS, cl_uint, &err);
    g_assert_no_error(err);

    /* Get device partition properties. */
    dpp = ccl_device_get_info_array(
        pdev, CL_DEVICE_PARTITION_PROPERTIES,
        cl_device_partition_property, &err);

    /* Make sure device is partitionable. */
    if ((err != NULL) && (err->code == CCL_ERROR_INFO_UNAVAILABLE_OCL)
            && (err->domain == CCL_ERROR)) {
        g_test_message("Test device could not be partitioned, as "\
            "such sub-devices test will not be performed.");
        ccl_context_destroy(ctx);
        return;
    }
    g_assert_no_error(err);

    /* Get maximum number sub-devices. */
    max_subdevs = ccl_device_get_info_scalar(
        pdev, CL_DEVICE_PARTITION_MAX_SUB_DEVICES, cl_uint, &err);
    g_assert_no_error(err);

    /* Test partition equally, if supported by device. */
    supported = CL_FALSE;
    for (i = 0; dpp[i] != 0; ++i) {
        if (dpp[i] == CL_DEVICE_PARTITION_EQUALLY) {
            supported = CL_TRUE;
            break;
        }
    }
    if (supported) {

        /* Find an appropriate number of compute units for each
         * sub-device. */
        for (i = 8; (cu = max_subdevs / i) == 0; i /= 2);

        /* Set partition properties. */
        const cl_device_partition_property eqprop[] =
            {CL_DEVICE_PARTITION_EQUALLY, cu, 0};

        /* Partition device. */
        subdevs = ccl_device_create_subdevices(
            pdev, eqprop, &num_subdevs, &err);
        g_assert_no_error(err);

        /* Check sub-devices. */
        for (i = 0; i < num_subdevs; ++i) {

            /* Check the number of compute units. */
            subcu = ccl_device_get_info_scalar(subdevs[i],
                CL_DEVICE_MAX_COMPUTE_UNITS, cl_uint, &err);
            g_assert_no_error(err);
            g_assert_cmpuint(subcu, ==, cu);

            /* Check the parent device. */
            parent_device = ccl_device_get_info_scalar(
                subdevs[i], CL_DEVICE_PARENT_DEVICE, cl_device_id, &err);
            g_assert_no_error(err);
            g_assert_cmphex(GPOINTER_TO_SIZE(parent_device), ==,
                GPOINTER_TO_SIZE(ccl_device_unwrap(pdev)));

            /* Check the partitioning style. */
            info = ccl_device_get_info(
                subdevs[i], CL_DEVICE_PARTITION_TYPE, &err);
            g_assert_no_error(err);
            g_assert_true(info->value != NULL);
            g_assert_cmpuint( /* Array must have at least two items */
                info->size, >=, 2 * sizeof(cl_device_partition_property));
            g_assert_cmpuint( /* Check partitioning type */
                    ((cl_device_partition_property *) info->value)[0],
                    ==,
                    CL_DEVICE_PARTITION_EQUALLY);
            g_assert_cmpuint( /* Check number of CUs in each device */
                    ((cl_device_partition_property *) info->value)[1], ==, cu);

            /* Test device query function for formatting partition
             * properties. */

            format_func = /* Get formatting function. */
                ccl_devquery_info_map[
                    ccl_devquery_get_index("PARTITION_TYPE")].format;
            format_func(info, out, CCL_TEST_DEVQUERY_MAXINFOLEN, "");
            g_assert_true(g_strrstr(out, "EQUALLY") != NULL);
            g_snprintf(aux, aux_len, "%d", cu);
            g_assert_true(g_strrstr(out, aux) != NULL); /* Check number of CUs */
        }

        /* Check that the last position is NULL. */
        g_assert_cmphex(
            GPOINTER_TO_SIZE(subdevs[i]), ==, GPOINTER_TO_SIZE(NULL));
    }

    /* Test partition by counts, if supported by device. */
    supported = CL_FALSE;
    for (i = 0; dpp[i] != 0; ++i) {
        if (dpp[i] == CL_DEVICE_PARTITION_BY_COUNTS) {
            supported = CL_TRUE;
            break;
        }
    }
    if (supported) {

        /* Allocate partition properties array and initialize it. */
        cl_device_partition_property * ctprop = g_slice_alloc0(
            (max_subdevs + 3) * sizeof(cl_device_partition_property));
        ctprop[0] = CL_DEVICE_PARTITION_BY_COUNTS;

        /* Find an appropriate number of compute units for each
         * sub-device. */
        cu = max_subdevs / 2;
        cl_uint total_cu = 0, total_cu_check = 0;
        if (cu == 0) {
            total_cu = 1;
            ctprop[1] = max_subdevs;
            ctprop[2] = CL_DEVICE_PARTITION_BY_COUNTS_LIST_END;
        } else {
            for (i = 1; (i <= max_subdevs) && (cu > 0); ++i) {
                total_cu += cu;
                ctprop[i] = cu;
                cu /= 2;
            }
            ctprop[i] = CL_DEVICE_PARTITION_BY_COUNTS_LIST_END;
        }

        /* Partition device. */
        subdevs = ccl_device_create_subdevices(
            pdev, ctprop, &num_subdevs, &err);
        g_assert_no_error(err);

        /* Check sub-devices. */
        for (i = 0; i < num_subdevs; ++i) {

            /* Check the number of compute units. */
            subcu = ccl_device_get_info_scalar(subdevs[i],
                CL_DEVICE_MAX_COMPUTE_UNITS, cl_uint, &err);
            g_assert_no_error(err);
            total_cu_check += subcu;

            /* Check the parent device. */
            parent_device = ccl_device_get_info_scalar(
                subdevs[i], CL_DEVICE_PARENT_DEVICE, cl_device_id, &err);
            g_assert_no_error(err);
            g_assert_cmphex(
                GPOINTER_TO_SIZE(parent_device),
                ==,
                GPOINTER_TO_SIZE(ccl_device_unwrap(pdev)));

            /* Check the partitioning style. */
            info = ccl_device_get_info(
                subdevs[i], CL_DEVICE_PARTITION_TYPE, &err);
            g_assert_no_error(err);
            g_assert_true(info->value != NULL);
            g_assert_cmpuint( /* Array must have at least three items */
                info->size, >=, 3 * sizeof(cl_device_partition_property));
            for (cl_uint j = 0;
                 ctprop[j] != CL_DEVICE_PARTITION_BY_COUNTS_LIST_END;
                 j++) {
                g_assert_cmpuint(
                    ((cl_device_partition_property *) info->value)[j],
                    ==,
                    ctprop[j]);
            }

            /* Test device query function for formatting partition
             * properties. */

            format_func = /* Get formatting function. */
                ccl_devquery_info_map[
                    ccl_devquery_get_index("PARTITION_TYPE")].format;
            format_func(info, out, CCL_TEST_DEVQUERY_MAXINFOLEN, "");
            g_assert_true(g_strrstr(out, "BY_COUNTS") != NULL);
            for (guint j = 1;
                 ctprop[j] != CL_DEVICE_PARTITION_BY_COUNTS_LIST_END;
                 j++) {
                /* Check if sub-device with the specified number of CUs appears
                 * in the string returned by the query. */
                g_snprintf(aux, aux_len, " %ld", ctprop[j]);
                g_assert_true(g_strrstr(out, aux) != NULL);
            }
        }

        /* Check that the total number of compute units is as
         * expected. */
        g_assert_cmpuint(total_cu_check, ==, total_cu);

        /* Check that the last position is NULL. */
        g_assert_cmphex(
            GPOINTER_TO_SIZE(subdevs[i]), ==, GPOINTER_TO_SIZE(NULL));

        /* Release memory associated with partition properties array. */
        g_slice_free1(
            (max_subdevs + 3) * sizeof(cl_device_partition_property),
            ctprop);
    }

    /* Confirm that memory allocated by wrappers has not yet been freed. */
    g_assert_false(ccl_wrapper_memcheck());

    /* Destroy stuff. */
    ccl_context_destroy(ctx);

    /* Confirm that memory allocated by wrappers has been properly freed. */
    g_assert_true(ccl_wrapper_memcheck());

#endif

}

/**
 * @internal
 *
 * @brief Main function.
 * @param[in] argc Number of command line arguments.
 * @param[in] argv Command line arguments.
 * @return Result of test run.
 * */
int main(int argc, char ** argv) {

    g_test_init(&argc, &argv, NULL);

    g_test_add_func(
        "/wrappers/device/info",
        info_test);

    g_test_add_func(
        "/wrappers/device/sub-devices",
        sub_devices_test);

    return g_test_run();
}
