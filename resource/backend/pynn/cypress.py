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
import time

if __name__ != "__main__":
    from constants import *

import logging
logger = logging.getLogger("cypress")
logger.setLevel(logging.DEBUG)


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
    PREMATURE_END_SIMULATORS = ["nmpm1", "ess"]

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
                if begin_elem is not None:
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
        elif (version[0:3] == '0.9'):
            return 9
        raise CypressException("Unsupported PyNN version '" + version +
                               "', supported are PyNN 0.6 to 0.9")

    @staticmethod
    def _check_neo_version():
        """
        Internally used to check the current neo version. Neo is used with 
        PyNN>0.8
        """
        import neo as n
        version = n.__version__
        if (version[0:3] == '0.3'):
            return 3
        elif (version[0:3] == '0.4'):
            return 4
        elif (version[0:3] == '0.5'):
            return 5
        raise CypressException("Unsupported PyNN version '" + version +
                               "', supported are PyNN 0.6 to 0.9")

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
            raise CypressException("Could not find simulator library " +
                                   library)

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
        :param version: PyNN version number that should be made available to
        the evaluated code.
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
        from pymarocco import PyMarocco

        pylogging.reset()
        pylogging.log_to_cout(pylogging.LogLevel.FATAL)
        # Activate logging
        from pysthal.command_line_util import init_logger
        init_logger("INFO", [
            ("ESS", "INFO"),
            ("marocco", "INFO"),
            ("calibtic", "INFO"),
            ("sthal", "FATAL")
        ])
        # Log everything to file
        # pylogging.log_to_file("log_back.txt", pylogging.LogLevel.INFO)

        # In case one of the UHEI platforms is used, there will by the pylogging
        # module, which is a bridge from Python to log4cxx. Tell log4cxx to send
        # its log messages back to Python.
        if hasattr(pylogging, "append_to_logging"):
            pylogging.append_to_logging("PyNN")

        marocco = PyMarocco()

        # Copy and delete non-standard setup parameters

        # Setting of the virtual neuron size referring to the number of
        # circuits (DenMems) connected to represent a single neuron
        if "neuron_size" in setup:
            marocco.neuron_placement.default_neuron_size(setup["neuron_size"])
            del setup["neuron_size"]
        else:
            marocco.neuron_placement.default_neuron_size(4)
        marocco = PyMarocco()

        # Reserve a OutputBuffer fod DNC inputs and output
        marocco.neuron_placement.restrict_rightmost_neuron_blocks(True)
        # Restrict number of logical neurons/neuron block
        marocco.neuron_placement.minimize_number_of_sending_repeaters(True)

        # Temporary (?) fix for the usage of a special HICANN chip
        hicann = None
        runtime = None

        # Two possibilities for choosing the HW Capacitance. This is unrelated
        # to biological capacitance (everything will be scaled automatically),
        # but changes the range of hardware parameters, especially weights.
        if "big_capacitor" in setup:
            logger.info("Using big capacitors")
            marocco.param_trafo.use_big_capacitors = True
            del setup["big_capacitor"]
        else:
            marocco.param_trafo.use_big_capacitors = False
            logger.info("Using small capacitors")

        # In the mapping process, bandwidth of input can be considered.
        # consider_firing_rate will estimate the firing rate either of the
        # SpikeSourcePoission Rate or the average rate between first and last
        # spike of the SpikeSourceArray. In addition, mapping can be forced to
        # use only a percentage of the bandwidth: setting
        # bandwidth_utilization(par) with a value of par < 1.0 will reduce the
        # seemingly available bandwidth for mapping by par.
        if "bandwidth" in setup:
            logger.info("Marocco bandwidth_utilization is set to " +
                        str(setup["bandwidth"]))
            marocco.input_placement.consider_firing_rate(True)
            marocco.input_placement.bandwidth_utilization(
                float(setup["bandwidth"]))
            del setup["bandwidth"]

        if simulator == "ess":
            marocco.backend = PyMarocco.ESS
            marocco.experiment_time_offset = 5.e-7
            if "calib_path" in setup:
                # Use custom calibration in both mapping and ESS!
                logger.info("Using custom calibration at " +
                            str(setup["calib_path"]))
                marocco.calib_backend = PyMarocco.XML
                marocco.calib_path = str(setup["calib_path"])
                marocco.ess_config.calib_path = str(setup["calib_path"])
                del setup["calib_path"]
        else:
            import pyhalbe.Coordinate as C
            from pymarocco import Defects
            from pymarocco.runtime import Runtime

            marocco.merger_routing.strategy(marocco.merger_routing.one_to_one)

            marocco.bkg_gen_isi = 125
            marocco.pll_freq = 125e6

            marocco.backend = PyMarocco.Hardware
            marocco.calib_backend = PyMarocco.XML

            # Chose calibration files
            if "calib_path" in setup:
                marocco.defects.path = marocco.calib_path = str(
                    setup["calib_path"])
                logger.info("Using custom calibration at " +
                            str(setup["calib_path"]))
                del setup["calib_path"]
            else:
                marocco.defects.path = marocco.calib_path = "/wang/data/calibration/guidebook"
            marocco.defects.backend = Defects.XML

            # Chose Wafer and HICANN. Have to be in accordance with parameters
            # for srun
            if "wafer" in setup:
                marocco.default_wafer = C.Wafer(int(setup["wafer"]))
                logger.info("Using custom wafer at " + str(setup["wafer"]))
                del setup["wafer"]
            else:
                marocco.default_wafer = C.Wafer(33)
            if "hicann" in setup:
                if isinstance(setup["hicann"], (list, tuple)):
                    hicann = []
                    for hno in setup["hicann"]:
                        hicann.append(C.HICANNOnWafer(C.Enum(int(hno))))
                else:
                    hicann = C.HICANNOnWafer(C.Enum(int(setup["hicann"])))
                logger.info("Using custom HICANN number " +
                            str(setup["hicann"]) + "\n")
                del setup["hicann"]
            else:
                hicann = C.HICANNOnWafer(C.Enum(367))  # choose hicann

            runtime = Runtime(marocco.default_wafer)
            setup["marocco_runtime"] = runtime

            #marocco.hicann_configurator = PyMarocco.HICANNv4Configurator

        # Pass the marocco object and the actual setup to the simulation setup
        # method
        sim.setup(marocco=marocco, **setup)

        # Return used neuron size

        return {"marocco": marocco, "hicann": hicann, "runtime": runtime}

    @staticmethod
    def _setup_spikey(setup, sim):
        import pylogging
        for log in ["HAL.Cal", "HAL.PyS", "HAL.Spi", "PyN.cfg", "PyN.syn",
                    "PyN.wks", "Default"]:
            pylogging.set_loglevel(pylogging.get(log),
                                   pylogging.LogLevel.DEBUG)
        if hasattr(pylogging, "append_to_logging"):
            pylogging.append_to_logging("PyNN")
        sim.setup(**setup)

    def _setup_simulator(self, setup, sim, simulator, version):
        """
        Internally used to setup the simulator with the given setup parameters.

        :param setup: setup dictionary to be passed to the simulator setup.
        """

        # Assemble the setup
        setup = self._eval_setup(setup, sim, simulator, version)

        # PyNN 0.7 compatibility hack: Update min_delay/timestep if only one
        # of the values is set
        if (("min_delay" not in setup) and ("timestep" in setup)):
            setup["min_delay"] = setup["timestep"]
        if (("min_delay" in setup) and ("timestep" not in setup)):
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
        elif (simulator == "spikey"):
            self._setup_spikey(setup, sim)
        else:
            sim.setup(**setup)
        return setup

    def _build_population(self, count, type_name, signals, record):
        """
        Used internally to create a PyNN neuron population according to the
        given parameters.

        :param count: is the size of the population
        :param type_name: is the name of the neuron type that should be used.
        :param signals: is an array with signals + data that should be recorded.
        :param record: is an array with signal names that should be recorded.
        """

        # Translate the neuron type to the PyNN neuron type
        if (not hasattr(self.sim, type_name)):
            raise CypressException("Neuron type '" + type_name +
                                   "' not supported by backend.")
        type_ = getattr(self.sim, type_name)
        is_source = type_name == TYPE_SOURCE

        # Create the population
        params = {}
        if is_source and self.simulator == "nmmc1":
            params = {"spike_times": []}  # sPyNNaker issue #190
        res = self.sim.Population(count, type_, params)

        # Temporary (?) fix for the usage of a special HICANN chip
        if self.simulator == "nmpm1" and not (self.backend_data["hicann"] is None):
            self.backend_data["marocco"].manual_placement.on_hicann(
                res, self.backend_data["hicann"])

        # Increment the neuron counter needed to work around absolute neuron ids
        # in PyNN 0.6, store the neuron index in the created population
        if not is_source:
            if self.version <= 6:
                setattr(res, "__offs", self.neuron_count)
            self.neuron_count += count

        # Setup recording. Check if data is homogeneous
        if((signals.shape[0] != count) or (count == 1)):
            if self.version <= 7:
                if (SIG_SPIKES in record):
                    res.record()
                if (SIG_V in record):
                    # Special handling for voltage recording with Spikey
                    if (self.simulator == "spikey"):
                        if self.record_v_count == 0:
                            setattr(res, "__spikey_record_v", 0)
                            self.sim.record_v(res[0], '')
                            logger.warning("Spikey can only record membrane voltage" +
                                           " for one neuron! Neuron 0 is recorded!")
                        else:
                            logger.warning("Spikey can only record membrane voltage" +
                                           " for one neuron! Records will be ignored")
                    else:
                        res.record_v()
                    # Increment the record_v_count variable
                    self.record_v_count += count
                if ((SIG_GE in record) or (SIG_GI in record)):
                    res.record_gsyn()
            elif (self.version >= 8):
                res.record(record)
        else:
            for i in xrange(0, len(record)):
                # Create a list of neuron indices to be recorded
                list = []
                for j in xrange(0, len(signals["record_" + record[i]])):
                    if signals["record_" + record[i]][j]:
                        # Spikey's record funtion uses PyIDs instead of ints
                        if(self.simulator == "spikey"):
                            list.append(res[j])
                        else:
                            list.append(j)

                if self.version <= 6:
                    if (SIG_SPIKES in record[i]):
                        res.record(list)
                    if (SIG_V in record[i]):
                        # Special handling for voltage recording with Spikey
                        if (self.simulator == "spikey"):
                            if self.record_v_count == 0:
                                setattr(res, "__spikey_record_v", int(list[0]))
                                self.sim.record_v(list[0], '')
                                self.record_v_count = 1
                                if(len(list) > 1):
                                    logger.warning("Spikey can only record " +
                                                   "membrane voltage for " +
                                                   "one neuron! Record from " +
                                                   "neuron " + str(list[0]))
                            else:
                                logger.warning("Spikey can only record membrane" +
                                               "voltage for one neuron! " +
                                               "voltage rec will be ignored")
                        else:
                            res.record_v(list)
                        # Increment the record_v_count variable
                        self.record_v_count += len(list)
                    if ((SIG_GE in record[i]) or (SIG_GI in record[i])):
                        res.record_gsyn(list)

                elif (self.version == 7):
                    # Special case SpiNNaker
                    if((self.simulator == "spinnaker") or
                            (self.simulator == "nmmc1")):
                        logger.warn("Setting up recording for individual cells" +
                                    " is not supported by SpiNNaker!")
                        if (SIG_SPIKES in record[i]):
                            res.record()
                        if (SIG_V in record[i]):
                            res.record_v()
                        if ((SIG_GE in record[i]) or (SIG_GI in record[i])):
                            res.record_gsyn()

                    # Every other backend supports PopulationView
                    else:
                        # Set record via PopulationView
                        pop = self.sim.PopulationView(res, list)
                        if (SIG_SPIKES in record[i]):
                            pop.record()
                        if (SIG_V in record[i]):
                            pop.record_v()
                            self.record_v_count += len(list)
                        if ((SIG_GE in record[i]) or (SIG_GI in record[i])):
                            res.record_gsyn(list)

                elif (self.version >= 8):
                    if (self.simulator == "nmmc1"):
                        logger.warn("Setting up recording for individual cells" +
                                    " is not supported by SpiNNaker!")
                        res.record(record[i])
                    else:
                        pop = self.sim.PopulationView(res, list)
                        pop.record(record[i])
        return res

    def _gather_records(self, signals, count):
        """
        Used internally to gather all signal names which are actually recorded

        :param count: is the size of the population
        :param signals: is an array with signals + data that should be recorded.
        """

        # Fetch all keys in the structured array starting with "record_"
        record_keys = filter(lambda x: x.startswith("record_"),
                             signals.dtype.fields.keys())

        # Fetch the name of the record keys
        signals_ = zip(map(lambda key: signals[key], record_keys), record_keys)

        # If data is homogeneous
        if(signals.shape[0] != count):
            # throw away "record_" of the keys, gather entries which are set to
            # one
            record = map(lambda x: x[1][7:], filter(
                lambda x: x[0] == 1, signals_))
            return record
        else:
            record = []
            for i in xrange(0, len(signals_)):
                _record = False
                # Check wether signal is recorded for any neuron
                for j in xrange(0, len(signals_[i][0])):
                    if signals_[i][0][j] != 0:
                        _record = True
                        break
                if(_record):
                    record.append(signals_[i][1])
            # Throw away the "record_"
            record = map(lambda x: x[7:], record)
            return record

    def _build_populations(self, populations, signals):
        # Create a population descriptor for each row in the array
        res = [None] * len(populations)
        for i in xrange(len(populations)):
            count = populations[i]["count"]
            type_name = TYPES[populations[i]["type"]]
            record = self._gather_records(signals[i], count)
            res[i] = {
                "count": count,
                "type_name": type_name,
                "record": record,
                "obj": self._build_population(int(count), type_name, signals[i], record)
            }
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
                        "Parameter count for one population must " +
                        "equal the number of neurons in the " + "population")
            if lst[begin]["nid"] == ALL_NEURONS:
                if count != 1:
                    raise CypressException(
                        "Only one parameter set may be given per " +
                        "population if all neuron parameters " +
                        "should be set.")
            else:
                for j in xrange(count):
                    if lst[begin + j]["nid"] != j:
                        raise CypressException(
                            "Neuron indices must start at zero " +
                            "and be sorted.")
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
                lst, lambda x: x["pid"],
                lambda pid, begin, end: iterate_pid_blocks_callback(lst, pid, begin, end, fun))

        # Iterate over all parameter matrices
        for ps in network["parameters"]:
            keys = filter(lambda x: x != "nid" and x != "pid",
                          ps.dtype.fields.keys())
            has_v_rest = "v_rest" in keys

            def init_param(key, pop, values):
                if hasattr(pop, "initialize"):
                    try:
                        if self.version <= 7:
                            pop.initialize(key, values)
                        else:
                            pop.initialize(**{key: values})
                        return
                    except:
                        pass
                logger.warning(
                    "Backend does not support explicit initialization of the membrane potential!")

            def block_set_params(pop, begin, end):
                if ps[begin]["nid"] == ALL_NEURONS:
                    if has_v_rest:
                        init_param("v", pop, ps[begin]["v_rest"])
                    if self.simulator == "nmpm1" or self.simulator == "ess":
                        for key in keys:
                            if self.version <= 7:
                                pop.set({key: float(ps[begin][key])})
                            else:
                                pop.set(**{key: ps[begin][key]})
                    else:
                        values = dict(
                            map(lambda key: (key, ps[begin][key]), keys))
                        if self.version <= 7:
                            pop.set(values)
                        else:
                            pop.set(**values)
                else:
                    if has_v_rest:
                        init_param("v", pop, ps[begin:end]["v_rest"].tolist())
                    if self.version <= 7:
                        for key in keys:
                            pop.tset(key, ps[begin:end][key].tolist())
                    else:
                        for key in keys:
                            pop.set(key=ps[begin:end][key].tolist())

            iterate_pid_blocks(ps, block_set_params)

        # Iterate over the spike time matrices, fix the spike times
        ts = network["spike_times"]

        def block_set_times(pop, begin, end):
            values = map(lambda x: x["times"]["times"].tolist(), ts[begin:end])
            if ts[begin]["nid"] == ALL_NEURONS:
                values = values * pop.size
            if self.version <= 7:
                pop.tset("spike_times", values)
            else:
                pop.set(spike_times=values)

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
        csv = np.ndarray(
            cs.shape, {name: cs.dtype.fields[name]
                       for name in ["nid_src", "nid_tar", "weight", "delay"]},
            cs, 0, cs.strides)

        # Fix delays in nest having to be larger than and multiples of the
        # simulation timestep
        if self.simulator == "nest":
            dt = self.get_time_step()
            csv["delay"] = np.maximum(np.round(csv["delay"] / dt), 1.0) * dt

        # Build the actual connections, iterate over blocks which share pid_src
        # and pid_tar
        def create_projection(elem, begin, end):
            # Separate the synaptic connections into inhibitory and excitatory
            # synapses if not on NEST -- use the absolute weight value
            separate_synapses = self.simulator != "nest"  # This is a bug in NEST
            if separate_synapses:
                csv[begin:end]["weight"] = np.abs(csv[begin:end]["weight"])

            # Create the connector
            connector = self.sim.FromListConnector(csv[begin:end].tolist())

            # Fetch source and target population objects
            source = populations[elem[0]]["obj"]
            target = populations[elem[1]]["obj"]

            if not separate_synapses:
                self.sim.Projection(source, target, connector)
            else:
                is_excitatory = elem[2]
                mechanism = "excitatory" if is_excitatory else "inhibitory"
                self.sim.Projection(
                    source, target, connector, target=mechanism)

        self._iterate_blocks(cs, lambda x: (x[0], x[1], x[4] >= 0.0),
                             create_projection)

    def _connect_connectors(self, populations, connections):
        """
        Instead of using the from lsit connector, this function uses pre defined
        connectors from pyNN, for which the mapping process is (hoopefully) optimized.
        :param populations: is the list of already created population objects.
        :param connections: is the list containing the connection descriptions.
        """

        def check_full_pop(pop, start, end):
            # Check wether the full population is used
            return (start == 0) and (end == len(pop))

        def get_connector_new(conn_id, parameter, allow_self_conn=False):
            """
            returns a connector according to conn_id (which is identical to
            those in cypress c++ part.
            :param conn_id: int representing a connection type
            :param parameter: optional parameter for the conneciton
            :param allow_self_conn: Allows self connections of a neuron. Disabled
            by default (not valid on some backends)
            """
            if conn_id == 1:
                return self.sim.AllToAllConnector(
                    allow_self_connections=allow_self_conn)
            elif conn_id == 2:
                return self.sim.OneToOneConnector()
            elif conn_id == 3:
                return self.sim.FixedProbabilityConnector(p_connect=parameter,
                                                          allow_self_connections=allow_self_conn)
            elif conn_id == 4:
                return self.sim.FixedNumberPreConnector(n=int(parameter),
                                                        allow_self_connections=allow_self_conn)
            elif conn_id == 5:
                return self.sim.FixedNumberPostConnector(n=int(parameter),
                                                         allow_self_connections=allow_self_conn)
            else:
                raise CypressException("Unknown Connector ID " + conn_id)

        # Blame SpiNNaker for "not implemented error" on setWeight of a
        # projection. Therefore we have to set weights and delays with the
        # connector
        def get_connector_old(conn_id, weight, delay, parameter, allow_self_conn=False):
            """
            returns a connector according to conn_id (which is identical to
            those in cypress c++ part.
            :param conn_id: int representing a connection type
            :param weight: weight of the connection
            :param delay: synaptic delay in the connection
            :param parameter: optional parameter for the conneciton
            :param allow_self_conn: Allows self connections of a neuron. Disabled
            by default (not valid on some backends)
            """
            if conn_id == 1:
                return self.sim.AllToAllConnector(weights=weight, delays=delay,
                                                  allow_self_connections=allow_self_conn)
            elif conn_id == 2:
                return self.sim.OneToOneConnector(weights=weight, delays=delay)
            elif conn_id == 3:
                return self.sim.FixedProbabilityConnector(p_connect=parameter,
                                                          weights=weight,
                                                          delays=delay,
                                                          allow_self_connections=allow_self_conn)
            elif conn_id == 4:
                return self.sim.FixedNumberPreConnector(n=int(parameter),
                                                        weights=weight,
                                                        delays=delay,
                                                        allow_self_connections=allow_self_conn)
            elif conn_id == 5:
                return self.sim.FixedNumberPostConnector(n=int(parameter),
                                                         weights=weight,
                                                         delays=delay,
                                                         allow_self_connections=allow_self_conn)
            else:
                raise CypressException("Unknown Connector ID " + conn_id)

        def connect_pop(pop1, pop2, conn_id, weight, delay, parameter=0):
            """
            This function connects two populations with a PyNN connector.
            :param pop*: Population or PopulationView object
            :param conn_id: int representing a connection type
            :param weight: weight of the connection
            :param delay: synaptic delay in the connection
            :param parameter: optional parameter for the conneciton
            """
            mechanism = 'excitatory' if weight > 0 else 'inhibitory'
            if self.version < 8:
                # Weight*s*, delay*s* are part of the connector, use option
                # "target"
                proj = self.sim.Projection(pop1, pop2, get_connector_old(
                    conn_id, np.abs(weight), delay, parameter), target=mechanism)
            else:
                # Weight, delay is part of a synapse object
                if self.simulator == "nest":
                    # Fix delays in nest having to be larger than and multiples of the
                    # simulation timestep
                    dt = self.get_time_step()
                    delay = np.maximum(np.round(delay / dt), 1.0) * dt
                synapse = self.sim.StaticSynapse(weight=weight, delay=delay)
                self.sim.Projection(pop1, pop2, connector=get_connector_new(
                    conn_id, parameter), synapse_type=synapse)
                # Note: In PyNN 0.8 using both negative weights AND
                # receptor_type="inhibitory" will create an excitatory synapse.
                # Using receptor_type="excitatory" and negative weights will
                # create and inhibitory connection.

        def get_pop_view(pop, start, end):
            """
            Returns a PopulationView object containing neurons from start to end.
            Checks compatibility with backend
            :param pop: Population object
            :param start: integer value for the first neuron included
            :param end: integer value for the first neuron *not* included
            """
            if check_full_pop(pop, start, end):
                return pop
            elif self.simulator in ["nmmc1", "spikey"]:
                logger.warn("Backend does not not support partial view" +
                            " on populations! Connecting using the full Population!")
                return pop
            else:
                return pop[start:end]

        # Iterate through all connection descriptors
        for conn in connections:
            src = get_pop_view(populations[conn[0]]['obj'], conn[1], conn[2])
            tar = get_pop_view(populations[conn[3]]['obj'], conn[4], conn[5])
            connect_pop(src, tar, conn[6], conn[7], conn[8], conn[9])

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
            res[i].sort()
            res[i] = np.array(zip(res[i]), dtype=[("times", np.float64)])
        return res

    @staticmethod
    def _convert_pyNN8_spikes(spikes, n):
        """
        Converts a pyNN8 spike train (neo datastructure), into a list
        of arrays containing the spike times for each neuron individually.
        """
        res = [[] for _ in xrange(n)]
        for spike_train in spikes:
            res[spike_train.annotations["source_index"]] = spike_train
        for i in xrange(n):
            res[i].sort()
            res[i] = np.array(zip(res[i]), dtype=[("times", np.float64)])
        return res

    @staticmethod
    def _convert_pyNN7_signal(data, idx, n):
        """
        Converts a pyNN7 data array, list of (nid, time, d1, ..., dN)-tuples
        into a matrix containing the time stamps and the data for each neuron.
        """

        # Create a list containing all timepoints and a mapping from time to
        # index
        return [np.array(
            map(lambda row: (row[1], row[idx]), filter(lambda row: row[0] == i,
                                                       data)),
            dtype=[("times", np.float64), ("values", np.float64)]) for i in xrange(n)]

    @staticmethod
    def _convert_pyNN8_signal(data, size, neo_version):
        """
        Converts a pyNN8 analog value array (neo datastructure), into a list
        of arrays containing the signal for each neuron individually.
        """
        res = [[[], []] for _ in xrange(size)]

        if (neo_version < 5):
            neuron_ids = data.channel_indexes
        else:
            # Neo = 0.5
            neuron_ids = data.channel_index.channel_ids

        for i in xrange(0, len(neuron_ids)):
            res[neuron_ids[i]] = np.array((data.times, data.T[i]))

        for i in xrange(size):
            res[i] = np.array(zip(res[i][0], res[i][1]), dtype=[
                              ("times", np.float64), ("values", np.float64)])
        return res

    def _fetch_spikey_voltage(self, population):
        """
        Hack which returns the voltages recorded by spikey.
        """
        res = [[[], []] for _ in xrange(population.size)]
        vs = self.sim.membraneOutput
        ts = self.sim.timeMembraneOutput
        res[getattr(population, "__spikey_record_v")] = np.array((ts, vs))
        for i in xrange(population.size):
            res[i] = np.array(zip(res[i][0], res[i][1]), dtype=[
                              ("times", np.float64), ("values", np.float64)])
        return res

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
            return self._convert_pyNN7_spikes(
                population.getSpikes(), population.size, idx_offs=idx_offs)
        elif (self.version >= 8):
            return self._convert_pyNN8_spikes(population.get_data().segments[
                0].spiketrains, population.size)
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
        elif (self._check_neo_version() < 5):
            for data in population.get_data().segments[0].analogsignalarrays:
                if (data.name == signal):
                    return self._convert_pyNN8_signal(data, population.size, 3)
        elif (self._check_neo_version() == 5):
            for data in population.get_data().segments[0].analogsignals:
                if (data.name == signal):
                    return self._convert_pyNN8_signal(data, population.size, 5)
        return []

    @staticmethod
    def _get_default_time_step():
        """
        Returns the value of the "DEFAULT_TIME_STEP" attribute, which is stored
        in different places for multiple PyNN versions.
        """
        if hasattr(pyNN.common, "DEFAULT_TIME_STEP"):
            return pyNN.common.DEFAULT_TIME_STEP
        elif (hasattr(pyNN.common, "control") and
              hasattr(pyNN.common.control, "DEFAULT_TIME_STEP")):
            return pyNN.common.control.DEFAULT_TIME_STEP
        return 0.1  # Above values are not defined in PyNN 0.6

    @staticmethod
    def _nmpm1_set_sthal_params(wafer, gmax, gmax_div):
        """
        synaptic strength:
        gmax: 0 - 1023, strongest: 1023
        gmax_div: 1 - 15, strongest: 1
        """
        import pyhalbe.Coordinate as C
        from pyhalbe import HICANN

        # for all HICANNs in use
        for hicann in wafer.getAllocatedHicannCoordinates():

            fgs = wafer[hicann].floating_gates
            # set parameters influencing the synaptic strength
            for block in C.iter_all(C.FGBlockOnHICANN):
                fgs.setShared(block, HICANN.shared_parameter.V_gmax0, gmax)
                fgs.setShared(block, HICANN.shared_parameter.V_gmax1, gmax)
                fgs.setShared(block, HICANN.shared_parameter.V_gmax2, gmax)
                fgs.setShared(block, HICANN.shared_parameter.V_gmax3, gmax)

            for driver in C.iter_all(C.SynapseDriverOnHICANN):
                for row in C.iter_all(C.RowOnSynapseDriver):
                    wafer[hicann].synapses[driver][row].set_gmax_div(
                        C.left, gmax_div)
                    wafer[hicann].synapses[driver][row].set_gmax_div(
                        C.right, gmax_div)

            # don't change values below
            for ii in xrange(fgs.getNoProgrammingPasses()):
                cfg = fgs.getFGConfig(C.Enum(ii))
                cfg.fg_biasn = 0
                cfg.fg_bias = 0
                fgs.setFGConfig(C.Enum(ii), cfg)

            for block in C.iter_all(C.FGBlockOnHICANN):
                fgs.setShared(block, HICANN.shared_parameter.V_dllres, 275)
                fgs.setShared(block, HICANN.shared_parameter.V_ccas, 800)

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
        used. Should be one of ["nest", "nmmc1", "nmpm1", "ess", "spikey"].
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
        if (hasattr(self.sim, "get_time_step") and
                not ((self.simulator in self.ANALOGUE_SYSTEMS) or
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

        # First time measurement point
        t1 = time.time()

        # Reset some state variables
        self.record_v_count = 0
        self.neuron_count = 0

        # Fetch the timestep
        timestep = self.get_time_step()

        # Generate the neuron populations and fetch the canonicalized population
        # descriptors
        populations = self._build_populations(
            network["populations"], network["signals"])
        self._set_population_parameters(populations, network)
        self._connect(populations, network["list_connections"])
        self._connect_connectors(populations, network["group_connections"])

        # Round up the duration to the timestep -- fixes a problem with
        # SpiNNaker
        duration = int((duration + timestep) / timestep) * timestep
        
        if self.simulator == "nmpm1":
            from pymarocco import PyMarocco
            # Only mapping
            self.backend_data["marocco"].skip_mapping = False
            self.backend_data["marocco"].backend = PyMarocco.None
            self.sim.reset()
            self.sim.run(duration)
            
            # Set some low-level parameters (current workflow...)
            self._nmpm1_set_sthal_params(self.backend_data[
                             "runtime"].wafer(), gmax=1023, gmax_div=1)
            # Disable mapping for the execution
            self.backend_data["marocco"].skip_mapping = True
            self.backend_data["marocco"].backend = PyMarocco.Hardware
            self.backend_data["marocco"].hicann_configurator = PyMarocco.HICANNv4Configurator

        # Run the simulation
        t2 = time.time()
        self.sim.run(duration)
        t3 = time.time()

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
                    res[i][signal] = self._fetch_signal(populations[i]["obj"],
                                                        signal)

        # End the simulation if this has not been done yet
        if (not (self.simulator in self.PREMATURE_END_SIMULATORS)):
            self.sim.end()

        # Store the time measurements
        t4 = time.time()
        runtimes = {
            "total": t4 - t1,
            "sim": t3 - t2,
            "initialize": t2 - t1,
            "finalize": t4 - t3
        }
        return res, runtimes
