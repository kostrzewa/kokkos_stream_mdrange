# approximately similarly sized 4D, 3D and 2D array extents
n4=(  4  8  12  16  24   32   48   64   80   96   112   128   144   160   192 )
n3=(  8 16  32  40  64  128  180  256  360  440   540   640   760   860  1100 ) 
n2=( 16 64 144 256 576 1024 2304 4096 6400 9216 12544 16384 20736 25600 36864 )

echo nt n factor policy kernel bw > results.dat 

# 2 sockets, 48 cores per socket
for nt in 96; do

  export OMP_NUM_THREADS=$nt
  export OMP_PLACES=cores
  export OMP_PROC_BIND=close
  
  for n in $( seq 0 $(( ${#n4[@]} - 1 )) ); do
    for f in 1 2 4 8 16 32 64; do
      N=$(( ${n2[$n]}*${n2[$n]} ))
      results=( $(./stream-kokkos-2d-mdrange-tiling-scan -n ${n2[$n]} -f ${f} | grep GB | awk '{print $1 " " $2}') )
      for i in $(seq 0 4); do
        echo $nt $N $f 2d-mdrange ${results[$(( 2*$i ))]} ${results[$(( 2*$i + 1 ))]} | tee -a results.dat
      done

      N=$(( ${n3[$n]}*${n3[$n]}*${n3[$n]} ))
      results=( $(./stream-kokkos-3d-mdrange-tiling-scan -n ${n3[$n]} -f ${f} | grep GB | awk '{print $1 " " $2}') )
      for i in $(seq 0 4); do
        echo $nt $N $f 3d-mdrange ${results[$(( 2*$i ))]} ${results[$(( 2*$i + 1 ))]} | tee -a results.dat
      done
      
      N=$(( ${n4[$n]}*${n4[$n]}*${n4[$n]}*${n4[$n]} ))
      results=( $(./stream-kokkos-4d-mdrange-tiling-scan -n ${n4[$n]} -f ${f} | grep GB | awk '{print $1 " " $2}') )
      for i in $(seq 0 4); do
        echo $nt $N $f 4d-mdrange ${results[$(( 2*$i ))]} ${results[$(( 2*$i + 1 ))]} | tee -a results.dat
      done
    done
  done || break
done
