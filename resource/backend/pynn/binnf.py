# -*- coding: utf-8 -*-

#   Cypress -- A C++ interface to PyNN
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

"""
Reader of the binnf serialisation format. Note that this format is not
standardised and always in sync with the corresponding implementation in CppNAM.
"""

import numpy as np
import struct


class BinnfException(Exception):
    """
    Exception type used throughout the binnf serialiser/deserialiser.
    """
    pass

# Numbers and constants defining the serialisation format

BLOCK_START_SEQUENCE = 0x665a8cda
BLOCK_END_SEQUENCE = 0x420062cb
BLOCK_TYPE_MATRIX = 0x01

TYPE_INT = 0x00
TYPE_FLOAT = 0x01
TYPE_MAP = {
    TYPE_INT: "int32",
    TYPE_FLOAT: "float32"
}

# Helper functions used to determine the length of a storage block

MAX_STR_SIZE = 1024
BLOCK_TYPE_LEN = 4
SIZE_LEN = 4
TYPE_LEN = 4
NUMBER_LEN = 4


def _str_len(s):
    """
    Returns the serialised length of a string in bytes.
    """
    return SIZE_LEN + len(s)


def _header_len(header):
    """
    Returns the serialised length of the header structure in bytes.
    """
    res = SIZE_LEN
    for elem in header:
        res += _str_len(elem["name"]) + TYPE_LEN
    return res


def _matrix_len(matrix):
    """
    Returns the serialised length of a matrix in bytes.
    """
    return 2 * SIZE_LEN + matrix.size * matrix.dtype.itemsize


def _block_len(name, header, matrix):
    """
    Returns the total length of a binnf block in bytes.
    """
    return (BLOCK_TYPE_LEN + _str_len(name) + _header_len(header)
            + _matrix_len(matrix))

# Serialisation helper functions


def _write_int(fd, i):
    fd.write(struct.pack("i", i))


def _write_str(fd, s):
    if (len(s) > MAX_STR_SIZE):
        raise BinnfException("String exceeds string size limit of " + MAX_STR_SIZE
                             + " bytes.")
    fd.write(struct.pack("i", len(s)))
    fd.write(s)

# Deserialisation helper functions


def _synchronise(fd, marker):
    sync = 0
    first = True
    while True:
        c = fd.read(1)
        if not c:
            if first:
                return False
            raise BinnfException("Unexpected end of file")
        sync = (sync >> 8) | (ord(c[0]) << 24)
        if sync == marker:
            return True
        first = False


def _read_int(fd):
    data = fd.read(4)
    if not data:
        raise BinnfException("Unexpected end of file")
    return struct.unpack("i", data)[0]


def _read_str(fd):
    data = fd.read(_read_int(fd))
    if not data:
        raise BinnfException("Unexpected end of file")
    return data


def _tell(fd):
    """
    Returns the current cursor position within the given file descriptor.
    Implements the C++ behaviour of iostream::tellg(). Returns -1 if the feature
    is not implemented (e.g. because we're reading from a stream).
    """
    try:
        return fd.tell()
    except:
        return -1


def header_to_dtype(header):
    return map(lambda x: (x["name"], TYPE_MAP[x["type"]]), header)


def serialise(fd, name, header, matrix):
    """
    Serialises a binnf data block.

    :param name: is the data block name.
    :param header: is the data block header, consisting of an array of
    dictionaries containing "name" and "type" blocks.
    :param matrix: matrix containing the data that should be serialised.
    """

    # Write the block header
    _write_int(fd, BLOCK_START_SEQUENCE)
    _write_int(fd, _block_len(name, header, matrix))
    _write_int(fd, BLOCK_TYPE_MATRIX)

    # Write the name string
    _write_str(fd, name)

    # Write the data header
    _write_int(fd, len(header))
    for i in xrange(len(header)):
        _write_str(fd, header[i]["name"])
        _write_int(fd, header[i]["type"])

    # Write the matrix data
    rows = matrix.shape[0]
    if (len(matrix.shape) == 1):
        cols = len(matrix.dtype.descr)
    else:
        cols = matrix.shape[1]

    if cols != len(header):
        raise BinnfException(
            "Disecrepancy between matrix number of columns and header")

    _write_int(fd, rows)
    _write_int(fd, cols)
    fd.write(matrix.tobytes())

    # Finalise the block
    _write_int(fd, BLOCK_END_SEQUENCE)


def deseralise(fd):
    name = ""
    header = []
    matrix = None

    # Read some meta-information, abort if we're at the end of the file
    if not _synchronise(fd, BLOCK_START_SEQUENCE):
        return None, None, None

    block_len = _read_int(fd)
    pos0 = _tell(fd)

    block_type = _read_int(fd)
    if block_type != BLOCK_TYPE_MATRIX:
        raise BinnfException("Unexpected block type")

    # Read the name
    name = _read_str(fd)

    # Read the header
    header_len = _read_int(fd)
    header = map(lambda _: {"name": "", "type": TYPE_INT},
                 xrange(header_len))
    for i in xrange(header_len):
        header[i]["name"] = _read_str(fd)
        header[i]["type"] = _read_int(fd)

    # Read the data
    rows = _read_int(fd)
    cols = _read_int(fd)
    if (cols != len(header)):
        raise BinnfException(
            "Disecrepancy between matrix number of columns and header")

    fmt = header_to_dtype(header)
    matrix = np.require(np.frombuffer(buffer(fd.read(rows * cols * NUMBER_LEN)),
                                      dtype=fmt), requirements=["WRITEABLE"])

    # Make sure the block size was correct
    pos1 = _tell(fd)
    if pos0 >= 0 and pos1 >= 0 and pos1 - pos0 != block_len:
        raise BinnfException("Invalid block length")

    # Make sure the end of the block is reached
    block_end = _read_int(fd)
    if block_end != BLOCK_END_SEQUENCE:
        raise BinnfException("Block end sequence not found")

    return name, header, matrix


def read_network(fd):
    EXPECTED_FIELDS = {
        "populations":
            {
                "count": "int32",
                "type": "int32"
            },
        "parameters":
            {
                "pid": "int32",
                "nid": "int32"
            },
        "target":
            {
                "pid": "int32",
                "nid": "int32"
            },
        "spike_times":
            {
                "times": "float32"
            },
        "connections":
            {
                "pid_src": ("int32", 0),
                "pid_tar": ("int32", 4),
                "nid_src": ("int32", 8),
                "nid_tar": ("int32", 12),
                "weight": ("float32", 16),
                "delay": ("float32", 20)
            }
    }

    def validate_matrix(name, matrix):
        """
        Makes sure the matrix contains the correct fields.
        """
        fields = matrix.dtype.fields
        if (name in EXPECTED_FIELDS):
            for name, _type in EXPECTED_FIELDS[name].items():
                if not (name in fields):
                    raise BinnfException("Expected mandatory header field \""
                                         + name + "\" of type \"" + _type + "\"")
                field = fields[name]
                type_name = _type[0] if isinstance(_type, tuple) else _type
                if field[0].name != type_name:
                    raise BinnfException("Mandatory header field \""
                                         + name + "\" must by of type " + type_name
                                         + ", but got " + field[0].name)
                if isinstance(_type, tuple) and field[1] != _type[1]:
                    raise BinnfException("Mandatory header field \""
                                         + name + "\" must be at offset " +
                                         str(_type[1])
                                         + ", but is at offset " + str(field[1]))

    # Construct the network descriptor from the binnf data
    network = {
        "parameters": [],
        "spike_times": []
    }
    target = None
    while True:
        # Deserialise a single input block
        name, _, matrix = deseralise(fd)

        # "None" is returned as soon as the end of the file is reached
        if name is None:
            break

        # Make sure all mandatory matrix fields are present
        validate_matrix(name, matrix)

        # Read the data matrices
        if name == "populations":
            if "populations" in network:
                raise BinnfException(
                    "Only a single \"populations\" instance is supported")
            network["populations"] = matrix
        elif name == "connections":
            if "connections" in network:
                raise BinnfException(
                    "Only a single \"connections\" instance is supported")
            network["connections"] = matrix
        elif name == "parameters":
            network["parameters"].append(matrix)
        elif name == "target":
            if matrix.size != 1:
                raise BinnfException(
                    "Target matrix must have exactly one element")
            target = {"pid": matrix[0]["pid"], "nid": matrix[0]["nid"]}
        elif name == "spike_times":
            if target is None:
                raise BinnfException("Target neuron was not set")
            network["spike_times"].append({
                "pid": target["pid"],
                "nid": target["nid"],
                "times": matrix
            })
            target = None
        else:
            raise BinnfException("Unsupported matrix type \"" + name + "\"")

    return network

# Headers used during serialisation
HEADER_TARGET = [{"name": "pid", "type": TYPE_INT},
                 {"name": "nid", "type": TYPE_INT}]
HEADER_TARGET_DTYPE = header_to_dtype(HEADER_TARGET)

HEADER_SPIKE_TIMES = [{"name": "times", "type": TYPE_FLOAT}]
HEADER_SPIKE_TIMES_DTYPE = header_to_dtype(HEADER_SPIKE_TIMES)

HEADER_TRACE = [{"name": "times", "type": TYPE_FLOAT},
                {"name": "values", "type": TYPE_FLOAT}]
HEADER_TRACE_DTYPE = header_to_dtype(HEADER_TRACE)

def write_result(fd, res):
    """
    Serialises the simulation result to binnf.

    :param fd: target file descriptor.
    :param res: simulation result.
    """
    for pid in xrange(len(res)):
        for signal in res[pid]:
            for nid in xrange(len(res[pid][signal])):
                serialise(fd, "target", HEADER_TARGET, np.array(
                    [(pid, nid)], dtype=HEADER_TARGET_DTYPE))
                matrix = res[pid][signal][nid]
                if signal == "spikes":
                    serialise(fd, "spike_times", HEADER_SPIKE_TIMES, matrix)
                else:
                    serialise(fd, "trace_" + signal, HEADER_TRACE, matrix)

# Export definitions

__all__ = ["serialise", "deserialise", "read_network", "BinnfException"]

