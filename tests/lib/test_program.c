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
 * Test the program class. Also tests the kernel class.
 *
 * @author Nuno Fachada
 * @date 2019
 * @copyright [GNU General Public License version 3 (GPLv3)](http://www.gnu.org/licenses/gpl.html)
 * */

#include <cf4ocl2.h>
#include <glib/gstdio.h>
#include "test.h"

#define CCL_TEST_PROGRAM_SUM "test_sum_full"

#define CCL_TEST_PROGRAM_SUM_FILENAME CCL_TEST_PROGRAM_SUM ".cl"

#define CCL_TEST_PROGRAM_BUF_SIZE 16
#define CCL_TEST_PROGRAM_LWS 8 /* Must be a divisor of CCL_TEST_PROGRAM_BUF_SIZE */
G_STATIC_ASSERT(CCL_TEST_PROGRAM_BUF_SIZE % CCL_TEST_PROGRAM_LWS == 0);

/**
 * @internal
 *
 * @brief Tests creation, getting info from and destruction of
 * program wrapper objects.
 * */
static void create_info_destroy_test() {

    /* Test variables. */
    CCLContext * ctx = NULL;
    CCLProgram * prg = NULL;
    CCLProgram * prg2 = NULL;
    CCLProgramBinary * prg_bin = NULL;
    CCLKernel * krnl = NULL;
    CCLWrapperInfo * info = NULL;
    CCLDevice * d = NULL;
    CCLDevice * d2 = NULL;
    CCLErr * err = NULL;
    gchar * tmp_dir_name;
    gchar * tmp_file_prefix;
    const char * build_log;
    cl_device_id * devices = NULL;
    cl_context context = NULL;

    /* Create a context with devices from first available platform. */
    ctx = ccl_test_context_new(0, &err);
    g_assert_no_error(err);

    /* Get first device in context (and in program). */
    d = ccl_context_get_device(ctx, 0, &err);
    g_assert_no_error(err);

    /* ************************************************** */
    /* 1. Create program from source file and destroy it. */
    /* ************************************************** */

    /* Get a temp. dir. */
    tmp_dir_name = g_dir_make_tmp("test_program_XXXXXX", &err);
    g_assert_no_error(err);

    /* Get a temp file prefix. */
    tmp_file_prefix = g_strdup_printf("%s%c%s",
        tmp_dir_name, G_DIR_SEPARATOR, CCL_TEST_PROGRAM_SUM_FILENAME);

    /* Create a temporary kernel file. */
    g_file_set_contents(
        tmp_file_prefix, CCL_TEST_PROGRAM_SUM_CONTENT, -1, &err);
    g_assert_no_error(err);

    /* Create a new program from kernel file. */
    prg = ccl_program_new_from_source_file(
        ctx, tmp_file_prefix, &err);
    g_assert_no_error(err);

    /* Destroy program. */
    ccl_program_destroy(prg);

    /* ****************************************************** */
    /* 2. Create program from source files (only one though). */
    /* ****************************************************** */

    const char * file_pref = (const char *) tmp_file_prefix;
    prg = ccl_program_new_from_source_files(ctx, 1, &file_pref, &err);
    g_assert_no_error(err);

    g_free(tmp_file_prefix);

    /* *********************************************** */
    /* 3. Check program info/build info, before build. */
    /* *********************************************** */

    /* Get some program info, compare it with expected info. */
    info = ccl_program_get_info(prg, CL_PROGRAM_CONTEXT, &err);
    g_assert_no_error(err);
    g_assert_true(*((cl_context *) info->value) == ccl_context_unwrap(ctx));

    /* Get number of devices from program info, check that this is the
     * same value as the number of devices in context. */
    info = ccl_program_get_info(prg, CL_PROGRAM_NUM_DEVICES, &err);
    g_assert_no_error(err);
    g_assert_cmpuint(
        *((cl_uint *) info->value), ==, ccl_context_get_num_devices(ctx, &err));
    g_assert_no_error(err);

    /* Get program source from program info, check that it is the
     * same as the passed source. */
    info = ccl_program_get_info(prg, CL_PROGRAM_SOURCE, &err);
    g_assert_no_error(err);
    g_assert_cmpstr((char *) info->value, ==, CCL_TEST_PROGRAM_SUM_CONTENT);

    /* Check that no build was performed yet. */
    info = ccl_program_get_build_info(prg, d, CL_PROGRAM_BUILD_STATUS, &err);
    g_assert_no_error(err);
    g_assert_cmpint(*((cl_build_status *) info->value), ==, CL_BUILD_NONE);

    /* *********************************************** */
    /* 4. Build program, check build info after build. */
    /* *********************************************** */

    /* **** BUILD PROGRAM **** */
    ccl_program_build(prg, NULL, &err);
    g_assert_no_error(err);

    /* Get some program build info, compare it with expected values. */
    info = ccl_program_get_build_info(prg, d, CL_PROGRAM_BUILD_STATUS, &err);
    g_assert_no_error(err);
    g_assert_true((*((cl_build_status *) info->value) == CL_BUILD_SUCCESS)
        || (*((cl_build_status *) info->value) == CL_BUILD_IN_PROGRESS));

    /* Get the build log, check that no error occurs. */
    info = ccl_program_get_build_info(prg, d, CL_PROGRAM_BUILD_LOG, &err);
    g_assert_no_error(err);

    /* ************************************ */
    /* 5. Get kernel and check kernel info. */
    /* ************************************ */

    /* Get kernel wrapper object. */
    krnl = ccl_program_get_kernel(prg, CCL_TEST_PROGRAM_SUM, &err);
    g_assert_no_error(err);

    /* Get some kernel info, compare it with expected info. */

    /* Get kernel function name from kernel info, compare it with the
     * expected value. */
    info = ccl_kernel_get_info(krnl, CL_KERNEL_FUNCTION_NAME, &err);
    g_assert_no_error(err);
    g_assert_cmpstr((gchar *) info->value, ==, CCL_TEST_PROGRAM_SUM);

    /* Check if the kernel context is the same as the initial context
     * and the program context. */
    info = ccl_kernel_get_info(krnl, CL_KERNEL_CONTEXT, &err);
    g_assert_no_error(err);
    g_assert_true(*((cl_context *) info->value) == ccl_context_unwrap(ctx));

    /* Check that program in kernel is the same program where kernel came
     * from. */
    info = ccl_kernel_get_info(krnl, CL_KERNEL_PROGRAM, &err);
    g_assert_no_error(err);
    g_assert_true(*((cl_program *) info->value) == ccl_program_unwrap(prg));

#ifdef CL_VERSION_1_2

    /* ****************************** */
    /* 6. Check kernel argument info. */
    /* ****************************** */

    cl_kernel_arg_address_qualifier kaaq;
    char * kernel_arg_type_name;
    char * kernel_arg_name;
    cl_uint ocl_ver;

    /* Get OpenCL version of program's underlying platform. */
    ocl_ver = ccl_program_get_opencl_version(prg, &err);
    g_assert_no_error(err);

    /* If platform supports kernel argument queries, get kernel argument
     * information and compare it with expected info. */
    if (ocl_ver >= 120) {

        /* First kernel argument. */
        kaaq = ccl_kernel_get_arg_info_scalar(
            krnl, 0, CL_KERNEL_ARG_ADDRESS_QUALIFIER,
            cl_kernel_arg_address_qualifier, &err);
        g_assert_true((err == NULL)
            || ((err->code == CCL_ERROR_INFO_UNAVAILABLE_OCL)
                && (err->domain == CCL_ERROR))
            || ((err->code == CL_KERNEL_ARG_INFO_NOT_AVAILABLE)
                && (err->domain == CCL_OCL_ERROR)));
        if (err == NULL) {
            g_assert_cmphex(kaaq, ==, CL_KERNEL_ARG_ADDRESS_GLOBAL);
        } else {
            ccl_err_clear(&err);
        }

        kernel_arg_type_name = ccl_kernel_get_arg_info_array(
            krnl, 0, CL_KERNEL_ARG_TYPE_NAME, char, &err);
        g_assert_true((err == NULL)
            || ((err->code == CCL_ERROR_INFO_UNAVAILABLE_OCL)
                && (err->domain == CCL_ERROR))
            || ((err->code == CL_KERNEL_ARG_INFO_NOT_AVAILABLE)
                && (err->domain == CCL_OCL_ERROR)));
        if (err == NULL) {
            g_assert_cmpstr(kernel_arg_type_name, ==, "uint*");
        } else {
            ccl_err_clear(&err);
        }

        kernel_arg_name = ccl_kernel_get_arg_info_array(
            krnl, 0, CL_KERNEL_ARG_NAME, char, &err);
        g_assert_true((err == NULL)
            || ((err->code == CCL_ERROR_INFO_UNAVAILABLE_OCL)
                && (err->domain == CCL_ERROR))
            || ((err->code == CL_KERNEL_ARG_INFO_NOT_AVAILABLE)
                && (err->domain == CCL_OCL_ERROR)));
        if (err == NULL) {
            g_assert_cmpstr(kernel_arg_name, ==, "a");
        } else {
            ccl_err_clear(&err);
        }

        /* Second kernel argument. */
        kaaq = ccl_kernel_get_arg_info_scalar(
            krnl, 1, CL_KERNEL_ARG_ADDRESS_QUALIFIER,
            cl_kernel_arg_address_qualifier, &err);
        g_assert_true((err == NULL)
            || ((err->code == CCL_ERROR_INFO_UNAVAILABLE_OCL)
                && (err->domain == CCL_ERROR))
            || ((err->code == CL_KERNEL_ARG_INFO_NOT_AVAILABLE)
                && (err->domain == CCL_OCL_ERROR)));
        if (err == NULL) {
            g_assert_cmphex(kaaq, ==, CL_KERNEL_ARG_ADDRESS_GLOBAL);
        } else {
            ccl_err_clear(&err);
        }

        kernel_arg_type_name = ccl_kernel_get_arg_info_array(
            krnl, 1, CL_KERNEL_ARG_TYPE_NAME, char, &err);
        g_assert_true((err == NULL)
            || ((err->code == CCL_ERROR_INFO_UNAVAILABLE_OCL)
                && (err->domain == CCL_ERROR))
            || ((err->code == CL_KERNEL_ARG_INFO_NOT_AVAILABLE)
                && (err->domain == CCL_OCL_ERROR)));
        if (err == NULL) {
            g_assert_cmpstr(kernel_arg_type_name, ==, "uint*");
        } else {
            ccl_err_clear(&err);
        }

        kernel_arg_name = ccl_kernel_get_arg_info_array(
            krnl, 1, CL_KERNEL_ARG_NAME, char, &err);
        g_assert_true((err == NULL)
            || ((err->code == CCL_ERROR_INFO_UNAVAILABLE_OCL)
                && (err->domain == CCL_ERROR))
            || ((err->code == CL_KERNEL_ARG_INFO_NOT_AVAILABLE)
                && (err->domain == CCL_OCL_ERROR)));
        if (err == NULL) {
            g_assert_cmpstr(kernel_arg_name, ==, "b");
        } else {
            ccl_err_clear(&err);
        }

        /* Third kernel argument. */
        kaaq = ccl_kernel_get_arg_info_scalar(
            krnl, 2, CL_KERNEL_ARG_ADDRESS_QUALIFIER,
            cl_kernel_arg_address_qualifier, &err);
        g_assert_true((err == NULL)
            || ((err->code == CCL_ERROR_INFO_UNAVAILABLE_OCL)
                && (err->domain == CCL_ERROR))
            || ((err->code == CL_KERNEL_ARG_INFO_NOT_AVAILABLE)
                && (err->domain == CCL_OCL_ERROR)));
        if (err == NULL) {
            g_assert_cmphex(kaaq, ==, CL_KERNEL_ARG_ADDRESS_GLOBAL);
        } else {
            ccl_err_clear(&err);
        }

        kernel_arg_type_name = ccl_kernel_get_arg_info_array(
            krnl, 2, CL_KERNEL_ARG_TYPE_NAME, char, &err);
        g_assert_true((err == NULL)
            || ((err->code == CCL_ERROR_INFO_UNAVAILABLE_OCL)
                && (err->domain == CCL_ERROR))
            || ((err->code == CL_KERNEL_ARG_INFO_NOT_AVAILABLE)
                && (err->domain == CCL_OCL_ERROR)));
        if (err == NULL) {
            g_assert_cmpstr(kernel_arg_type_name, ==, "uint*");
        } else {
            ccl_err_clear(&err);
        }

        kernel_arg_name = ccl_kernel_get_arg_info_array(
            krnl, 2, CL_KERNEL_ARG_NAME, char, &err);
        g_assert_true((err == NULL)
            || ((err->code == CCL_ERROR_INFO_UNAVAILABLE_OCL)
                && (err->domain == CCL_ERROR))
            || ((err->code == CL_KERNEL_ARG_INFO_NOT_AVAILABLE)
                && (err->domain == CCL_OCL_ERROR)));
        if (err == NULL) {
            g_assert_cmpstr(kernel_arg_name, ==, "c");
        } else {
            ccl_err_clear(&err);
        }

        /* Fourth kernel argument. */
        kaaq = ccl_kernel_get_arg_info_scalar(
            krnl, 3, CL_KERNEL_ARG_ADDRESS_QUALIFIER,
            cl_kernel_arg_address_qualifier, &err);
        g_assert_true((err == NULL)
            || ((err->code == CCL_ERROR_INFO_UNAVAILABLE_OCL)
                && (err->domain == CCL_ERROR))
            || ((err->code == CL_KERNEL_ARG_INFO_NOT_AVAILABLE)
                && (err->domain == CCL_OCL_ERROR)));
        if (err == NULL) {
            g_assert_cmphex(kaaq, ==, CL_KERNEL_ARG_ADDRESS_PRIVATE);
        } else {
            ccl_err_clear(&err);
        }

        kernel_arg_type_name = ccl_kernel_get_arg_info_array(
            krnl, 3, CL_KERNEL_ARG_TYPE_NAME, char, &err);
        g_assert_true((err == NULL)
            || ((err->code == CCL_ERROR_INFO_UNAVAILABLE_OCL)
                && (err->domain == CCL_ERROR))
            || ((err->code == CL_KERNEL_ARG_INFO_NOT_AVAILABLE)
                && (err->domain == CCL_OCL_ERROR)));
        if (err == NULL) {
            g_assert_cmpstr(kernel_arg_type_name, ==, "uint");
        } else {
            ccl_err_clear(&err);
        }

        kernel_arg_name = ccl_kernel_get_arg_info_array(
            krnl, 3, CL_KERNEL_ARG_NAME, char, &err);
        g_assert_true((err == NULL)
            || ((err->code == CCL_ERROR_INFO_UNAVAILABLE_OCL)
                && (err->domain == CCL_ERROR))
            || ((err->code == CL_KERNEL_ARG_INFO_NOT_AVAILABLE)
                && (err->domain == CCL_OCL_ERROR)));
        if (err == NULL) {
            g_assert_cmpstr(kernel_arg_name, ==, "d");
        } else {
            ccl_err_clear(&err);
        }

        /* Bogus request, should return NULL and should raise an error. */
        kernel_arg_type_name = ccl_kernel_get_arg_info_array(
            krnl, 0, 0 /* invalid value */, char, &err);
        g_assert_true(kernel_arg_type_name == NULL);
        g_assert (err != NULL);
        ccl_err_clear(&err);
    }

#endif

    /* ************************************* */
    /* 7. Test binary-related functionality. */
    /* ************************************* */

    /* Save binaries for all available devices (which we will load into
     * a new program later). */
    char ** filenames;

    tmp_file_prefix = g_strdup_printf(
        "%s%ctest_", tmp_dir_name, G_DIR_SEPARATOR);

    ccl_program_save_all_binaries(
        prg, tmp_file_prefix, ".bin", &filenames, &err);
    g_assert_no_error(err);

    g_free(tmp_file_prefix);

    cl_uint num_devs = ccl_program_get_num_devices(prg, &err);
    g_assert_no_error(err);
    CCLDevice * const * devs = ccl_program_get_all_devices(prg, &err);
    g_assert_no_error(err);

    g_debug(" ==== NUMDEVS=%d =====\n", num_devs);
    for (cl_uint i = 0; i < num_devs; ++i)
        g_debug("=> '%s'\n", filenames[i]);

    /* Save binary for a specific device (which we will load into a new
     * program later). */
    tmp_file_prefix = g_strdup_printf(
        "%s%ctest_prg.bin", tmp_dir_name, G_DIR_SEPARATOR);

    ccl_program_save_binary(prg, d, tmp_file_prefix, &err);
    g_assert_no_error(err);

    /* Save all binaries without keeping the filenames and an empty suffix
     * (these will be discarded, just test the function). */
    ccl_program_save_all_binaries(prg, tmp_file_prefix, "", NULL, &err);
    g_assert_no_error(err);

    /* Create a new program using the saved binaries. */
    prg2 = ccl_program_new_from_binary_files(
        ctx, num_devs, devs, (const char **) filenames, NULL, &err);
    g_assert_no_error(err);

    /* Destroy the filenames string array. */
    ccl_strv_clear(filenames);

    /* Destroy program created with saved binary files. */
    ccl_program_destroy(prg2);

    /* Get binary in variable. */
    prg_bin = ccl_program_get_binary(prg, d, &err);
    g_assert_no_error(err);

    /* Test program creation with binary. */
    for (guint i = 0; i < 2; i++) {

        if (i == 0) {
            /* Create program using the ccl_program_new_from_binaries()
             * function. */
            prg2 = ccl_program_new_from_binaries(
                ctx, 1, &d, &prg_bin, NULL, &err);
        } else if (i == 1) {
            /* Create program using ccl_program_new_from_binary() helper
             * funcion. */
            prg2 = ccl_program_new_from_binary(ctx, d, prg_bin, NULL, &err);
        }
        g_assert_no_error(err);

        /* Check that device is the correct one. */
        d2 = ccl_program_get_device(prg2, 0, &err);
        g_assert_no_error(err);
        g_assert_cmphex(GPOINTER_TO_SIZE(d), ==, GPOINTER_TO_SIZE(d2));

        devices = ccl_program_get_info_array(
            prg2, CL_PROGRAM_DEVICES, cl_device_id, &err);
        g_assert_no_error(err);
        g_assert_cmphex(GPOINTER_TO_SIZE(devices[0]),
            ==, GPOINTER_TO_SIZE(ccl_device_unwrap(d)));

        context = ccl_program_get_info_scalar(
            prg2, CL_PROGRAM_CONTEXT, cl_context, &err);
        g_assert_no_error(err);
        g_assert_cmphex(GPOINTER_TO_SIZE(context),
            ==, GPOINTER_TO_SIZE(ccl_context_unwrap(ctx)));

        /* Destroy program created with binary. */
        ccl_program_destroy(prg2);

    }

    /* ********************************************** */
    /* 8. Test program created with wrap constructor. */
    /* ********************************************** */

    /* Create program using the wrap constructor. */
    prg2 = ccl_program_new_wrap(ccl_program_unwrap(prg));

    /* It must be the same program as the original one. */
    g_assert_cmphex(GPOINTER_TO_SIZE(prg), ==, GPOINTER_TO_SIZE(prg2));

    /* Destroy it. */
    ccl_program_destroy(prg2);

    /* Destroy original program. */
    ccl_program_destroy(prg);

    /* ******************************** */
    /* 9. Create a program from binary. */
    /* ******************************** */

    /* Create a new program using the specifically saved binary. */
    prg = ccl_program_new_from_binary_file(
        ctx, d, tmp_file_prefix, NULL, &err);
    g_assert_no_error(err);

    g_free(tmp_file_prefix);

    /* **** BUILD PROGRAM **** */

    /* Use the build_full function for testing, not really required
     * (we could have used the "short" version). */
    ccl_program_build_full(prg, 1, &d, NULL, NULL, NULL, &err);
    g_assert_no_error(err);

    /* ***************************************************************** */
    /* 10. Get some program build info, compare it with expected values. */
    /* ***************************************************************** */

    /* Get build status. */
    info = ccl_program_get_build_info(
        prg, d, CL_PROGRAM_BUILD_STATUS, &err);
    g_assert_no_error(err);
    g_assert_true(*((cl_build_status *) info->value) == CL_BUILD_SUCCESS);

    /* Get build log via program build info. */
    info = ccl_program_get_build_info(
        prg, d, CL_PROGRAM_BUILD_LOG, &err);
    g_assert_true((err == NULL) || ((err->code == CCL_ERROR_INFO_UNAVAILABLE_OCL) &&
        (err->domain == CCL_ERROR)));
    ccl_err_clear(&err);

    /* Get concatenated build log (ie build logs for all devices associated
     * with the program). */
    for (int i = 0; i < 2; i++) {

        /* We get the build log two times at the program level to test for
         * clearing concatenated build log cache . */
        build_log = ccl_program_get_build_log(prg, &err);
        g_assert_true((err == NULL) || ((err->code == CCL_ERROR_INFO_UNAVAILABLE_OCL) &&
            (err->domain == CCL_ERROR)));
        if (info) {
            g_assert_true(g_strrstr(build_log, (char *) info->value));
        }
        ccl_err_clear(&err);
    }

    /* Get build log via program build info array. */
    build_log = ccl_program_get_build_info_array(
        prg, d, CL_PROGRAM_BUILD_LOG, char, &err);
    g_assert_true((err == NULL) || ((err->code == CCL_ERROR_INFO_UNAVAILABLE_OCL) &&
        (err->domain == CCL_ERROR)));
    if (info) {
        g_assert_cmpstr(build_log, ==, (char *) info->value);
    }
    ccl_err_clear(&err);

    /* ***** */
    /* Done! */
    /* ***** */

    /* Confirm that memory allocated by wrappers has not yet been freed. */
    g_assert_false(ccl_wrapper_memcheck());

    /* Destroy stuff. */
    ccl_program_destroy(prg);
    ccl_context_destroy(ctx);

    /* Free strings. */
    g_free(tmp_dir_name);

    /* Confirm that memory allocated by wrappers has been properly
     * freed. */
    g_assert_true(ccl_wrapper_memcheck());
}

/**
 * @internal
 *
 * @brief Test running kernels via the program class.
 * */
static void execute_test() {

    /* Test variables. */
    CCLContext * ctx = NULL;
    CCLProgram * prg = NULL;
    CCLKernel * krnl = NULL;
    CCLDevice * d = NULL;
    CCLQueue * cq = NULL;
    size_t gws;
    size_t lws;
    cl_uint a_h[CCL_TEST_PROGRAM_BUF_SIZE];
    cl_uint b_h[CCL_TEST_PROGRAM_BUF_SIZE];
    cl_uint c_h[CCL_TEST_PROGRAM_BUF_SIZE];
    cl_uint d_h ;
    CCLBuffer * a_w;
    CCLBuffer * b_w;
    CCLBuffer * c_w;
    CCLEvent * evt_w1;
    CCLEvent * evt_w2;
    CCLEvent * evt_kr;
    CCLEvent * evt_r1;
    CCLEventWaitList ewl = NULL;
    CCLErr * err = NULL;

    /* Create a context with devices from first available platform. */
    ctx = ccl_test_context_new(0, &err);
    g_assert_no_error(err);

    /* Get first device in context (and in program). */
    d = ccl_context_get_device(ctx, 0, &err);
    g_assert_no_error(err);

    /* Create a command queue. */
    cq = ccl_queue_new(ctx, d, CL_QUEUE_PROFILING_ENABLE, &err);
    g_assert_no_error(err);

    /* Set kernel enqueue properties and initialize host data. */
    gws = CCL_TEST_PROGRAM_BUF_SIZE;
    lws = CCL_TEST_PROGRAM_LWS;

    /* Test 3 ways of running kernels via the program class. */
    for (cl_uint i = 0; i < 3; i++) {

        /* Create a program. */
        prg = ccl_program_new_from_source(ctx, CCL_TEST_PROGRAM_SUM_CONTENT, &err);
        g_assert_no_error(err);

        /* Build program. */
        ccl_program_build(prg, NULL, &err);
        g_assert_no_error(err);

        /* Populate host buffers. */
        for (cl_uint j = 0; j < CCL_TEST_PROGRAM_BUF_SIZE; ++j) {
            a_h[j] = (cl_uint) g_test_rand_int();
            b_h[j] = (cl_uint) g_test_rand_int();
        }
        d_h = (cl_uint) g_test_rand_int();

        /* Create device buffers. */
        a_w = ccl_buffer_new(ctx, CL_MEM_READ_ONLY,
            CCL_TEST_PROGRAM_BUF_SIZE * sizeof(cl_uint), NULL, &err);
        g_assert_no_error(err);
        b_w = ccl_buffer_new(ctx, CL_MEM_READ_ONLY,
            CCL_TEST_PROGRAM_BUF_SIZE * sizeof(cl_uint), NULL, &err);
        g_assert_no_error(err);
        c_w = ccl_buffer_new(ctx, CL_MEM_WRITE_ONLY,
            CCL_TEST_PROGRAM_BUF_SIZE * sizeof(cl_uint), NULL, &err);
        g_assert_no_error(err);

        /* Copy host data to device buffers without waiting for transfer
        * to terminate before continuing host program. */
        evt_w1 = ccl_buffer_enqueue_write(
            a_w, cq, CL_FALSE, 0,
            CCL_TEST_PROGRAM_BUF_SIZE * sizeof(cl_uint), a_h, NULL, &err);
        g_assert_no_error(err);
        evt_w2 = ccl_buffer_enqueue_write(
            b_w, cq, CL_FALSE, 0,
            CCL_TEST_PROGRAM_BUF_SIZE * sizeof(cl_uint), b_h, NULL, &err);
        g_assert_no_error(err);

        /* Initialize event wait list and add the two transfer events. */
        ccl_event_wait_list_add(&ewl, evt_w1, evt_w2, NULL);

        /* Execute kernel via program class in three different ways: */
        if (i == 0) {
            /* Use ccl_program_enqueue_kernel_v() with args */

            void * args[] = {a_w, b_w, c_w, ccl_arg_priv(d_h, cl_uint), NULL};
            evt_kr =  ccl_program_enqueue_kernel_v(
                prg, CCL_TEST_PROGRAM_SUM, cq,
                1, NULL ,&gws, &lws, &ewl, args, &err);
            /* Waiting for the two transfer events to terminate will empty
             * the event wait list). */
            g_assert_no_error(err);

        } else if (i == 1) {
            /* Use ccl_program_enqueue_kernel() with args */

            evt_kr =  ccl_program_enqueue_kernel(
                prg, CCL_TEST_PROGRAM_SUM, cq,
                1, NULL ,&gws, &lws, &ewl, &err,
                a_w, b_w, c_w, ccl_arg_priv(d_h, cl_uint), NULL);
            /* Waiting for the two transfer events to terminate will empty
             * the event wait list). */
            g_assert_no_error(err);

        } else if (i == 2) {
            /* Use ccl_program_enqueue_kernel() without args, setting
             * them previously and separately.*/

            /* Kernel kernel and set arguments to it directly. */
            krnl = ccl_program_get_kernel(prg, CCL_TEST_PROGRAM_SUM, &err);
            g_assert_no_error(err);

            ccl_kernel_set_args(
                krnl, a_w, b_w, c_w, ccl_arg_priv(d_h, cl_uint), NULL);

            /* Run kernel via program class without setting arguments. */
            evt_kr =  ccl_program_enqueue_kernel(
                prg, CCL_TEST_PROGRAM_SUM, cq,
                1, NULL ,&gws, &lws, &ewl, &err, NULL);

            /* Waiting for the two transfer events to terminate will empty
             * the event wait list). */
            g_assert_no_error(err);
        }

        /* Add the kernel termination event to the wait list. */
        ccl_event_wait_list_add(&ewl, evt_kr, NULL);

        /* Sync. queue for events in wait list (just the kernel event in
        * this case) to terminate before going forward... */
        ccl_enqueue_barrier(cq, &ewl, &err);
        g_assert_no_error(err);

        /* Read back results from host without waiting for
        * transfer to terminate before continuing host program.. */
        evt_r1 = ccl_buffer_enqueue_read(
            c_w, cq, CL_FALSE, 0,
            CCL_TEST_PROGRAM_BUF_SIZE * sizeof(cl_uint), c_h, NULL, &err);
        g_assert_no_error(err);

        /* Add read back results event to wait list. */
        ccl_event_wait_list_add(&ewl, evt_r1, NULL);

        /* Wait for all events in wait list to terminate (this will empty
        * the wait list). */
        ccl_event_wait(&ewl, &err);
        g_assert_no_error(err);

        /* Check results are as expected. */
        for (cl_uint j = 0; j < CCL_TEST_PROGRAM_BUF_SIZE; j++) {
            g_assert_cmpuint(c_h[j], ==, a_h[j] + b_h[j] + d_h);
            g_debug("c_h[%d] = %d\n", i, c_h[j]);
        }

        /* Destroy the memory objects. */
        ccl_buffer_destroy(a_w);
        ccl_buffer_destroy(b_w);
        ccl_buffer_destroy(c_w);

        /* Destroy the program. */
        ccl_program_destroy(prg);

        /* Confirm that memory allocated by wrappers has not yet been freed. */
        g_assert_false(ccl_wrapper_memcheck());
    }

    /* Destroy stuff. */
    ccl_queue_destroy(cq);
    ccl_context_destroy(ctx);

    /* Confirm that memory allocated by wrappers has been properly freed. */
    g_assert_true(ccl_wrapper_memcheck());
}

/**
 * @internal
 *
 * @brief Test program and kernel wrappers ref counting.
 * */
static void ref_unref_test() {

    CCLContext * ctx = NULL;
    CCLErr * err = NULL;
    CCLProgram * prg = NULL;
    CCLKernel * krnl1 = NULL;
    CCLKernel * krnl2 = NULL;

    const char * src = CCL_TEST_PROGRAM_SUM_CONTENT;

    /* Get some context. */
    ctx = ccl_test_context_new(0, &err);
    g_assert_no_error(err);

    /* Create a program from source. */
    prg = ccl_program_new_from_source(ctx, src, &err);
    g_assert_no_error(err);

    /* Build program. */
    ccl_program_build(prg, NULL, &err);
    g_assert_no_error(err);

    /* Get kernel wrapper from program (will be the instance kept in the
     * program wrapper). */
    krnl1 = ccl_program_get_kernel(prg, CCL_TEST_PROGRAM_SUM, &err);
    g_assert_no_error(err);

    /* Create another kernel wrapper for the same kernel. This should
     * yield a different object because we're not getting it from
     * the program wrapper. */
    krnl2 = ccl_kernel_new(prg, CCL_TEST_PROGRAM_SUM, &err);
    g_assert_no_error(err);

    /* Check that they're different. */
    g_assert_cmphex(GPOINTER_TO_SIZE(krnl1), !=, GPOINTER_TO_SIZE(krnl2));

    /* Check that each has a ref count of 1. */
    g_assert_cmpuint(ccl_wrapper_ref_count((CCLWrapper *) krnl1), ==, 1);
    g_assert_cmpuint(ccl_wrapper_ref_count((CCLWrapper *) krnl2), ==, 1);

    /* Increment the ref count of the directly created kernel. */
    ccl_kernel_ref(krnl2);
    g_assert_cmpuint(ccl_wrapper_ref_count((CCLWrapper *) krnl1), ==, 1);
    g_assert_cmpuint(ccl_wrapper_ref_count((CCLWrapper *) krnl2), ==, 2);

    /* Get rid of the directly created kernel. */
    ccl_kernel_unref(krnl2);
    ccl_kernel_unref(krnl2);

    /* Reference the program object, check its ref count. */
    ccl_program_ref(prg);
    g_assert_cmpuint(ccl_wrapper_ref_count((CCLWrapper *) prg), ==, 2);
    ccl_program_unref(prg);

    /* Confirm that memory allocated by wrappers has not yet been freed. */
    g_assert_false(ccl_wrapper_memcheck());

    /* Destroy remaining stuff. */
    ccl_program_destroy(prg);
    ccl_context_destroy(ctx);

    /* Confirm that memory allocated by wrappers has been properly freed. */
    g_assert_true(ccl_wrapper_memcheck());
}

#ifdef CL_VERSION_1_2

static const char * src_head[] = {
    "#define SOMETYPE char\n",
    "SOMETYPE some_function(SOMETYPE a, size_t b) {\n" \
    "	return (SOMETYPE) (a + b);\n" \
    "}\n"};

static const char src_main[] =
    "#include \"head.h\"\n" \
    "__kernel void complinktest(__global SOMETYPE * buf) {\n" \
    "	size_t gid = get_global_id(0);\n" \
    "	buf[gid] = some_function(buf[gid], gid);\n" \
    "}\n";

static const char * src_head_name = "head.h";

#endif

/**
 * @internal
 *
 * @brief Test program and kernel wrappers ref counting.
 * */
static void compile_link_test() {

#ifndef CL_VERSION_1_2

    g_test_skip(
        "Test skipped due to lack of OpenCL 1.2 support.");

#else

    /* Test variables. */
    CCLContext * ctx = NULL;
    CCLDevice * dev = NULL;
    CCLQueue * cq = NULL;
    CCLProgram * prg_head = NULL;
    CCLProgram * prg_main = NULL;
    CCLProgram * prg_exec = NULL;
    CCLBuffer * buf = NULL;
    cl_char hbuf_in[8] = {-3, -2, -1, 0, 1, 2, 3, 4};
    cl_char hbuf_out[8];
    size_t ws = 8;
    CCLErr * err = NULL;

    /* Get the test context with the pre-defined device. */
    ctx = ccl_test_context_new(120, &err);
    g_assert_no_error(err);
    if (!ctx) return;

    /* Get first device in context. */
    dev = ccl_context_get_device(ctx, 0, &err);
    g_assert_no_error(err);

    /* Create a command queue. */
    cq = ccl_queue_new(ctx, dev, 0, &err);
    g_assert_no_error(err);

    /* Create device buffer and initialize it with values from host
     * buffer in. */
    buf = ccl_buffer_new(
        ctx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 8, hbuf_in, &err);
    g_assert_no_error(err);

    /* Create header program. */
    prg_head =
        ccl_program_new_from_sources(ctx, 2, src_head, NULL, &err);
    g_assert_no_error(err);

    /* Create main program */
    prg_main = ccl_program_new_from_source(ctx, src_main, &err);
    g_assert_no_error(err);

    /* Compile main program. */
    ccl_program_compile(
        prg_main, 1, &dev, NULL, 1, &prg_head,
        &src_head_name, NULL, NULL, &err);
    g_assert_no_error(err);

    /* Link programs into an executable program. */
    prg_exec = ccl_program_link(
        ctx, 1, &dev, NULL, 1, &prg_main, NULL, NULL, &err);
    g_assert_no_error(err);

    /* Run program. */
    ccl_program_enqueue_kernel(
        prg_exec, "complinktest", cq, 1, NULL,
        &ws, &ws, NULL, &err, buf, NULL);
    g_assert_no_error(err);

    /* Read results back to host. */
    ccl_buffer_enqueue_read(buf, cq, CL_TRUE, 0, 8, hbuf_out, NULL, &err);
    g_assert_no_error(err);

    /* Terminate queue. */
    ccl_queue_finish(cq, &err);
    g_assert_no_error(err);

    /* Check results. */
    for (cl_char i = 0; i < 8; ++i)
        g_assert_cmpint(hbuf_out[i], ==, hbuf_in[i] + i);

    /* Confirm that memory allocated by wrappers has not yet been freed. */
    g_assert_false(ccl_wrapper_memcheck());

    /* Free stuff. */
    ccl_buffer_destroy(buf);
    ccl_program_destroy(prg_exec);
    ccl_program_destroy(prg_main);
    ccl_program_destroy(prg_head);
    ccl_queue_destroy(cq);
    ccl_context_destroy(ctx);

    /* Confirm that memory allocated by wrappers has been properly freed. */
    g_assert_true(ccl_wrapper_memcheck());

#endif

}

/**
 * @internal
 *
 * @brief Test error conditions.
 * */
static void errors_test() {

    /* Test variables. */
    CCLContext * ctx = NULL;
    CCLDevice * dev = NULL;
    CCLProgram * prg = NULL;
    CCLErr * err = NULL;
    const char * bad_src[] = { NULL, "text", "more text" };
    const size_t bad_src_len[] = { 4, 5, 5 };

    /* Get the test context with the pre-defined device. */
    ctx = ccl_test_context_new(0, &err);
    g_assert_no_error(err);
    dev = ccl_context_get_device(ctx, 0, &err);
    g_assert_no_error(err);

    /* ********************************************************* */
    /* 1. Check error when creating program with invalid source. */
    /* ********************************************************* */
    prg = ccl_program_new_from_sources(ctx, 3, bad_src, bad_src_len, &err);
    g_assert_null(prg);
    g_assert_error(err, CCL_OCL_ERROR, CL_INVALID_VALUE);
    g_clear_error(&err);

#ifdef CL_VERSION_1_2

    /* ********************************************************************* */
    /* 2. Check error when trying to create a program with built-in kernels. */
    /* ********************************************************************* */
    prg = ccl_program_new_from_built_in_kernels(
        ctx, 1, &dev, "badkernel1;badkernel2", &err);
    g_assert_null(prg);
    g_assert_nonnull(err);
    g_assert(err->domain == CCL_OCL_ERROR);
    g_clear_error(&err);

#endif

    /* Free stuff. */
    ccl_context_destroy(ctx);

    /* Confirm that memory allocated by wrappers has been properly freed. */
    g_assert_true(ccl_wrapper_memcheck());
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
        "/wrappers/program/create-info-destroy",
        create_info_destroy_test);

    g_test_add_func(
        "/wrappers/program/execute",
        execute_test);

    g_test_add_func(
        "/wrappers/program/ref-unref",
        ref_unref_test);

    g_test_add_func(
        "/wrappers/program/compile-link",
        compile_link_test);

    g_test_add_func(
        "/wrappers/program/errors",
        errors_test);

    return g_test_run();
}
