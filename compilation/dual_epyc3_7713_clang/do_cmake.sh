source load_modules.sh 

CXXFLAGS="-O3 -mfma -mtune=znver3 -march=znver3" \
cmake \
  -DCMAKE_CXX_COMPILER=$(which clang++) \
  -DKokkos_ARCH_ZEN3=ON \
  -DCMAKE_BUILD_TYPE=RELEASE \
  -DKokkos_ENABLE_OPENMP=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  ~/code/kokkos_stream_mdrange

