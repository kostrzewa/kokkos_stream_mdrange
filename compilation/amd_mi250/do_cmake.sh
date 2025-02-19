source load_modules.sh

cmake \
  -DCMAKE_BUILD_TYPE=RELEASE \
  -DCMAKE_C_COMPILER=cc \
  -DCMAKE_CXX_COMPILER=CC \
  -DKokkos_ARCH_ZEN3=ON \
  -DKokkos_ENABLE_OPENMP=ON \
  -DKokkos_ARCH_AMD_GFX90A=ON \
  -DKokkos_ENABLE_HIP=ON \
  ~/code/kokkos_stream_mdrange

