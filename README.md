# HPC2k25-MPI
MPI distributed program to find the shortest path in distributed graphs.
It utilizes custom-build RDMA implementation to implement Delta-stepping.

Whole project is manged with `pixi`, no need to install dependencies. Only MPI runtime 
is required (as most HPC centers ship own).

## Build
To build the system one can simply run `pixi run build`. Keep in mind that MPI 
implementation is required!

## Generating and running
To generate some example data run `pixi run generate`. To run the code justy 
type `pixi run start`.
