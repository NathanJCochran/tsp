tsp
===

Includes a variety of heuristic algorithms for solving the Travelling Salesperson Problem.  The algorithms included nearest neighbor, 2-opt, simulated anneal, and a hybrid of 2-opt and simumlated anneal.

#### Compilation:
`make tsp`

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

#### Input/Output
Note that input can be provided in a variety of ways:

1. From stdin (or via a file redirected through stdin)
2. From a file, as specified with the `-f` option

In the former case, results will be output directly to stdout.  In the latter case, results will be output to a file named after the input file, but with a .tour extension: `[input file].tour`.  However, additional information (such as can be triggered with the `-v` and `-d` options) will still be sent to stdout.
