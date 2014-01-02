tsp
===

Includes a variety of heuristic algorithms for solving the Travelling Salesperson Problem.  The algorithms included nearest neighbor, 2-opt, simulated anneal, and a hybrid of 2-opt and simumlated anneal.

#### Compilation: `make`

#### Usage:

  Usage: ./tsp -[adntv] -[f filename]
  Algorithms:
  	-Default: Nathan's Hybrid (honestly the best choice)
  	-n: Nearest Neighbor (only)
  	-t: Two-opt
  	-a: Simulated Anneal
  Display modes:
  	-v: Verbose (minor progress messages)
  	-d: Debug (lots of detailed messages)
  Input/Output:
  	-f: Specify file to use as input/source file
	      Note: this will result in a output file named [input file].tour

