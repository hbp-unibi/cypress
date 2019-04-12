Cypress
=======

![Synfire Chain Example](https://raw.githubusercontent.com/hbp-sanncs/cypress/master/docs/synfire_result_spikey.png)
*Synfire chain example running on the Spikey neuromorphic hardware system*

Cypress is a C++ Spiking Neural Network Simulation Framework. It allows to describe
and execute spiking neural networks on various simulation platforms directly in
C++ code. It provides a wrapper around PyNN, which allows to directly run networks
on the Human Brain Project (HBP) neuromorphic hardware systems. Cypress also allows
to transparently execute the networks remotely on the HBP Neuromorphic Compute Platform,
significantly shortening the time required when performing experiments.

[![Build Status](https://travis-ci.org/hbp-unibi/cypress.svg?branch=master)](https://travis-ci.org/hbp-unibi/cypress) [![Documentation](https://img.shields.io/badge/docs-doxygen-blue.svg)](https://hbp-unibi.github.io/cypress/index.html)

Installation
------------

Cypress requires a C++14 compliant compiler such as GCC 4.9 and CMake in version 3.0 (or later). In order to run Cypress applications on the Neuromorphic Compute Platform, they must be statically linked. To this end you should install the `glibc-static` and `libstdc++-static` libraries provided by your distribution. On Fedora you can install these using
```bash
sudo dnf install glibc-static libstdc++-static
```

Furthermore Python in version 2.7 and `pip` must be installed, as well as the PyPi packages `pyNN`, `requests` and `pyminifier`. You can install the latter using
```bash
sudo pip install pyNN requests pyminifier
```

In order to run network simulations you also need to install NEST or PyNN with an appropriate simulator backend (for example sPyNNaker). See http://www.nest-simulator.org/ for information on how to install NEST.

Once the above requirements are fulfilled, simply run
```bash
git clone https://github.com/hbp-sanncs/cypress
mkdir cypress/build && cd cypress/build
cmake ..
make && make test
sudo make install
```

Example
-------

```c++
#include <cypress/cypress.hpp>

using namespace cypress;

int main(int argc, const char *argv[])
{
    // Print some usage information
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <SIMULATOR>" << std::endl;
        return 1;
    }

    // Create the network description and run it
    Network net = Network()
        .add_population<SpikeSourceArray>("source", 1, {100.0, 200.0, 300.0})
        .add_population<IfCondExp>("neuron", 4, {}, {"spikes"})
        .add_connection("source", "neuron", Connector::all_to_all(0.16))
        .run(argv[1]);

    // Print the results
    for (auto neuron: net.population<IfCondExp>("neuron")) {
        std::cout << "Spike times for neuron " << neuron.nid() << std::endl;
        std::cout << neuron.signals().get_spikes();
    }
    return 0;
}
```

You can compile this code using the following command line:
```bash
g++ -std=c++14 cypress_test.cpp -o cypress_test -lcypress -lpthread
```
Then run it with the native NEST backing using
```bash
./cypress_test nest
```
Other backends include `pynn.X` where `X` is the name of a PyNN backend, or
`nmpi.X` or `nmpi.pynn.X` where `X` is the name of a PyNN backend that should
be executed on the NMPI platform. See below for details.

Features
--------

### Consistent API

The API of Cypress is extremely consistent: network description objects exhibit a common
set of methods for setting parameters and performing connections. The following constructs
are all valid:

```c++
Population<IfCondExp> pop(net, 10, IfCondExpParameters().v_rest(-65.0).v_thresh(-40.0));
PopulationView<IfCondExp> view = pop(3, 5);
Neuron<IfCondExp> n1 = view[0]; // Neuron three
Neuron<IfCondExp> n2 = pop[1];

view.connect_to(pop[6], Connector::all_to_all(0.1, 0.1)); // weight, delay
n2.connect_to(view, Connector::all_to_all(0.1, 0.1));
```

### Execute on various simulators

Cypress utilises PyNN to provide a multitude of simulation platforms, including the
digital manycore system SpiNNaker (early codename NM-MC1) developed at the University of Manchester and the
analogue physical model system BrainScaleS (early codename NM-PM1) developed at the Kirchhoff Institute for Physics
at Heidelberg University.

### Harness the power of C++

Especially when procedurally constructing large neural networks, Python might be
a major bottleneck. However, constructing networks in C++ is extremely fast. Furthermore,
this amazing alien C++ technology-thingy allows to detect most errors in your code before actually
executing it. Who would have thought of that?

### Supported Backends

Currently we support the following backends:
 * `pynn.nest` is using the NESt simulator via PyNN (see http://www.nest-simulator.org/ for installation instructions) 
 * `nest` **experimental** implementation of NEST SLI 
 * `spinnaker` uses the digital SpiNNaker architecture (see https://spinnakermanchester.github.io/)
 * `nmpm1` is using the BrainScaleS system (https://brainscales.kip.uni-heidelberg.de/) and its executable system specification `ESS`
 * `spikey`, the predecessor of BrainScaleS (https://www.kip.uni-heidelberg.de/vision/research/spikey/)

Futhermore, backends available through the Neuromorphic Platform Service nmpi can be used via `nmpi.backend`. If you have access to the Heidelberg server environment, jobs can be executed from the entry server or one of the computation nodes by using `slurm.x`, with which resources are automatically allocated. 

Backend specific performance and setup knobs
------------
There are several optimization knobs you can use to tune the simulation. These can be used by amending the backend string, e.g. ```spinnaker='{"option" : value, "option2" : value2, ...}'```. This is used for backend specific options expected from ```pynn.setup(...)```, but also for some additional option which are listed here.

### BrainScaleS and ESS
 * `neuron_size` Number of neuron circuits combined to one neuron. Possible values: 2,4,6,...; Default: 4
 * `big_capacitor`  Using big capacitors on HICANNs. Possible values: true, false; Default: false
 * `bandwidth`  Consider the firing rate of input neurons and only use the partial maximal bandwidth. The smaller the value, the less neurons are mapped to a single HICANN. Possible values: [0,1]; Default: None
 * `calib_path` Path to calibration files. Possible values: "path/to/calib" ; Default: Current default calib path

Only on the hardware:
 * `hicann` and `wafer`:  Choose the Wafer and the HICANN for your network emulation. Possible values: Available HICANNs and Wafers, also list of HICANNs; Default: 367 and 33
 * `digital_weight`:  Directly set the digital weight instead of PyNN weights. Currently not supports ListConnectors with varying weights. Possible values: true, false; Default: false

### SpiNNaker
 * `neurons_per_core` Sets the maximal number of neurons for the IaF neuron with conductance based synapses. Possible values: 1, ..., 255, ... ; Default: None (255?)

FAQ
------------

Q: Executing make && make test (or during simulations) the program aborts with a seg fault.  
A: Try building without static linking (comment line 52 in CMakeLists.txt) ```SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")```

Q: On SpiNNaker, there is an issue:```C++ exception with description "KeyError: "Unexpected key type: <type 'unicode'>" ```  
A: Manually patch ```lib/python2.7/site-packages/SpiNNUtilities-1!4.0.1-py2.7.egg/spinn_utilities/ranged/abstract_dict.py ```: replace ```str ``` by ```basestring ``` in line 222

## Authors

This project has been initiated by Andreas Stöckel in 2016 while working
at Bielefeld University in the [Cognitronics and Sensor Systems Group](http://www.ks.cit-ec.uni-bielefeld.de/) which is
part of the [Human Brain Project, SP 9](https://www.humanbrainproject.eu/neuromorphic-computing-platform) and has been continued by Christoph Jenzen.

## License

This project and all its files are licensed under the
[GPL version 3](http://www.gnu.org/licenses/gpl.txt) unless explicitly stated
differently.
