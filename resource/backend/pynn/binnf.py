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
BLOCK_TYPE_LOG = 0x02

TYPE_INT8 = 0
TYPE_UINT8 = 1
TYPE_INT16 = 2
TYPE_UINT16 = 3
TYPE_INT32 = 4
TYPE_UINT32 = 5
TYPE_FLOAT32 = 6
TYPE_INT64 = 7
TYPE_FLOAT64 = 8

TYPE_MAP = {
    TYPE_INT8: "int8",
    TYPE_UINT8: "uint8",
    TYPE_INT16: "int16",
    TYPE_UINT16: "uint16",
    TYPE_INT32: "int32",
    TYPE_UINT32: "uint32",
    TYPE_FLOAT32: "float32",
    TYPE_INT64: "int64",
    TYPE_FLOAT64: "float64"
}

INV_TYPE_MAP = {v: k for k, v in TYPE_MAP.iteritems()}

SEV_DEBUG = 10
SEV_INFO = 20
SEV_WARNING = 30
SEV_ERROR = 40
SEV_FATAL = 50

# Helper functions used to determine the length of a storage block

BLOCK_TYPE_LEN = 4
SIZE_LEN = 4
TYPE_LEN = 4


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
    return SIZE_LEN + matrix.size * matrix.dtype.itemsize


def _matrix_block_len(name, header, matrix):
    """
    Returns the total length of a binnf block in bytes.
    """
    return (BLOCK_TYPE_LEN + _str_len(name) + _header_len(header) +
            _matrix_len(matrix))


def _log_block_len(module, msg):
    """
    Returns the total length of a binnf block in bytes.
    """
    return (BLOCK_TYPE_LEN + 12 + _str_len(module) + _str_len(msg))

# Serialisation helper functions


def _write_int(fd, i):
    fd.write(struct.pack("i", i))


def _write_double(fd, d):
    fd.write(struct.pack("d", d))


def _write_str(fd, s):
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


def _read_double(fd):
    data = fd.read(8)
    if not data:
        raise BinnfException("Unexpected end of file")
    return struct.unpack("d", data)[0]


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
    return np.dtype(map(lambda x: (x["name"], TYPE_MAP[x["type"]]), header))


def dtype_to_header(dt):
    """
    Sorts the fields in the numpy datatype dt by their byte offset and converts
    them into an array of dictionaries containing the name and type of each
    entry.
    """
    return map(lambda x: {"name": x[0], "type": INV_TYPE_MAP[x[1][0].name]},
               sorted(dt.fields.items(), key=lambda x: x[1][1]))


def serialise_matrix(fd, name, matrix):
    """
    Serialises a binnf data block.

    :param name: is the data block name.
    :param header: is the data block header, consisting of an array of
    dictionaries containing "name" and "type" blocks.
    :param matrix: matrix containing the data that should be serialised.
    """

    # Fetch the matrix header from the matrix
    matrix = np.require(matrix, requirements=["C_CONTIGUOUS"])
    header = dtype_to_header(matrix.dtype)

    # Write the block header
    _write_int(fd, BLOCK_START_SEQUENCE)
    _write_int(fd, _matrix_block_len(name, header, matrix))
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
    if hasattr(matrix, 'tobytes'):  # only exists since Numpy 1.9
        fd.write(matrix.tobytes())
    else:
        matrix.tofile(fd, sep="")

    # Finalise the block
    _write_int(fd, BLOCK_END_SEQUENCE)


def serialise_log(fd, time, severity, module, msg):
    """
    Serialises a log message and sends it to the receiving side.

    :param time: is the Unix timestamp at which the message was recorded
    :param severity: is the severity level of the message, should be one of
    SEV_DEBUG, SEV_INFO, SEV_WARNING, SEV_ERROR or SEV_FATAL.
    :param module: is the name of the module the error message originated from
    :param msg: is the actual message that should be logged.
    """
    _write_int(fd, BLOCK_START_SEQUENCE)
    _write_int(fd, _log_block_len(module, msg))
    _write_int(fd, BLOCK_TYPE_LOG)

    _write_double(fd, time)
    _write_int(fd, severity)
    _write_str(fd, module)
    _write_str(fd, msg)

    _write_int(fd, BLOCK_END_SEQUENCE)



def deserialise_matrix(fd):
    # Read the name
    name = _read_str(fd)

    # Read the header
    header_len = _read_int(fd)
    header = map(lambda _: {"name": "", "type": 0}, xrange(header_len))
    for i in xrange(header_len):
        header[i]["name"] = _read_str(fd)
        header[i]["type"] = _read_int(fd)

    # Read the data
    rows = _read_int(fd)
    fmt = header_to_dtype(header)
    matrix = np.require(
        np.frombuffer(
            buffer(fd.read(rows * fmt.itemsize)), dtype=fmt),
        requirements=["WRITEABLE", "C_CONTIGUOUS"])
    return name, header, matrix


def deserialise_log(fd):
    """
    Reads a log message from the given file descriptor. A log message consists
    of a double containing the log timestamp, the severity string, the module
    string, and the actual message.
    """
    time = _read_double(fd)
    severity = _read_int(fd)
    module = _read_str(fd)
    msg = _read_str(fd)
    return time, severity, module, msg


def deserialise(fd):
    """
    Deserialises a Binnf block from the sequence. Returns the block type and
    a tuple containing the actual data
    """
    # Read some meta-information, abort if we're at the end of the file
    if not _synchronise(fd, BLOCK_START_SEQUENCE):
        return None, None

    block_len = _read_int(fd)
    pos0 = _tell(fd)

    block_type = _read_int(fd)
    if block_type == BLOCK_TYPE_MATRIX:
        res = deserialise_matrix(fd)
    elif block_type == BLOCK_TYPE_LOG:
        res = deserialise_log(fd)
    else:
        raise BinnfException("Unexpected block type")

    # Make sure the block size was correct
    pos1 = _tell(fd)
    if pos0 >= 0 and pos1 >= 0 and pos1 - pos0 != block_len:
        raise BinnfException("Invalid block length")

    # Make sure the end of the block is reached
    block_end = _read_int(fd)
    if block_end != BLOCK_END_SEQUENCE:
        raise BinnfException("Block end sequence not found")

    return block_type, res


def read_network(fd):
    EXPECTED_FIELDS = {
        "populations": ["count", "type"],
        "parameters": ["pid", "nid"],
        "target": ["pid", "nid"],
        "spike_times": ["times"],
        "list_connection": ["nid_src", "nid_tar", "weight", "delay"],
        "list_connection_header": ["pid_src", "pid_tar", "inh", "file"],
        "group_connections":
        ["pid_src", "nid_src_start", "nid_src_end", "pid_tar", "nid_tar_start",
         "nid_tar_end", "connector_id", "weight", "delay", "parameter"],
    }

    def validate_matrix(name, matrix):
        """
        Makes sure the matrix contains the correct fields.
        """
        fields = matrix.dtype.fields
        if (name in EXPECTED_FIELDS):
            for n in EXPECTED_FIELDS[name]:
                if not (n in fields):
                    raise BinnfException("Expected mandatory header field \"" +
                                         name + "\"")

    # Construct the network descriptor from the binnf data
    network = {"parameters": [], "spike_times": [],
               "signals": [], "list_connections": []}
    target = None
    while True:
        # Deserialise a single input block
        block_type, res = deserialise(fd)
        # Abort once the end of the file is reached
        if block_type is None:
            break
        elif block_type != BLOCK_TYPE_MATRIX:
            raise BinnfException("Unexpected Binnf block!")

        # Make sure all mandatory matrix fields are present
        name, _, matrix = res
        validate_matrix(name, matrix)

        # Read the data matrices
        if name == "populations":
            if "populations" in network:
                raise BinnfException(
                    "Only a single \"populations\" instance is supported")
            network["populations"] = matrix
        elif name == "list_connection_header":
            if "list_connection_header" in network:
                raise BinnfException(
                    "Only a single \"list_connection_header\" instance is supported")
            network["list_connection_header"] = matrix
        elif name == "list_connection":
            network["list_connections"].append(matrix)
        elif name == "group_connections":
            if "group_connections" in network:
                raise BinnfException(
                    "Only a single \"group_connections\" instance is supported")
            network["group_connections"] = matrix
        elif name == "parameters":
            network["parameters"].append(matrix)
        elif name == "signals":
            network["signals"].append(matrix)
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
HEADER_TARGET = [{"name": "pid",
                  "type": TYPE_INT32}, {"name": "nid",
                                        "type": TYPE_INT32}]
HEADER_TARGET_DTYPE = header_to_dtype(HEADER_TARGET)

HEADER_RUNTIMES = [{"name": "total",
                    "type": TYPE_FLOAT64}, {"name": "sim",
                                            "type": TYPE_FLOAT64},
                   {"name": "initialize",
                    "type": TYPE_FLOAT64}, {"name": "finalize",
                                            "type": TYPE_FLOAT64}]
HEADER_RUNTIMES_DTYPE = header_to_dtype(HEADER_RUNTIMES)


def write_result(fd, res):
    """
    Serialises the simulation result to binnf.

    :param fd: target file descriptor.
    :param res: simulation result.
    """
    for pid in xrange(len(res)):
        for signal in res[pid]:
            for nid in xrange(len(res[pid][signal])):
                serialise_matrix(
                    fd,
                    "target",
                    np.array(
                        [(pid, nid)], dtype=HEADER_TARGET_DTYPE))
                matrix = res[pid][signal][nid]
                if signal == "spikes":
                    serialise_matrix(fd, "spike_times", matrix)
                else:
                    serialise_matrix(fd, "trace_" + signal, matrix)


def write_runtimes(fd, times):
    """
    Serialises the simulation runtimes to binnf.

    :param fd: target file descriptor.
    :param times: object containing "total", "sim", "initialize" and "finalize"
    keys with the runtimes in seconds.
    """
    serialise_matrix(
        fd,
        "runtimes",
        np.array(
            [(times["total"], times["sim"], times["initialize"],
              times["finalize"])],
            dtype=HEADER_RUNTIMES_DTYPE))

# Export definitions

__all__ = ["serialise_matrix", "serialise_log", "deserialise", "read_network",
           "BinnfException"]
