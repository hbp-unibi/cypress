Cypress Python Code
===================

This folder contains the Python code which interfaces with PyNN on the one hand,
and the C++ portion of cypress on the other hand. Note that the cypress Python
code is largely derived from PyNNLess, yet in contrast to PyNNLess a lot of
canonicalisation was removed, as the C++ code is supposed to generate canonical
input data.

Communication between the C++ code and PyNN is in the binnf format via standard
in and out. To this end, the C++ code executes the "cli.py" with the correct
arguments and streams the network data to stdin, while waiting for the result
on stdout of the child process.
