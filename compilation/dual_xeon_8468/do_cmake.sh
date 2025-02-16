source load_modules.sh 

CXXFLAGS="-O3 -mfma -mtune=sapphirerapids -march=sapphirerapids" \
cmake \
  -DKokkos_ARCH_NATIVE=ON \
  -DCMAKE_BUILD_TYPE=RELEASE \
  -DKokkos_ENABLE_OPENMP=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  ~/code/kokkos_stream_mdrange

