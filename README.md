Dataset:
- A graph of any number of nodes and edges can be generated using the given bpg.py python script. The variables denoting the amount of nodes in each set and the amount of approximate unique edges (approximate because duplicates might occur during randomization) must be changed from within the script to desired values before running. The script will generate "edges.dat" and "field.dat", which can then be read into C++ hashmaps using read&convert.cpp.
- The unweighted graph used for the ParButterfly paper has also been given, along with a script to read it into C++ hashmaps as well.
- Sample datasets given include:
  1. "edges.dat" and "field.dat" files which define the edges and nodes in the bipartite graph. This graph has 5k nodes and ≈100k unique edges.
  2. The ParButterfly paper graph stored in "paperGraph.txt"
  
Running:
1. Configure a C++ project in any IDE and paste all the cpp files in src from their respective folders into the project directory.
2. The MPI version "mpiver.cpp" must be run separately as MPI files are normally run, but with -fopenmp flag enabled to ensure OpenMP is defined, else it will run but not utilize OpenMP.
3. The Serial and OpenMP versions must be run by "Source.cpp" (with -fopenmp flag for OpenMP), however correct dataset file names must be ensured in both the serialCount.h and OpenMPCount.h files before running Source.cpp.
4. Thread count must be specified in both the OpenMPCount.h and mpiver.cpp files. By default, it will utilize the maximum CPU threads available.
