# Kokkos based stream benchmark with 1D and 2D, 3D, 4D and 5D views

* `stream-kokkos-range.cpp`: regular stream benchmark using 1D views and a 1D RangePolicy for the `parallel_for`
* `stream-kokkos-2d-mdrange.cpp`: stream benchmark using 2D views and a 2D MDRangePolicy for the `parallel_for`
* `stream-kokkos-3d-mdrange.cpp`: stream benchmark using 3D views and a 3D MDRangePolicy for the `parallel_for`
* `stream-kokkos-4d-mdrange.cpp`: stream benchmark using 4D views and a 4D MDRangePolicy for the `parallel_for`
* `stream-kokkos-5d-mdrange.cpp`: stream benchmark using 5D views and a 5D MDRangePolicy for the `parallel_for`

## Compilation instructions

Example compilation scripts are provided in the `compilation` directory for different architectures.

## Run scripts and results

Results of runs as well as run scripts are provided in the `run_scripts_and_results` directory for different architectures.

In the run scripts, the array extents are roughly matched. The worst matching is for the 5D version of the benchmark.

CPU benchmarks are run on both one and two sockets (using `OMP_PLACES=cores OMP_PROC_BIND=close` to ensure thread pinning to one socket when fewer threads are used than there are CPU cores).

See [results](run_scripts_and_results/kokkos_stream_mdrange.pdf) for result plots.
