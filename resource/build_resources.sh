#!/usr/bin/env bash

#   Cypress -- C++ Spiking Neural Network Simulation Framework
#   Copyright (C) 2016 Andreas Stöckel
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
OUT_DIR=${1:-$DIR}
mkdir -p "$OUT_DIR"

HAS_PYMINIFIER=`(( echo "print(\"Hello World\");" | pyminifier /dev/fd/0 ) &> /dev/null ) && echo 1 || echo 0`;
if [ "$HAS_PYMINIFIER" = 0 ]; then
	echo "Optional dependency \"pyminifier\" not found. Install using"
	echo "    pip install pyminifier"
	echo "to reduce Cypress library size."
fi

function minify {
	if [ "$HAS_PYMINIFIER" != 0 ]; then
		pyminifier --bzip2 /dev/fd/0
	else
		cat
	fi
}

function build_resource {
	RESOURCE_NAME=${1}
	shift
	echo "// This file is auto-generated. Do not modify."
	echo
	echo "#pragma once"
	echo
	echo "#ifndef RES_${RESOURCE_NAME}_HPP"
	echo "#define RES_${RESOURCE_NAME}_HPP"
	echo
	echo "#include <cypress/util/resource.hpp>"
	echo
	echo "namespace cypress {"
	echo "const Resource Resources::${RESOURCE_NAME}(std::vector<char>({"
	cat $* | minify | hexdump -v -e '1 1 "0x%02x,"'
	echo -n "}));"
	echo "}"
	echo
	echo "#endif /* RES_${RESOURCE_NAME}_HPP */"
}

# Build the PyNN script Python code into a resource
build_resource PYNN_INTERFACE \
	"$DIR/backend/pynn/constants.py" \
	"$DIR/backend/pynn/binnf.py" \
	"$DIR/backend/pynn/cypress.py" \
	"$DIR/backend/pynn/cli.py" > "$OUT_DIR/pynn_interface.hpp"


