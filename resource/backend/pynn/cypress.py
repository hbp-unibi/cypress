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
Contains the backend code responsible for mapping the current experiment setup
to a PyNN 0.7 or 0.8 simulation. Introduces yet another abstraction layer to
allow the description of a network independent of the actual PyNN version.
"""

# PyNN libraries
import pyNN
import pyNN.common
import numpy as np

if __name__ != "__main__":
    from constants import *

class CypressException(Exception):
    """
    Exception type used within Cypress.
    """
    pass


class CypressImportException(Exception):
    """
    Exception type used to signify that importing the simulator library failed.
    """
    pass


class Cypress:
    """
    This class is used as an abstraction to the actual PyNN backend, which may
    either be present in version 0.6, 0.7 or 0.8. Furthermore, it constructs
    the network from a descriptor read from binnf.
    """

    # List of simulators that need a call to "end" before the results are
    # retrieved
    PREMATURE_END_SIMULATORS = ["nmpm1"]

    # List of simulators which are hardware systems without an explicit
    # timestep
    ANALOGUE_SYSTEMS = ["ess", "nmpm1", "spikey"]

    #
    # Private methods
    #

    @staticmethod
    def _iterate_blocks(lst, eqn_fun, cbck_fun):
        count = len(lst)
        begin = 0
        begin_elem = None
        for i in xrange(count + 1):
            elem = eqn_fun(lst[i]) if i < count else None
            if i == count or elem != begin_elem:
                if not begin_elem is None:
                    cbck_fun(begin_elem, begin, i)
                begin_elem = elem
                begin = i

    @staticmethod
    def _check_version(version=pyNN.__version__):
        """
        Internally used to check the current PyNN version. Sets the "version"
        variable to the correct API version and raises an exception if the PyNN
        version is not supported.

        :param version: the PyNN version string to be checked, should be the
        value of pyNN.__version__
        """
        if (version[0:3] == '0.6'):
            return 6
        elif (version[0:3] == '0.7'):
            return 7
        elif (version[0:3] == '0.8'):
            return 8
        raise CypressException("Unsupported PyNN version '"
                               + version + "', supported are PyNN 0.6, 0.7 and 0.8")

    @classmethod
    def _load_simulator(cls, library):
        """
        Internally used to load the simulator with the specified name. Raises
        an CypressException if the specified simulator cannot be loaded.

        :param libarary: simulator library name as passed to the constructor.
        """

        # Try to load the simulator module
        try:
            import importlib
            return importlib.import_module(library)
        except ImportError:
            raise CypressException(
                "Could not find simulator library " + library)

    @staticmethod
    def _eval_setup(setup, sim, simulator, version):
        """
        Passes dictionary entries starting with "$" through the Python "eval"
        function. A "\\$" at the beginning of the line is replaced with "$"
        without evaluation, a "\\\\" at the beginning of the line is replaced
        with "\\".

        :param setup: setup dictionary to which the evaluation should be
        applied.
        :param sim: simulation object that should be made available to the
        evaluated code.
        :param simulator: simulator name that should be made available to the
        evaluated code.
        :param version: PyNN version number that should be made available to the
        evaluated code.
        """
        res = {}
        for key, value in setup.items():
            if (isinstance(value, str)):
                if (len(value) >= 1 and value[0] == "$"):
                    value = eval(value[1:])
                elif (len(value) >= 2 and value[0:2] == "\\$"):
                    value = "$" + value[2:]
                elif (len(value) >= 2 and value[0:2] == "\\\\"):
                    value = "\\" + value[2:]
            res[key] = value
        return res

    @staticmethod
    def _setup_nmpm1(setup, sim, simulator):
        """
        Performs additional setup necessary for NMPM1. Creates a new Marocco
        (MApping ROuting Calibration and COnfiguration for HICANN Wafers)
        instance and sets it up. Marocco setup parameters were taken from
        https://github.com/electronicvisions/hbp_platform_demo/blob/master/nmpm1/run.py
        """
        import pylogging
        from pymarocco import PyMarocco, Placement
        from pyhalbe.Coordinate import HICANNGlobal, Enum

        # Deactivate logging
        for domain in ["Default", "marocco", "sthal.HICANNConfigurator.Time"]:
            pylogging.set_loglevel(
                pylogging.get(domain), pylogging.LogLevel.ERROR)

        # Copy and delete non-standard setup parameters
        neuron_size = setup["neuron_size"]
        hicann_number = setup["hicann"]
        del setup["neuron_size"]
        del setup["hicann"]

        marocco = PyMarocco()
        marocco.placement.setDefaultNeuronSize(neuron_size)
        marocco.placement.use_output_buffer7_for_dnc_input_and_bg_hack = True
        marocco.placement.minSPL1 = False
        if simulator == "ess":
            marocco.backend = PyMarocco.ESS
            marocco.calib_backend = PyMarocco.Default
        else:
            marocco.backend = PyMarocco.Hardware
            marocco.calib_backend = PyMarocco.XML
            marocco.calib_path = "/wang/data/calibration/wafer_0"

        hicann = HICANNGlobal(Enum(hicann_number))

        # Pass the marocco object and the actual setup to the simulation setup
        # method
        sim.setup(marocco=marocco, **setup)

        # Return the marocco object and a list containing all HICANN
        return {
            "marocco": marocco,
            "hicann": hicann,
            "neuron_size": neuron_size
        }


    def _setup_simulator(self, setup, sim, simulator, version):
        """
        Internally used to setup the simulator with the given setup parameters.

        :param setup: setup dictionary to be passed to the simulator setup.
        """

        # Assemble the setup
        setup = self._eval_setup(setup, sim, simulator, version)

        # PyNN 0.7 compatibility hack: Update min_delay/timestep if only one
        # of the values is set
        if ((not "min_delay" in setup) and ("timestep" in setup)):
            setup["min_delay"] = setup["timestep"]
        if (("min_delay" in setup) and (not "timestep" in setup)):
            setup["timestep"] = setup["min_delay"]

        # PyNN 0.7 compatibility hack: Force certain parameters to be floating
        # point values (fixes "1" being passed as "timestep" instead of 1.0)
        for key in ["timestep", "min_delay", "max_delay"]:
            if (key in setup):
                setup[key] = float(setup[key])

        # Try to setup the simulator, do not output the clutter from the
        # simulators
        if (simulator == "nmpm1" or simulator == "ess"):
            self.backend_data = self._setup_nmpm1(setup, sim, simulator)
            self.repeat_projections = self.backend_data["neuron_size"]
        else:
            sim.setup(**setup)
        return setup

    def _build_population(self, count, type_name, record):
        """
        Used internally to creates a PyNN neuron population according to the
        given parameters.

        :param count: is the size of the population
        :param type_name: is the name of the neuron type that should be used.
        :param record: is an array with signals that should be recorded.
        """

        # Translate the neuron type to the PyNN neuron type
        if (not hasattr(self.sim, type_name)):
            raise CypressException("Neuron type '" + type_name
                                   + "' not supported by backend.")
        type_ = getattr(self.sim, type_name)
        is_source = type_name == TYPE_SOURCE

        # Create the population
        params = {}
        if is_source and self.simulator == "nmmc1":
            params = {"spike_times": []} # sPyNNaker issue #190
        res = self.sim.Population(count, type_, params)

        # Increment the neuron counter needed to work around a bug in spikey,
        # store the neuron index in the created population
        if not is_source:
            if self.version <= 6:
                setattr(res, "__offs", self.neuron_count)
            self.neuron_count += count

        # Setup recording
        if self.version <= 7:
            # Setup recording
            if (SIG_SPIKES in record):
                res.record()
            if (SIG_V in record):
                # Special handling for voltage recording with Spikey
                if (self.simulator == "spikey"):
                    if self.record_v_count == 0:
                        setattr(res, "__spikey_record_v", True)
                        self.sim.record_v(res[0], '')
                else:
                    res.record_v()

                # Increment the record_v_count variable
                self.record_v_count += count
            if ((SIG_GE in record) or (SIG_GI in record)):
                res.record_gsyn()
        elif (self.version == 8):
            # Setup recording
            res.record(record)

        # For NMPM1: register the population in the marocco instance
        if self.simulator == "nmpm1" and not is_source:
            self.backend_data["marocco"].placement.add(res,
                                                       self.backend_data["hicann"])

        return res

    def _build_populations(self, populations):
        # Fetch all keys in the structured array starting with "record_"
        record_keys = filter(lambda x: x.startswith("record_"),
                             populations.dtype.fields.keys())

        # Create a population descriptor for each row in the array
        res = [None] * len(populations)
        for i in xrange(len(populations)):
            # Fetch the name of the record keys (excluding the record_) for
            # which a "1" is set in the corresponding structured array
            # entry
            count = populations[i]["count"]
            type_name = TYPES[populations[i]["type"]]
            record = map(lambda x: x[1][7:], filter(lambda x: x[0] == 1,
                                                    zip(map(lambda key: populations[i][key], record_keys),
                                                        record_keys)))
            res[i] = {
                "count": count,
                "type_name": type_name,
                "record": record,
                "obj": self._build_population(int(count), type_name, record)}
        return res

    def _set_population_parameters(self, populations, network):
        """
        Updates the individual population parameters according to the data
        matrices read from the binnf.
        """
        ALL_NEURONS = 0x7FFFFFFF

        def iterate_pid_blocks_callback(lst, pid, begin, end, fun):
            """
            Helper function for iterate_pid_blocks. Ensures that each pid block
            contains values for every single neuron in the referenced
            population.
            """
            pop = populations[pid]["obj"]
            count = end - begin
            if count != pop.size:
                if count != 1 or lst[begin]["nid"] != ALL_NEURONS:
                    raise CypressException(
                        "Parameter count for one population must "
                        + "equal the number of neurons in the "
                        + "population")
            if lst[begin]["nid"] == ALL_NEURONS:
                if count != 1:
                    raise CypressException(
                        "Only one parameter set may be given per "
                        + "population if all neuron parameters "
                        + "should be set.")
            else:
                for j in xrange(count):
                    if lst[begin + j]["nid"] != j:
                        raise CypressException(
                            "Neuron indices must start at zero "
                            + "and be sorted.")
            fun(pop, begin, end)

        def iterate_pid_blocks(lst, fun):
            """
            Helper function which iterates over a list of elements containing
            "pid" and "nid" dictionary entries. Calls the callback function
            "fun" for a range of elements for which "pid" is equal. Makes sure
            the callback function is only called when the length of the range
            corresponds to the number of neurons in the pid-th population.
            """
            self._iterate_blocks(
                lst,
                lambda x: x["pid"],
                lambda pid,
                begin,
                end: iterate_pid_blocks_callback(
                    lst,
                    pid,
                    begin,
                    end,
                    fun))

        # Iterate over all parameter matrices
        for ps in network["parameters"]:
            keys = filter(lambda x: x != "nid" and x != "pid",
                          ps.dtype.fields.keys())

            def block_set_params(pop, begin, end):
                if ps[begin]["nid"] == ALL_NEURONS:
                    for key in keys:
                        if self.version <= 7:
                            pop.set({key: float(ps[begin][key])})
                        else:
                            pop.set(**{key: ps[begin][key]})
                else:
                    for key in keys:
                        pop.tset(key, ps[begin:end][key].tolist())
            iterate_pid_blocks(ps, block_set_params)

        # Iterate over the spike time matrices, fix the spike times
        ts = network["spike_times"]

        def block_set_times(pop, begin, end):
            values = map(lambda x: x["times"]["times"].tolist(), ts[begin:end])
            if ts[begin]["nid"] == ALL_NEURONS:
                values = values * pop.size
            pop.tset("spike_times", values)
        iterate_pid_blocks(ts, block_set_times)

    def _connect(self, populations, connections):
        """
        Performs the actual connections between neurons. First transforms the
        given connections list into a sorted connection matrix, then tries to
        batch-execute connections between neurons in the same source and target
        population.

        :param populations: is the list of already created population objects.
        :param connections: is the list containing the connections.
        """

        # Get a view of the matrix containing only the listed columns
        cs = connections
        csv = np.ndarray(cs.shape,
                         {name: cs.dtype.fields[name] for name in
                          ["nid_src", "nid_tar", "weight", "delay"]
                          }, cs, 0, cs.strides)

        # Build the actual connections, iterate over blocks which share pid_src
        # and pid_tar
        def create_projection(elem, begin, end):
            # Separate the synaptic connections into inhibitory and excitatory
            # synapses if not on NEST -- use the absolute weight value
            separate_synapses = self.simulator != "nest" # This is a bug in NEST
            if separate_synapses:
                csv[begin:end]["weight"] = np.abs(csv[begin:end]["weight"])

            # Create the connector
            connector = self.sim.FromListConnector(csv[begin:end].tolist())

            # Fetch source and target population objects
            source = populations[elem[0]]["obj"]
            target = populations[elem[1]]["obj"]

            if not separate_synapses:
                for _ in xrange(self.repeat_projections):
                    self.sim.Projection(source, target, connector)
            else:
                is_excitatory = elem[2]
                mechanism = "excitatory" if is_excitatory else "inhibitory"
                for _ in xrange(self.repeat_projections):
                    self.sim.Projection(source, target, connector,
                        target=mechanism)

        self._iterate_blocks(cs, lambda x: (x[0], x[1], x[4] >= 0.0), create_projection)

    @staticmethod
    def _convert_pyNN7_spikes(spikes, n, idx_offs=0, t_scale=1.0):
        """
        Converts a pyNN7 spike train, list of (nid, time)-tuples, into a list
        of lists containing the spike times for each neuron individually.
        """

        # Create one result list for each neuron
        res = [[] for _ in xrange(n)]
        for row in spikes:
            nIdx = int(row[0]) - idx_offs
            if nIdx >= 0 and nIdx < n:
                res[nIdx].append(float(row[1]) * t_scale)
            elif row[0] >= 0 and row[0] < n:
                # In case the Spikey indexing bug gets fixed, this code should
                # execute instead of the above.
                res[int(row[0])].append(float(row[1]) * t_scale)

        # Make sure the resulting lists are sorted by time
        for i in xrange(n):
            res[i] = np.array(res[i], dtype=np.float32)
            res[i].sort()
        return res

    @staticmethod
    def _convert_pyNN8_spikes(spikes):
        """
        Converts a pyNN8 spike train (some custom datastructure), into a list
        of arrays containing the spike times for each neuron individually.
        """
        return [np.array(spikes[i], dtype=np.float32)
                for i in xrange(len(spikes))]

    @staticmethod
    def _convert_pyNN7_signal(data, idx, n):
        """
        Converts a pyNN7 data array, list of (nid, time, d1, ..., dN)-tuples
        into a matrix containing the time stamps and the data for each neuron.
        """

        # Create a list containing all timepoints and a mapping from time to
        # index
        return [np.array(map(lambda row: [row[1], row[idx]], filter(
            lambda row: row[0] == i, data)), dtype=np.float32) for i in xrange(n)]

    def _fetch_spikey_voltage(self, population):
        """
        Hack which returns the voltages recorded by spikey.
        """
        vs = self.sim.membraneOutput
        ts = self.sim.timeMembraneOutput
        return [np.array(np.vstack((ts, vs)).transpose(), dtype=np.float32)]

    def _fetch_spikes(self, population):
        """
        Fetches the recorded spikes from a neuron population and performs all
        necessary data structure conversions for the PyNN versions.

        :param population: reference at a PyNN population object from which the
        spikes should be obtained.
        """
        if (self.version <= 7):
            # Workaround for spikey, which seems to index the neurons globally
            # instead of per-neuron
            idx_offs = (getattr(population, "__offs")
                        if hasattr(population, "__offs") else 0)
            if self.simulator == "nmpm1":
                return self._convert_pyNN7_spikes(population.getSpikes(),
                                                  population.size, idx_offs=idx_offs + 1, t_scale=1000.0)
            else:
                return self._convert_pyNN7_spikes(population.getSpikes(),
                                                  population.size, idx_offs=idx_offs)
        elif (self.version == 8):
            return self._convert_pyNN8_spikes(
                population.get_data().segments[0].spiketrains)
        return []

    def _fetch_signal(self, population, signal):
        """
        Converts an analog signal recorded by PyNN 0.7 or 0.8 to a common
        format with a data matrix containing the values for all neurons in the
        population and an independent timescale.

        :param population: reference at a PyNN population object from which the
        spikes should be obtained.
        :param signal: name of the signal that should be returned.
        """
        if (self.simulator == "nmpm1"):
            return []
        if (self.version <= 7):
            if (signal == SIG_V):
                # Special handling for the spikey simulator
                if (self.simulator == "spikey"):
                    if (hasattr(population, "__spikey_record_v")):
                        return self._fetch_spikey_voltage(population)
                else:
                    return self._convert_pyNN7_signal(population.get_v(), 2,
                                                      population.size)
            elif (signal == SIG_GE):
                return self._convert_pyNN7_signal(population.get_gsyn(), 2,
                                                  population.size)
            elif (signal == SIG_GI):
                # Workaround in bug #124 in sPyNNaker, see
                # https://github.com/SpiNNakerManchester/sPyNNaker/issues/124
                if (self.simulator != "nmmc1"):
                    return self._convert_pyNN7_signal(population.get_gsyn(), 3,
                                                      population.size)
        elif (self.version == 8):
            for data in population.get_data().segments[0].analogsignalarrays:
                if (data.name == signal):
                    return [np.array(np.vstack((data.times, data.transpose()[0])).transpose(
                    ), dtype=np.float32) for i in xrange(population.size)]
        return []

    @staticmethod
    def _get_default_time_step():
        """
        Returns the value of the "DEFAULT_TIME_STEP" attribute, which is stored
        in different places for multiple PyNN versions.
        """
        if hasattr(pyNN.common, "DEFAULT_TIME_STEP"):
            return pyNN.common.DEFAULT_TIME_STEP
        elif (hasattr(pyNN.common, "control")
                and hasattr(pyNN.common.control, "DEFAULT_TIME_STEP")):
            return pyNN.common.control.DEFAULT_TIME_STEP
        return 0.1  # Above values are not defined in PyNN 0.6

    #
    # Public interface
    #

    # Actual simulator instance, loaded by the "load" method.
    sim = None

    # Name of the simulator
    simulator = ""

    # Copy of the setup parameters
    setup = {}

    # Additional objects required by other backends
    backend_data = {}

    # Currently loaded pyNN version (as an integer, either 7 or 8 for 0.7 and
    # 0.8 respectively)
    version = 0

    # Number of times the connections must be repeated -- required for NMPM1
    repeat_projections = 1

    # Number of times the output potential has been recorded -- some platforms
    # only support a limited number of voltage recordings. Only valid for
    # PyNN 0.7 or lower.
    record_v_count = 0

    # Global count of non-source neurons within all populations
    neuron_count = 0

    def __init__(self, simulator, library, setup={}):
        """
        Tries to load the PyNN simulator with the given name. Throws an
        exception if the simulator could not be found or no compatible PyNN
        version is loaded. Calls the "setup" method with the given simulator
        setup.

        :param simulator: canonical name of the PyNN backend that should be
        used. Should be one of ["nest", "spinnaker", "hicann", "spikey"].
        :param library: the library that should be imported for code execution.
        :param setup: structure containing parameters to be passed to the
        "setup" method of the simulator
        """

        self.simulator = simulator
        self.version = self._check_version()
        self.sim = self._load_simulator(library)
        self.setup = self._setup_simulator(setup, self.sim, self.simulator,
                                           self.version)

    def get_time_step(self):
        # Fetch the simulation timestep, work around bugs #123 and #147 in
        # sPyNNaker.
        timestep = self._get_default_time_step()
        if (hasattr(self.sim, "get_time_step") and not (
                (self.simulator in self.ANALOGUE_SYSTEMS) or
                (self.simulator == "nmmc1"))):
            timestep = self.sim.get_time_step()
        elif ("timestep" in self.setup):
            timestep = self.setup["timestep"]
        return timestep

    def run(self, network, duration=0):
        """
        Builds and runs the network described in the "network" structure.

        :param network: Dictionary with two entries: "populations" and
        "connections", where the first introduces the individual neuron
        populations and their parameters and the latter is an adjacency list
        containing the connection weights and delays between neurons.
        :param duration: Simulation duration. If smaller than or equal to zero,
        the simulation duration is automatically determined depending on the
        last input spike time.
        :return: the recorded signals for each population, signal type and
        neuron
        """

        # Reset some state variables
        self.record_v_count = 0
        self.neuron_count = 0

        # Fetch the timestep
        timestep = self.get_time_step()

        # Generate the neuron populations and fetch the canonicalized population
        # descriptors
        populations = self._build_populations(network["populations"])
        self._set_population_parameters(populations, network)
        self._connect(populations, network["connections"])

        # Round up the duration to the timestep -- fixes a problem with
        # SpiNNaker
        duration = int((duration + timestep) / timestep) * timestep

        # Run the simulation
        self.sim.run(duration)

        # End the simulation to fetch the results on nmpm1
        if (self.simulator in self.PREMATURE_END_SIMULATORS):
            self.sim.end()

        # Gather the recorded data and store it in the result structure
        res = [{} for _ in xrange(len(populations))]
        for i in xrange(len(populations)):
            for signal in populations[i]["record"]:
                if (signal == SIG_SPIKES):
                    res[i][signal] = self._fetch_spikes(populations[i]["obj"])
                else:
                    res[i][signal] = self._fetch_signal(
                        populations[i]["obj"], signal)

        # End the simulation if this has not been done yet
        if (not (self.simulator in self.PREMATURE_END_SIMULATORS)):
            self.sim.end()

        return res

