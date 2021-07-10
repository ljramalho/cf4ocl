/*
 * This file is part of cf4ocl (C Framework for OpenCL).
 *
 * cf4ocl is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * cf4ocl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with cf4ocl. If not, see
 * <http://www.gnu.org/licenses/>.
 * */

/**
 * @file
 * Functions for querying OpenCL devices.
 *
 * @author Nuno Fachada
 * @date 2019
 * @copyright [GNU Lesser General Public License version 3 (LGPLv3)](http://www.gnu.org/licenses/lgpl.html)
 * */

#ifndef _CCL_DEVICE_QUERY_H_
#define _CCL_DEVICE_QUERY_H_

#include <string.h>
#include "ccl_common.h"
#include "ccl_device_wrapper.h"
#include "ccl_errors.h"
#include "ccl_oclversions.h"

/**
 * @defgroup CCL_DEVICE_QUERY Device query
 *
 * This @ref CCL_DEVICE_QUERY "module" facilitates the querying of OpenCL
 * devices. It primarily supports the @ref ccl_devinfo utility, but may also be
 * of use to client code.
 *
 * @{
 */

/**
 * Output formatting function.
 *
 * @param[in] info Information wrapper object.
 * @param[out] out Character buffer (pre-allocated) where to write formatted
 * output.
 * @param[in] size Maximum output length.
 * @param[in] units Parameter units suffix.
 * @return Formatted output (same address as parameter `out`).
 * */
typedef char * (*ccl_devquery_format)(CCLWrapperInfo * info, char * out,
    size_t size, const char * units);

/**
 * Maps a string to a `cl_device_info` bitfield.
 * */
typedef struct ccl_devquery_map {

    /**
     * Parameter name string.
     * @public
     * */
    const char * const param_name;

    /**
     * CL device information bitfield.
     * @public
     * */
    const cl_device_info device_info;

    /**
     * Long description of parameter.
     * @public
     * */
    const char * const description;

    /**
     * Output formatting function.
     * @public
     * */
    const ccl_devquery_format format;

    /**
     * Parameter units suffix.
     * @public
     * */
    const char * const units;

} CCLDevQueryMap;

/** Size of parameter information map. */
CCL_EXPORT
extern const int ccl_devquery_info_map_size;

/** Map of parameter name strings to respective `cl_device_info`
 * bitfields, long description string, format output function and a
 * units suffix. */
CCL_EXPORT
extern const CCLDevQueryMap ccl_devquery_info_map[];

/* Return the index of the device information map object of the
 * given parameter name. */
CCL_EXPORT
int ccl_devquery_get_index(const char * name);

/* Get a final device info prefix in the same format as
 * kept in the ccl_devquery_info_map. */
CCL_EXPORT
gchar * ccl_devquery_get_prefix_final(const char * prefix);

/* Return a `cl_device_info` object given its name. */
CCL_EXPORT
cl_device_info ccl_devquery_name(const char * name);

/* Search for a device information parameter by matching part
 * of its name. This function is supposed to be used in a loop. */
CCL_EXPORT
const CCLDevQueryMap * ccl_devquery_match(const char * substr, int * idx);

/**
 * Map an OpenCL `cl_device_type` object to a string identifying
 * the device type.
 *
 * @param[in] type The OpenCL `cl_device_type`.
 * @return String identifying device type.
 * */
#define ccl_devquery_type2str(type) \
    (((type) & CL_DEVICE_TYPE_CPU) ? "CPU" : \
        (((type) & CL_DEVICE_TYPE_GPU) ? "GPU" : \
            (((type) & CL_DEVICE_TYPE_ACCELERATOR) ? "Accelerator" : \
                (((type) & CL_DEVICE_TYPE_CUSTOM) ? "Custom" : \
                    "Unknown"))))

/** @} */

#endif
