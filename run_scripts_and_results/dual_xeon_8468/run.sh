echo nt n policy kernel bw > results.dat 

# 48 threads, close binding -> use a single socket
# 96 threads, both sockets
for nt in 48 96; do
  export OMP_NUM_THREADS=$nt
  export OMP_PLACES=cores
  export OMP_PROC_BIND=close
  for n in 4 8 12 16 24 32 64 96 128 144 192; do
    n4=$(( n*n*n*n ))
    results_rng=( $(./stream-kokkos -n $n4 | grep GB | awk '{print $1 " " $2}') )
    for i in $(seq 0 4); do
      echo $nt $n range ${results_rng[$(( 2*$i ))]} ${results_rng[$(( 2*$i + 1 ))]} | tee -a results.dat
    done
    results_md=( $(./stream-kokkos-mdrange -n $n | grep GB | awk '{print $1 " " $2}') )
    for i in $(seq 0 4); do
      echo $nt $n mdrange ${results_md[$(( 2*$i ))]} ${results_md[$(( 2*$i + 1 ))]} | tee -a results.dat
    done
  done || break
done
