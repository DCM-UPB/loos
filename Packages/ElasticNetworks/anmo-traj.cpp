/*
  anmo-traj

  (c) 2008,2013 Tod D. Romo, Grossfield Lab
      Department of Biochemistry
      University of Rochster School of Medicine and Dentistry

*/


/*
  This file is part of LOOS.

  LOOS (Lightweight Object-Oriented Structure library)
  Copyright (c) 2008,2013 Tod D. Romo
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
#include <boost/thread/thread.hpp>

typedef boost::thread* Bthread;

#include <limits>
#include <time.h>

#include "hessian.hpp"
#include "enm-lib.hpp"
#include "anm-lib.hpp"


using namespace std;
using namespace loos;
using namespace ENM;


namespace opts = loos::OptionsFramework;
namespace po = loos::OptionsFramework::po;


// Globals...  Icky poo!

string prefix;

int verbosity;

string spring_desc;
string bound_spring_desc;

string fullHelpMessage() {

  string s = 
    "SYNOPSIS\n"                                                                      
    "\n"
    "ANM-based trajectory analysis (modeled after Hall, et al, JACS 129:11394 (2007))\n"
    "\n"                                                                                      
    "DESCRIPTION\n"                                                                      
    "\n"
    "Computes the anisotropic network model for each frame in a trajectory.\n"
    "The smallest non-zero eigenvalue is written to a matrix.  This tool can compute\n"
    "either the all-to-all dot product between corresponding eigenvectors for each\n"
    "frame, or it can use the covariance overlap between the full set of eigenpairs\n"
    "computed for each frame.\n"
    "\n"
    "The following output files are created (using the optional prefix):\n"
    "\tanmo_traj_s.asc  - Smallest eigenvalue (magnitude of lowest frequency mode)\n"
    "\t                  First column is timestep, second column is the magnitude.\n"
    "\tanmo_traj_D.asc  - Matrix of dot products between corresponding eigenvectors (default)\n"
    "\tanmo_traj_O.asc  - Matrix of covariance overlaps (if requested).\n"
    "\n"
    "\n"
    "* Spring Constant Control *\n"
    "Contacts between beads in an ANM are connected by a single potential\n"
    "which is described by a hookean spring.  The stiffness of each connection\n"
    "can be modified using various definitions of the spring constant.\n"
    "The spring constant used is controlled by the --spring option.\n"
    "If only the name for the spring function is given, then the default\n"
    "parameters are used.  Alternatively, the name may include a\n"
    "comma-separated list of parameters to be passed to the spring\n"
    "function, i.e. --spring=distance,15.0\n\n"
    "Available spring functions:\n";

  vector<string> names = springNames();
  for (vector<string>::const_iterator i = names.begin(); i != names.end(); ++i)
    s = s + "\t" + *i + "\n";
  
  s += 
    "\n\n"
    "* Threading *\n"
    "Since the covariance overlap is an expensive calculation, the all-to-all\n"
    "covariance overlap code is multithreaded.  You can control how many threads\n"
    "are used with the --threads option.  Our testing suggests that the best\n"
    "performance is achieved with a non-threaded ATLAS and using only as many\n"
    "threads as you have physical cores.\n"
    "\n"
    "* Adding \"Connectivity\" *\n"
    "ANM also supports construction of spring connections based on\n"
    "pseudo-connectivity.  This allows beads neighboring in sequence\n"
    "to be connected by a separate \"bound\" spring, chosen using the\n"
    "--bound option.  In this case the other or \"non-bound\" spring is\n"
    "chosen with the --spring option.\n"
    "\n"
    "\n\n"
    "EXAMPLES\n\n"
    "anmo-traj --prefix b2ar b2ar.pdb b2ar.dcd\n"
    "\tCompute the ANM for all alpha-carbons in b2ar.  The output files are\n"
    "\tb2ar_s.asc (eigenvalues) and b2ar_U.asc (eigenvectors).\n"
    "\n"
    "anmo-traj --selection 'resid >= 10 && resid <= 50 && name == \"CA\"' foo.pdb foo.dcd\n"
    "\tCompute the ANM for residues #10 through #50 with a 15 Angstrom cutoff\n"
    "\ti.e. construct contacts using only the CA's that are within 15 Angstroms\n"
    "\tThe model is in foo.pdb and the trajectory is stored in foo.dcd.  Output files\n"
    "\tcreated are anm_traj_s.asc (eigenvalues) and anm_traj_U.asc (eigenvectors).\n"
    "\n"
    "anmo-traj -S=exponential,-1.3 foo.pdb foo.dcd\n"
    "\tCompute an ANM using an spring function where the magnitude of\n"
    "\tthe connection decays exponentially with distance at a rate of\n"
    "\texp(-1.3*r) where r is the distance between contacts.  Note:\n"
    "\tin this case all beads are connected - which can eliminate\n"
    "\tan error in the numeric eigendecomposition.\n"
    "\n"
    "anmo-traj -b=constant,100 -S=exponential,-1.3 foo.pdb foo.dcd\n"
    "\tSimilar to the example above, but using connectivity.  Here\n"
    "\tresidues that are adjacent in sequence are connected by\n"
    "\tsprings with a constant stiffness of \"100\" and all other\n"
    "\tresidues are connected by springs that decay exponentially\n"
    "\twith distance\n"
    "\n"
    "NOTES\n"
    "- The default selection (if none is specified) is to pick CA's\n"
    "- The output is ASCII format suitable for use with Matlab/Octave/Gnuplot\n"
    "- Verbsity setting of 1 will give progress updates\n"
    "\n"
    "SEE ALSO\n"
    "\n"
    "gnm, gnm-traj, anm\n"
    "\n";

  return(s);
}




// @cond TOOLS_INTERNAL
class ToolOptions : public opts::OptionsPackage {
public:
  
  void addGeneric(po::options_description& o) {
    o.add_options()
      ("spring", po::value<string>(&spring_desc)->default_value("distance"),"Spring function to use")
      ("bound", po::value<string>(&bound_spring_desc), "Bound spring")
      ("coverlap", po::value<bool>(&coverlap)->default_value(false), "Use covariance overlap rather than dot-product")
      ("threads", po::value<uint>(&nthreads)->default_value(2), "Number of threads to use for covariance overlap calculation")
      ("partial", po::value<uint>(&partial)->default_value(0), "Number of modes to use in coverlap (0 = all)");
    
    
    
  }

  string print() const {
    ostringstream oss;
    oss << boost::format("spring='%s',bound='%s',coverlap=%d,nthreads=%d,partial=%d") % spring_desc % bound_spring_desc % coverlap % nthreads % partial;
    return(oss.str());
  }

  bool coverlap;
  uint nthreads;
  uint partial;
};



/*
 A local "fast" version of ANM.  The standard ANM
 uses the SVD to diagonalize the Hessian matrix.  This
 is slow and computes RSV's, which are not necessary here.
 So, we use LOOS' eigenDecomp() function (that uses
 DSYEV for the diagonalizaiton)
*/

class FastANM : public ElasticNetworkModel {
public:
  FastANM(SuperBlock* b) : ElasticNetworkModel(b) { prefix_ = "anm"; }

  void solve() {

    if (verbosity_ > 2)
      std::cerr << "Building hessian...\n";
    buildHessian();

    loos::Timer<> t;
    if (verbosity_ > 1)
      std::cerr << "Computing Decomp of hessian...\n";
    t.start();

    eigenvals_ = eigenDecomp(hessian_);
    eigenvecs_ = hessian_;
    
    t.stop();
    if (verbosity_ > 1)
      std::cerr << "Decomp took " << loos::timeAsString(t.elapsed()) << std::endl;

  }


private:

};



/*
  This tool can use either dot products or covariance overlaps to
  analyze the fluctuation space for each ANM.  The Analyzer class
  provides the interface to abstract which method is used.
*/

struct Analyzer 
{
  Analyzer() : _k(0) 
  {
  }
  

  virtual ~Analyzer() 
  {
  }
  

  // Store a new set of eigenpairs, t is the corresponding timestep
  virtual void accumulate(const uint t, const DoubleMatrix& eigvals, const DoubleMatrix& eigvecs) =0;

  // Perform the analysis.  Prefix is the output matrix prefix and header is the associated metadata
  virtual void analyze(const string& prefix, const string& header) =0;

  // Used by derived classes that need an index for the passed ANM
  uint _k;
};




// Only retain the dominant eigenvector for dot product, and the first two eigenvalues
// Note: actually writes out the absolute value of the dot product

struct DotAnalyze : public Analyzer 
{
  DotAnalyze(const uint natoms, const uint nframes) :
    _natoms(natoms), _nframes(nframes),
    _eigvals(DoubleMatrix(nframes, 3)),
    _eigvecs(DoubleMatrix(natoms*3, nframes))
  {
  }

  void accumulate(const uint t, const DoubleMatrix& eigvals, const DoubleMatrix& eigvecs) 
  {
    _eigvals(_k, 0) = t;
    _eigvals(_k, 1) = eigvals[6];
    _eigvals(_k, 2) = eigvals[7];
    
    for (uint i=0; i<_natoms*3; ++i)
      _eigvecs(i, _k) = eigvecs(i, 6);

    ++_k;
  }
  
  void analyze(const string& prefix, const string& header) 
  {
    writeAsciiMatrix(prefix + "_s.asc", _eigvals, header);

    DoubleMatrix D = MMMultiply(_eigvecs, _eigvecs, true, false);
    for (ulong i=0; i<D.size(); ++i)
      D[i] = abs(D[i]);
    
    writeAsciiMatrix(prefix + "_D.asc", D, header);
  }
  

  uint _natoms, _nframes;
  DoubleMatrix _eigvals;
  DoubleMatrix _eigvecs;
};


// ---------------------------------------------------------

/*
 For performance reasons, the covariance overlap code is multi-threaded
 Initial testing suggests best performance is to NOT use the threaded
 ATLAS and use only as many threads as physical cores.

 As always, YMMV...
*/



typedef vector<DoubleMatrix> VDMat;


// Worker threads coordinate on which column of the all-to-all covariance overlap
// matrix to compute via the Master object.

class Master
{
public:
  Master(const uint nr) :
    toprow(0),
    maxrows(nr),
    verbose(false),
    start_time(time(0))
  {
  }

  Master(const uint nr, const bool b) :
    toprow(0),
    maxrows(nr),
    verbose(b),
    start_time(time(0)),
    total_elements(nr*(nr-1)/2)
  {
  }
  

  void setVerbose(const bool b) 
  {
    verbose = b;
  }
  
  
  // Checks whether there are any columns left to work on
  // and places the column index into the passed pointer.

  bool workAvailable(uint* ip) 
  {

    mtx.lock();
    if (toprow >= maxrows) {
      mtx.unlock();
      return(false);
    }
    *ip = toprow++;

    if (verbose) {
      if (toprow % 100 == 0) {
	time_t dt = elapsedTime();
	uint n = (toprow - 2) * (toprow-1) / 2;
	uint d = dt * total_elements / n - dt;

	uint hrs = d / 3600;
	uint remain = d % 3600;
	uint mins = remain / 60;
	uint secs = remain % 60;
	
	cerr << "Row = " << setw(8) << toprow << "\tElapsed = " << setw(10) << dt << " s\tEstimated Remain = ";
	if (hrs)
	  cerr << setw(3) << hrs << "h ";
	cerr << setw(2) << mins << "m " << secs << "s\n";
      }
    }
    
    mtx.unlock();
    return(true);
  }

  time_t elapsedTime() const 
  {
    return (time(0) - start_time);
  }
  

private:
  uint toprow, maxrows;
  bool verbose;
  time_t start_time;
  uint total_elements;
  boost::mutex mtx;
};



/*
  Worker thread processes a column of the all-to-all matrix.  Gets which
  column to work on from the associated Master object.

  Will store pointers to the vector of matrices representing eigenpairs
*/

class Worker 
{
public:
  Worker(DoubleMatrix* over, VDMat* vals, VDMat* vecs, Master* pmaster) : O(over), eigvals(vals), eigvecs(vecs), master(pmaster)
  {
  }

  Worker(const Worker& w) 
  {
    O = w.O;
    eigvals = w.eigvals;
    eigvecs = w.eigvecs;
    master = w.master;
  }
  

  void calc(const uint i) 
  {
    for (uint j=0; j<i; ++j) {
      double d = covarianceOverlap((*eigvals)[i], (*eigvecs)[i], (*eigvals)[j], (*eigvecs)[j]);
      (*O)(j, i) = (*O)(i, j) = d;
    }
    
  }

  void operator()() 
  {
    uint i;
    
    while (master->workAvailable(&i))
      calc(i);
  }
  

private:
  DoubleMatrix* O;
  VDMat* eigvals;
  VDMat* eigvecs;
  Master* master;
};




// Top level object/interface.  Will create np Worker threads, cloned from the
// one passed into the constructor.

template<class W>
class Threader 
{
public:
  Threader(W* wrkr, const uint np) :
    worker(wrkr),
    threads(vector<Bthread>(np))
  {

    for (uint i=0; i<np; ++i) {
      Worker w(*worker);
      threads[i] = new boost::thread(w);
    }
    
  }


  void join() 
  {
    for (uint i=0; i<threads.size(); ++i)
      threads[i]->join();
  }
  

  ~Threader() 
  {
    for (uint i=0; i<threads.size(); ++i)
      delete threads[i];
  }

  W* worker;
  vector<Bthread> threads;
};


// ---------------------------------------------------------
  


// Analyze ANM results using covariance overlap
// Can use a partial-coverlap (i.e. subset of modes)

struct CoverlapAnalyze : public Analyzer 
{
  CoverlapAnalyze(const bool v, const uint n, const uint nmodes, const uint nframes) :
    _verbosity(v),
    _nprocs(n),
    _nmodes(nmodes),
    _dom_eigvals(DoubleMatrix(nframes, 3))
  {
  }
  

  void accumulate(const uint t, const DoubleMatrix& eigvals, const DoubleMatrix& eigvecs) 
  {

    uint idx = _eigvals.size();
    _dom_eigvals(idx, 0) = t;
    _dom_eigvals(idx, 1) = eigvals[6];
    _dom_eigvals(idx, 2) = eigvals[7];

    DoubleMatrix e = submatrix(eigvals, loos::Math::Range(6, _nmodes+6), loos::Math::Range(0, eigvals.cols()));
    for (ulong i=0; i<e.rows(); ++i)
      e[i] = 1.0 / e[i];
    _eigvals.push_back(e);
    
    e = submatrix(eigvecs, loos::Math::Range(0, eigvecs.rows()), loos::Math::Range(6, _nmodes + 6));
    _eigvecs.push_back(e);
  }
  

  


  void analyze(const string& prefix, const string& header) 
  {
    DoubleMatrix O(_eigvecs.size(), _eigvecs.size());

    writeAsciiMatrix(prefix + "_s.asc", _dom_eigvals, header);

    if (_verbosity)
      cerr << boost::format("Computing coverlaps for %d frames using %d threads.\n")
	% _eigvecs.size() % _nprocs;
    
    Master master(O.rows(), _verbosity);
    Worker worker(&O, &_eigvals, &_eigvecs, &master);
    Threader<Worker> threads(&worker, _nprocs);
    
    threads.join();

    for (uint i=0; i<_eigvecs.size(); ++i)
      O(i, i) = 1.0;

    if (_verbosity) {
      cerr << "Done!\n";
      cerr << "Time to calculate coverlap matrix was " << master.elapsedTime() << " seconds\n";
    }
    

    writeAsciiMatrix(prefix + "_O.asc", O, header);
  }

    
  bool _verbosity;
  uint _nprocs;
  uint _nmodes;
  DoubleMatrix _dom_eigvals;

  vector<DoubleMatrix> _eigvals;
  vector<DoubleMatrix> _eigvecs;
};



// @endcond



loos::Math::Matrix<int> buildConnectivity(const AtomicGroup& model) {
  uint n = model.size();
  loos::Math::Matrix<int> M(n, n);
  
  for (uint j=0; j<n-1; ++j)
    for (uint i=j; i<n; ++i)
      if (i == j)
        M(j, i) = 1;
      else {
        M(j, i) = model[j]->isBoundTo(model[i]);
        M(i, j) = M(j, i);
      }
  
  return(M);
}



int main(int argc, char *argv[]) {

  string header = invocationHeader(argc, argv);
  
  opts::BasicOptions* bopts = new opts::BasicOptions(fullHelpMessage());
  opts::OutputPrefix* propts = new opts::OutputPrefix("anm_traj");
  opts::BasicSelection* sopts = new opts::BasicSelection("name == 'CA'");
  opts::BasicTrajectory* tropts = new opts::BasicTrajectory;
  ToolOptions* topts = new ToolOptions;

  opts::AggregateOptions options;
  options.add(bopts).add(propts).add(sopts).add(tropts).add(topts);
  if (!options.parse(argc, argv))
    exit(-1);

  AtomicGroup model = tropts->model;
  AtomicGroup subset = selectAtoms(model, sopts->selection);
  pTraj traj = tropts->trajectory;

  verbosity = bopts->verbosity;
  prefix = propts->prefix;


  if (verbosity > 0)
    cerr << boost::format("Selected %d atoms from %s\n") % subset.size() % tropts->model_name;

  // Determine which kind of scaling to apply to the Hessian...
  vector<SpringFunction*> springs;
  SpringFunction* spring = 0;
  spring = springFactory(spring_desc);
  springs.push_back(spring);

  vector<SuperBlock*> blocks;
  SuperBlock* blocker = new SuperBlock(spring, subset);
  blocks.push_back(blocker);


  // Handle Decoration (if necessary)
  if (!bound_spring_desc.empty()) {
    if (! model.hasBonds()) {
      cerr << "Error- cannot use bound springs unless the model has connectivity\n";
      exit(-10);
    }
    loos::Math::Matrix<int> M = buildConnectivity(subset);
    SpringFunction* bound_spring = springFactory(bound_spring_desc);
    springs.push_back(bound_spring);

    BoundSuperBlock* decorator = new BoundSuperBlock(blocker, bound_spring, M);
    blocks.push_back(decorator);

    blocker = decorator;
  }


  // Setup the ANM calculation object
  FastANM anm(blocker);
  anm.prefix(prefix);
  anm.meta(header);
  anm.verbosity(verbosity);


  // Now configure the analyzer
    
  uint t = tropts->skip;
  uint nframes = traj->nframes() - tropts->skip;
  uint natoms = subset.size();

  Analyzer* analyzer;
  if (topts->coverlap) {
    uint nmodes = topts->partial ? topts->partial : 3 * natoms - 6;
    cerr << boost::format("Using %d modes in coverlap\n") % nmodes;
    analyzer = new CoverlapAnalyze(verbosity, topts->nthreads, nmodes, nframes);
  } else
    analyzer = new DotAnalyze(natoms, nframes);


  // Setup progress counter
  PercentProgressWithTime watcher;
  ProgressCounter<PercentTrigger, EstimatingCounter> slayer(PercentTrigger(0.1), EstimatingCounter(nframes - tropts->skip));
  slayer.attach(&watcher);
  if (verbosity)
    slayer.start();


  while (traj->readFrame()) {
    traj->updateGroupCoords(subset);
    anm.solve();
    analyzer->accumulate(t++, anm.eigenvalues(), anm.eigenvectors());

    if (verbosity)
      slayer.update();
  }

  if (verbosity)
    slayer.finish();

  analyzer->analyze(prefix, header);

  for (vector<SuperBlock*>::iterator i = blocks.begin(); i != blocks.end(); ++i)
    delete *i;
  for (vector<SpringFunction*>::iterator i = springs.begin(); i != springs.end(); ++i)
    delete *i;
}