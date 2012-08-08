/*
  model-select.cpp

  Takes a model (PDB, PSF, etc) and a selection string.  Parses the selection, then
  applies it to the PDB and writes the output to stdout.  This tool is
  used maily for checking your selection strings to make sure you're
  actually selecting what you intend to select...
*/



/*
  This file is part of LOOS.

  LOOS (Lightweight Object-Oriented Structure library)
  Copyright (c) 2008, Tod D. Romo
  Department of Biochemistry and Biophysics
  School of Medicine & Dentistry, University of Rochester

  This package (LOOS) is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation under version 3 of the License.

  This package is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <loos.hpp>

using namespace std;
using namespace loos;
namespace opts = loos::OptionsFramework;
namespace po = loos::OptionsFramework::po;


enum SplitMode { NONE, RESIDUE, MOLECULE, SEGID, NAME };

SplitMode mode;
string model_name;


string fullHelpMessage(void) {
  string msg =
    "\n"
    "SYNOPSIS\n"
    "\tRaw dump of a model subset in LOOS\n"
    "\n"
    "DESCRIPTION\n"
    "\n"
    "\tThis tool is useful for diagnosing problems with selections and how\n"
    "LOOS reads model files.  It will write out a pseudo-XML representation\n"
    "of the information it has stored about the selected subset.\n"
    "\n"
    "EXAMPLES\n"
    "\n"
    "\tmodel-select model.pdb >model.xml\n"
    "This example writes out ALL atoms\n"
    "\n"
    "\tmodel-select --selection 'name == \"CA\"' model.pdb >model-ca.xml\n"
    "This example only writes out alpha-carbons.\n"
    "\n"
    "\tmodel-select --selection 'resid <= 100' --splitby molecule >model-mols.xml\n"
    "This example splits the first 100 residues into molecules as determined\n"
    "by the sytem's connectivity.  Each group is written out separately.\n"
    "\n";

  return(msg);
}



class ToolOptions : public opts::OptionsPackage {
public:

  void addGeneric(po::options_description& o) {
    o.add_options()
      ("splitby", po::value<string>(&mode_string), "Split by molecule, residue, segid, name");
  }

  void addHidden(po::options_description& o) {
    o.add_options()
      ("model", po::value<string>(&model_name), "model");
  }

  void addPositional(po::positional_options_description& p) {
    p.add("model", 1);
  }

  bool postConditions(po::variables_map& map) {
    mode = NONE;

    if (!mode_string.empty()) {
      if (mode_string == "molecule")
        mode = MOLECULE;
      else if (mode_string == "residue")
        mode = RESIDUE;
      else if (mode_string == "segid")
        mode = SEGID;
      else if (mode_string == "name")
        mode = NAME;
      else
        return(false);
    }

    return(true);
  }

  string help() const { return("model"); }

  string print() const {
    ostringstream oss;

    oss << boost::format("mode='%s', model='%s'") % mode_string % model_name;
    return(oss.str());
  }


private:
  string mode_string;
};


void dumpChunks(const vector<AtomicGroup>& chunks) {
  for (uint i=0; i<chunks.size(); ++i) {
    cout << "<!-- *** Group #" << i << " -->\n";
    cout << chunks[i] << endl << endl; 
  }
}


int main(int argc, char *argv[]) {

  string header = invocationHeader(argc, argv);

  opts::BasicOptions* bopts = new opts::BasicOptions(fullHelpMessage());
  opts::BasicSelection* sopts = new opts::BasicSelection;
  ToolOptions* topts = new ToolOptions();

  opts::AggregateOptions options;
  options.add(bopts).add(sopts).add(topts);
  if (!options.parse(argc, argv))
    exit(-1);

  AtomicGroup model = createSystem(model_name);
  AtomicGroup subset = selectAtoms(model, sopts->selection);

  cerr << "You selected " << subset.size() << " atoms out of " << model.size() << endl;

  cout << "<!-- " << header << " -->\n";
  vector<AtomicGroup> chunks;
  map<string, AtomicGroup> named_chunks;

  switch(mode) {
  case MOLECULE:
    chunks = subset.splitByMolecule();
    dumpChunks(chunks);
    break;

  case RESIDUE:
    chunks = subset.splitByResidue();
    dumpChunks(chunks);
    break;

  case SEGID:
    chunks = subset.splitByUniqueSegid();
    dumpChunks(chunks);
    break;

  case NAME:
    named_chunks = subset.splitByName();
    for (map<string, AtomicGroup>::const_iterator i = named_chunks.begin(); i != named_chunks.end(); ++i) {
      cout << "<!-- Group for name '" << i->first << "' -->\n";
      cout << i->second << endl << endl;;
    }
    break;

  case NONE:
    cout << subset << endl;
    break;
  }

}
