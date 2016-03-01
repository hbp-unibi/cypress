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
    // Print some usage information
    if (argc != 2 && !NMPI::check_args(argc, argv)) {
        std::cout << "Usage: " << argv[0] << " <SIMULATOR>" << std::endl;
        return 1;
    }

    // Create the network description and run it
    Network net = Network()
        .add_population<SpikeSourceArray>("source", 1, {100.0, 200.0, 300.0})
        .add_population<IfCondExp>("neuron", 4, {}, {"spikes"})
        .add_connection("source", "neuron", Connector::all_to_all(0.016))
        .run(NMPI(argv[1], argc, argv));

    // Print the results
    for (auto neuron: net.population<IfCondExp>("neuron")) {
        std::cout << "Spike times for neuron " << neuron.nid() << std::endl;
        std::cout << neuron.signals().get_spikes();
    }
    return 0;
}
```

This example creates a network consisting of one spike source array and a
population of four linear integrate and fire neurons with conductance based synapses,
uploads the program to the NMPI and prints the results on the standard output.

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
a major bottleneck. However, constructing networks in C++ is extremely fast. Furthermore,
this amazing alien C++ technology-thingy allows to detect most errors in your code before actually
executing it. Who would have thought of that?

## Authors

This project has been initiated by Andreas Stöckel in 2016 while working
at Bielefeld University in the [Cognitronics and Sensor Systems Group](http://www.ks.cit-ec.uni-bielefeld.de/) which is
part of the [Human Brain Project, SP 9](https://www.humanbrainproject.eu/neuromorphic-computing-platform).

## License

This project and all its files are licensed under the
[GPL version 3](http://www.gnu.org/licenses/gpl.txt) unless explicitly stated
differently.
