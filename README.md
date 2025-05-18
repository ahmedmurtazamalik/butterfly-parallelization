Dataset:
- A graph of any number of nodes and edges can be generated using the given bpg.py python script. The variables denoting the amount of nodes in each set and the amount of approximate unique edges (approximate because duplicates might occur during randomization) must be changed from within the script to desired values before running. The script will generate "edges.dat" and "field.dat", which can then be read into C++ hashmaps using read&convert.cpp.
- The unweighted graph used for the ParButterfly paper has also been given, along with a script to read it into C++ hashmaps as well.
- The generated datasets can be plotted using the "plot.py" script.
- Sample datasets given include:
  1. "edges.dat" and "field.dat" files which define the edges and nodes in the bipartite graph. This graph has 5k nodes and ≈100k unique edges.
  2. The ParButterfly paper graph stored in "paperGraph.txt"

Note: Before running, make sure to adjust filepaths for the datasets in all the necessary header and code files, or move/copy the data to the code/header file directories. Also adjust the dataset file names in the code if need be.

Running:
1. Configure a C++ project in any IDE and paste all the cpp files in src from their respective folders into the project directory.
2. The MPI version "mpiver.cpp" must be run separately as MPI files are normally run, but with -fopenmp flag enabled to ensure OpenMP is defined, else it will run but not utilize OpenMP.
3. The Serial and OpenMP versions must be run by "Source.cpp" (with -fopenmp flag for OpenMP), however correct dataset file names must be ensured in both the serialCount.h and OpenMPCount.h files before running Source.cpp.
4. Thread count must be specified in both the OpenMPCount.h and mpiver.cpp files. By default, it will utilize the maximum CPU threads available.

Note: If you are running in an IDE that offers integrated OpenMP support like Microsoft Visual Studio, then you don't need to use CLI execution commands and subsequently the -fopenmp flag, you can run it directly. The -fopenmp flag is only needed when running on a CLI, and this goes for both the Source.cpp and mpiver.cpp files.
