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
	RESOURCE_NAME="${1}"; shift
	OUT_FILE="${1}"; shift
	PY_OUT_FILE="`dirname "$OUT_FILE"`/`basename $OUT_FILE .hpp`.py"
	mkdir -p "`dirname "$OUT_FILE"`"
	(echo "// This file is auto-generated. Do not modify."
	echo
	echo "#pragma once"
	echo
	echo "#ifndef RES_${RESOURCE_NAME}_HPP"
	echo "#define RES_${RESOURCE_NAME}_HPP"
	echo
	echo "#include <cypress/util/resource.hpp>"
	echo
	echo "namespace cypress {"
	echo "const Resource Resources::${RESOURCE_NAME}(std::vector<uint8_t>({"
	cat $* | minify | tee $PY_OUT_FILE | hexdump -v -e '1 1 "0x%02x,"'
	echo -n "}));"
	echo "}"
	echo
	echo "#endif /* RES_${RESOURCE_NAME}_HPP */") > $OUT_FILE
	chmod +x $PY_OUT_FILE # Mark the concatenated python file as executable
}

# Download the hbp_neuromorphic_platform python package if it does not exist yet
if [ ! -f "$DIR/backend/nmpi/lib/nmpi/nmpi_user.py" ]; then
	echo "Downloading current version of the hbp_neuromorphic_platform package"
	mkdir -p "$DIR/backend/nmpi/lib"
	pip install -t "$DIR/backend/nmpi/lib" hbp_neuromorphic_platform
	echo "Done."
fi

# Build the PyNN script Python code into a resource
build_resource PYNN_INTERFACE "$OUT_DIR/pynn/pynn_interface.hpp" \
	"$DIR/backend/pynn/constants.py" \
	"$DIR/backend/pynn/binnf.py" \
	"$DIR/backend/pynn/cypress.py" \
	"$DIR/backend/pynn/cli.py"

# Build the PyNN script Python code into a resource
build_resource PYNN_BINNF_LOOPBACK "$OUT_DIR/pynn/pynn_binnf_loopback.hpp" \
	"$DIR/backend/pynn/binnf.py" \
	"$DIR/backend/pynn/loopback.py"

# Build the NMPI Python code into a resource
build_resource NMPI_BROKER "$OUT_DIR/nmpi/nmpi_broker.hpp" \
	"$DIR/backend/nmpi/header.py" \
	"$DIR/backend/nmpi/lib/nmpi/nmpi_user.py" \
	"$DIR/backend/nmpi/broker.py"

