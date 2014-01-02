tsp
---

Includes a variety of heuristic algorithms for solving the Travelling Salesperson Problem: nearest neighbor, 2-opt, simulated anneal, and a hybrid of 2-opt and simumlated anneal.

This program solves cases of the Euclidean TSP.  In other words, these algorithms all operate on x and y coordinates  that specify the locations of cities in a Euclidean plane.  Distances between cities therefore satisfy the triangle inequality, making this a special case of the metric TSP.

#### Compilation:
`make tsp`

#### Usage:
	Usage: ./tsp {-n|-t|-a} {-v|-d} {[-f filename] | [input data...]}
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

#### Input/Output:
Note that input can be provided in a variety of ways:

1. From stdin (or via a file redirected through stdin)
2. From a file, as specified with the `-f` option

In the former case, results will be output directly to stdout.  In the latter case, results will be output to a file named after the input file, but with a .tour extension: `[input file].tour`.  However, in this latter case, additional information (such as can be triggered with the `-v` and `-d` options) will still be sent to stdout.

##### Input Format:

Input, whether from stdin or a file, must adhere to the following 3-column format, in which each city is given a unique id number (starting with 0), an x-coordinate, and a y-coordinate:

> id x-coordinate y-coordinate

For example, a valid input file might look like the following:

> 0 779 765  
> 1 717 102  
> 2 628 770  
> 3 555 155  
> 4 146 939  
> 5 967 993  
> 6 113 188  
> 7  51 762  
> 8 367 234  
> 9 971 813  
> 10  12 378

##### Output Format:

Output, whether to stdout or to a file, will have the following format:

> total tour length  
> 1st stop in tour  
> 2nd stop in tour  
> 3rd stop in tour  
> etc...  

A valid solution for the example input given above might therefore look like:

> 3327  
> 0  
> 5  
> 9  
> 1  
> 3  
> 8  
> 6  
> 10  
> 7  
> 4  
> 2  
