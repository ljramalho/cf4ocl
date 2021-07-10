#!/usr/bin/env bash
#
# This script scrapes the OpenCL function names from the OpenCL headers and
# obtains the most recent documentation URLs for each function.
#
# Requires: ctags curl cut head
# Author: Nuno Fachada <faken@fakenmc.com>
# Licence: GNU General Public License version 3 (GPLv3)
# Date: 2019
#
ocl_headers="../ocl/CL/*"
macros_to_ignore=$(grep -h -o "#define .*_[0-9]_[0-9].*" ${ocl_headers} | \
    sort | cut -d " " -f 2 | uniq | tr '\n' ',')
macros_to_ignore="${macros_to_ignore}CL_API_ENTRY,CL_API_CALL"
doc_url_base="https://www.khronos.org/registry/OpenCL/sdk"

ocl_versions="2.1 2.0 1.2 1.1 1.0"
ocl_functions=$(LC_ALL=C ctags -I $macros_to_ignore -x --c-kinds=p \
    ${ocl_headers} | cut -f1 -d " ")
for ocl_fun in $ocl_functions
do
    for ocl_ver in $ocl_versions
    do
        doc_url="${doc_url_base}/${ocl_ver}/docs/man/xhtml/${ocl_fun}.html"
        doc_status=$(curl -s --head ${doc_url} | head -n 1 | cut -f2 -d " ")
        if [[ $doc_status == "200" ]]
        then
            echo "${ocl_fun}() ${doc_url}"
            break
        fi
    done
done
