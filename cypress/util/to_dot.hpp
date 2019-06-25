#include <cypress/cypress.hpp>

namespace cypress {
namespace {
std::vector<std::string> group_conns = {
    "AllToAllConnector", "OneToOneConnector", "RandomConnector",
    "FixedFanInConnector", "FixedFanOutConnector"};
}
void create_dot(const NetworkBase &netw,
                const std::string graph_label = "Network Architecture",
                const std::string filename = "graph.dot",
                const bool call_dot = true)
{
	std::ofstream file(filename, std::ios_base::out);
	file << "digraph graphname \n{\n";
	file << "\t#___________________STYLE___________________\n";
	file << "graph [bgcolor=\"white\"]\n"
	     << "\tedge [ colorscheme=\"paired12\"]\n"
	     << "\tnode [ colorscheme=\"paired12\", "
	        "style=filled,width=2.0,height=1.0,fixedsize=true]\n";
	file << "\t label = \"" << graph_label << "\"\n";

	file << "\n\t#___________________NODES___________________\n";
	for (const auto &i : netw.populations()) {
		file << "\t" << i.pid();
		if (i[0].type().spike_source) {
			file << " [shape=invhouse,labelloc=\"t\",fillcolor=3";
		}
		else {
			file << " [shape=ellipse,fillcolor=1";
		}
		file << ",label=<";
		if (i.name() != "") {
			file << i.name() << "<BR/>";
		}
		else {
			file << i.pid() << "<BR/>";
		}
		file << "<I>" << i[0].type().name << "</I>>];" << std::endl;
	}

	file << "\n\t#__________________EDGES__________________\n";
	for (const auto &i : netw.connections()) {
		file << "\t" << i.pid_src() << " -> " << i.pid_tar() << " [label=<"
		     << i.label() << "<BR/><I>" << i.connector().name() << "<BR/>"
		     << i.connector().synapse_name() << "</I>>";
		if (i.connector().synapse().get() &&
		    i.connector().synapse().get()->parameters()[0] >= 0) {
			file << "color=4,";
		}
		else if (i.connector().synapse().get() &&
		         i.connector().synapse().get()->parameters()[0] < 0) {
			file << "color=2"
			     << ",arrowhead=dot,";
		}
		else {
			file << "color=black,";
		}
		file << "style=bold];" << std::endl;
	}

	file << "\n\t#________________LISTCONNS________________\n";
	for (size_t i = 0; i < netw.connections().size(); i++) {
		if (std::find(group_conns.begin(), group_conns.end(),
		              netw.connections()[i].connector().name()) !=
		    group_conns.end()) {
			continue;
		}

		file << "\tsubgraph cluster_connector" << i << "{\n";
		{
			auto pop = netw.populations()[netw.connections()[i].pid_src()];
			// First all nodes to a cluster
			file << "\t\tsubgraph cluster_connector_pops" << i << "{\n"
			     << "\t\t\tnode "
			        "[shape=circle,fillcolor=1,width=0.5,height=0.5]\n";
			file << "\t\t\tstyle = \"dotted\"\n";

			if (pop.name() != "") {
				file << "\t\t\tlabel = \"Population " << pop.name() << "\"\n";
			}
			else {
				file << "\t\t\tlabel = \"Population ID" << pop.name() << "\"\n";
			}
			for (size_t n = 0; n < pop.size(); n++) {
				file << "\t\t\ts" << i << n << "\n";
			}
			file << "\t\t}\n";
		}

		{
			auto pop = netw.populations()[netw.connections()[i].pid_tar()];
			// First all nodes to a cluster
			file << "\t\tsubgraph cluster_connector_popt" << i << "{\n"
			     << "\t\t\tnode "
			        "[shape=circle,fillcolor=1,width=0.5,height=0.5]\n";
			file << "\t\t\tstyle = \"dotted\"\n";

			if (pop.name() != "") {
				file << "\t\t\tlabel = \" Population " << pop.name() << "\"\n";
			}
			else {
				file << "\t\t\tlabel = \"Population ID" << pop.name() << "\"\n";
			}
			for (size_t n = 0; n < pop.size(); n++) {
				file << "\t\t\tt" << i << n << "\n";
			}
			file << "\t\t}\n";
		}

		if (netw.connections()[i].label() != "") {
			file << "\t\tlabel = \"Connector " << netw.connections()[i].label()
			     << "\"\n";
		}
		else {
			file << "\t\tlabel = \"Connector ID " << i << "\"\n";
		}
		file << "\t\tstyle = \"solid\"\n"
		     << "\t\tcolor = \"black\"\n";

		std::vector<LocalConnection> vec;
		netw.connections()[i].connector().connect(netw.connections()[i], vec);

		for (auto conn : vec) {
			file << "\t\t"
			     << "s" << i << conn.src << " -> "
			     << "t" << i << conn.tar << " [";
			if (conn.SynapseParameters[0] >= 0) {
				file << "color=4,";
			}
			else {
				file << "color=2,arrowhead=dot,";
			}
			file << "label=\"" << conn.SynapseParameters[0] << "\\n"
			     << conn.SynapseParameters[1] << "\"]\n";
		}
		file << "\t}\n";
	}

	file << "}" << std::endl;
	file.close();
	if (call_dot) {
		system(("dot -Tpdf " + filename + " -O").c_str());
	}
}
}  // namespace cypress
