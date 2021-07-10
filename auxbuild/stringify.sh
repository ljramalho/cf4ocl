#!/usr/bin/env bash
#
# Stringify an OpenCL kernel file given in the standard input.
#
# Requires: sed awk
# Author: Nuno Fachada <faken@fakenmc.com>
# Licence: GNU General Public License version 3 (GPLv3)
# Date: 2019
#

# Remove C-style comments
sed -r ':a; s%(.*)/\*.*\*/%\1%; ta; /\/\*/ !b; N; ba' <&0 | \
    # Remove empty lines
    awk 'NF > 0' | \
    # "Stringify"
    sed -e 's/\\/\\\\/g;s/"/\\"/g;s/^/"/;s/$/\\n"/' | \
    # Add tabs before and backslash after each line
    awk '{printf "\t%s \\\n", $0}'
    # Add empty line without backslash
    echo -e "\t\"\""
