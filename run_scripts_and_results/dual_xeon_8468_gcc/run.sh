# approximately similarly sized 5D, 4D, 3D, 2D and 1D array extents
# the 5D extents are poorly matched
n5=(  4  6   8  10  12   16   20   28   32   40    44    48    52    58    68 )
n4=(  4  8  12  16  24   32   48   64   80   96   112   128   144   160   192 )
n3=(  8 16  32  40  64  128  180  256  360  440   540   640   760   860  1100 ) 
n2=( 16 64 144 256 576 1024 2304 4096 6400 9216 12544 16384 20736 25600 36864 )
n1=( 256 4096 20736 65536 331776 1048576 5308416 16777216 40960000 84934656 157351936 268435456 429981696 655360000 1358954496 )

echo nt n policy kernel bw > results.dat 

# 48 threads, close binding -> use a single socket
# 96 threads, both sockets
for nt in 48 96; do
  export OMP_NUM_THREADS=$nt
  export OMP_PLACES=cores
  export OMP_PROC_BIND=close
  for n in $( seq 0 $(( ${#n4[@]} - 1 )) ); do

    N=${n1[$n]}
    results=( $(./stream-kokkos-range -n ${n1[$n]} | grep GB | awk '{print $1 " " $2}') )
    for i in $(seq 0 4); do
      echo $nt $N range ${results[$(( 2*$i ))]} ${results[$(( 2*$i + 1 ))]} | tee -a results.dat
    done
    
    N=$(( ${n2[$n]}*${n2[$n]} ))
    results=( $(./stream-kokkos-2d-mdrange -n ${n2[$n]} | grep GB | awk '{print $1 " " $2}') )
    for i in $(seq 0 4); do
      echo $nt $N 2d-mdrange ${results[$(( 2*$i ))]} ${results[$(( 2*$i + 1 ))]} | tee -a results.dat
    done

    N=$(( ${n3[$n]}*${n3[$n]}*${n3[$n]} ))
    results=( $(./stream-kokkos-3d-mdrange -n ${n3[$n]} | grep GB | awk '{print $1 " " $2}') )
    for i in $(seq 0 4); do
      echo $nt $N 3d-mdrange ${results[$(( 2*$i ))]} ${results[$(( 2*$i + 1 ))]} | tee -a results.dat
    done

    N=$(( ${n4[$n]}*${n4[$n]}*${n4[$n]}*${n4[$n]} ))
    results=( $(./stream-kokkos-4d-mdrange -n ${n4[$n]} | grep GB | awk '{print $1 " " $2}') )
    for i in $(seq 0 4); do
      echo $nt $N 4d-mdrange ${results[$(( 2*$i ))]} ${results[$(( 2*$i + 1 ))]} | tee -a results.dat
    done
    
    N=$(( ${n5[$n]}*${n5[$n]}*${n5[$n]}*${n5[$n]}*${n5[$n]} ))
    results=( $(./stream-kokkos-5d-mdrange -n ${n5[$n]} | grep GB | awk '{print $1 " " $2}') )
    for i in $(seq 0 4); do
      echo $nt $N 5d-mdrange ${results[$(( 2*$i ))]} ${results[$(( 2*$i + 1 ))]} | tee -a results.dat
    done
  done || break
done
