source load_modules.sh 

cmake \
  -DCMAKE_BUILD_TYPE=RELEASE \
  -DKokkos_ARCH_ZEN3=ON \
  -DKokkos_ARCH_AMPERE80=ON \
  -DKokkos_ENABLE_CUDA=ON \
  -DKokkos_ENABLE_CUDA_CONSTEXPR=ON \
  -DKokkos_ENABLE_OPENMP=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  ~/code/kokkos_stream_mdrange

