Cypress
=======

Cypress is a C++ Spiking Neural Network Simulation Framework. It allows to describe
and execute spiking neural networks on various simulation platforms directly in
C++ code. It provides a wrapper around PyNN, which allows to directly run networks
on the Human Brain Project (HBP) neuromorphic hardware systems. Cypress also allows
to transparently execute the networks remotely on the HBP Neuromorphic Compute Platform,
significantly shortening the time required when performing experiments.

Example
-------

```c++
#include <cypress/cypress.hpp>

using namespace cypress;

int main(int argc, const char *argv[])
{
    Network().population<SpikeSourceArray>("source", 1, {100.0, 200.0, 300.0})
        .population<IfCondExp>("neuron", 4)
        .connect("source", "neuron", Connector::all_to_all(0.016))
        .run(NMPI("nmmc1", argc, argv));
    return 0;
}
```

This one-line example creates a network consisting of one spike source array and a
population of four linear integrate and fire neurons with conductance based synapses
and uploads the program to the NMPI.

For more examples see ? or follow the tutorial on ?.

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
digital manycore system NM-MC1 developed at the University of Manchester and the
analogue physical model system NM-PM1 developed at the Kirchhoff Institute for Physics
at Heidelberg University.

### Harness the power of C++

Especially when procedurally constructing large neural networks, Python might be
a major bottleneck -- constructing networks in C++ is extremely fast. Furthermore,
the amazing alien C++ technology allows to detect most errors in your code before actually
executing it. Who would have thought of that?
