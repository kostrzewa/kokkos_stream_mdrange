/*
//@HEADER
// ************************************************************************
//
//                        Kokkos v. 3.0
//       Copyright (2020) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY NTESS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL NTESS OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// ************************************************************************
//
// Modifications by Simon Schlepphorst (Uni Bonn) and
//                  Bartosz Kostrzewa (Uni Bonn) 
//
//@HEADER
*/

#include <Kokkos_Core.hpp>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <getopt.h>
#include <utility>
#include <iostream>
#include <limits>

#include <sys/time.h>

#define STREAM_NTIMES 20
using real_t = double;

#define HLINE "-------------------------------------------------------------\n"

using StreamDeviceArray =
    Kokkos::View<real_t ****, Kokkos::MemoryTraits<Kokkos::Restrict>>;
#if defined(KOKKOS_ENABLE_CUDA)
using constStreamDeviceArray =
    Kokkos::View<const real_t ****, Kokkos::MemoryTraits<Kokkos::RandomAccess>>;
#else
using constStreamDeviceArray =
    Kokkos::View<const real_t ****, Kokkos::MemoryTraits<Kokkos::Restrict>>;
#endif
using StreamHostArray = typename StreamDeviceArray::HostMirror;

using StreamIndex = int;

template <int rank>
using Policy      = Kokkos::MDRangePolicy<Kokkos::Rank<rank>>;

template <std::size_t... Idcs>
constexpr Kokkos::Array<std::size_t, sizeof...(Idcs)>
make_repeated_sequence_impl(std::size_t value, std::integer_sequence<std::size_t, Idcs...>)
{
  return { ((void)Idcs, value)... };
}

template <std::size_t N>
constexpr Kokkos::Array<std::size_t,N> make_repeated_sequence(std::size_t value)
{
  return make_repeated_sequence_impl(value, std::make_index_sequence<N>{});
}

template <typename V>    
auto    
get_tiling(const V view, const size_t factor = 1)    
{    
  constexpr auto rank = view.rank();    
  // extract the dimensions from the view layout (assuming no striding)    
  const auto & dimensions = view.layout().dimension;    
  Kokkos::Array<std::size_t,rank> dims;    
  for(int i = 0; i < rank; ++i){    
    dims[i] = dimensions[i];    
  }    
  // extract the recommended tiling for this view from a "default" policy     
  const auto rec_tiling = Policy<rank>(make_repeated_sequence<rank>(0),dims).tile_size_recommended();    
    
  if constexpr (std::is_same_v<typename V::execution_space, Kokkos::DefaultHostExecutionSpace>){    
    // for OpenMP we parallelise over the outermost dimension and so    
    // the chunk size corresponds to the view extent of the rightmost dimension
    // for OpenMP the
    // using 'factor' we keep the total tile volume constant but exchange it between the
    // outermost and innermost index
    const auto tile_size = view.extent(3)/factor == 0 ? 1 : view.extent(3)/factor;
    return Kokkos::Array<std::size_t,rank>({factor,2,view.extent(2),tile_size});    
  } else {    
    // for GPUs we use the recommended tiling for now, we just need to convert it appropriately    
    // from "array_index_type"    
    // unfortunately the recommended tile size can exceed the maximum block     
    // size on GPUs for large ranks -> let's cap the tiling at 4 dims    
    constexpr auto max_rank = rank > 4 ? 4 : rank;    
    Kokkos::Array<std::size_t,max_rank> res;    
    for(int i = 0; i < max_rank; ++i){    
      res[i] = rec_tiling[i];
    }
    // for GPUs, we reduce the tile-size on the fastest-running (left-most)
    // and increase the one on the slowest-running (right-most) dimension
    const auto tile_size = res[0]/factor == 0 ? 1 : res[0]/factor;
    res[0] = tile_size;
    res[max_rank-1] *= factor;
    return res;
  }
}

constexpr real_t ainit = 1.0;
constexpr real_t binit = 1.1;
constexpr real_t cinit = 0.0;

int parse_args(int argc, char **argv, StreamIndex &stream_array_size, size_t &tiling_factor) {
  // Defaults
  stream_array_size = 32;
  tiling_factor = 1;

  const std::string help_string =
      "  -n <N>, --nelements <N>\n"
      "     Create stream views containing <N>^4 elements.\n"
      "     Default: 32\n"
      "  -f <F>, --factor <F>\n"
      "     factor to inversely scale the fastest and slowest-running tile dimensions\n"
      "     Default: 1\n"
      "  -h, --help\n"
      "     Prints this message.\n"
      "     Hint: use --kokkos-help to see command line options provided by "
      "Kokkos.\n";

  static struct option long_options[] = {
      {"nelements", required_argument, NULL, 'n'},
      {"factor", required_argument, NULL, 'f'},
      {"help", no_argument, NULL, 'h'},
      {NULL, 0, NULL, 0}};

  int c;
  int option_index = 0;
  while ((c = getopt_long(argc, argv, "n:f:h", long_options, &option_index)) !=
         -1)
    switch (c) {
      case 'n': stream_array_size = atoi(optarg); break;
      case 'f': tiling_factor = static_cast<size_t>(atoi(optarg)); break;
      case 'h':
        printf("%s", help_string.c_str());
        return -2;
        break;
      case 0: break;
      default:
        printf("%s", help_string.c_str());
        return -1;
        break;
    }
  return 0;
}

void perform_set(const StreamDeviceArray a, const real_t scalar, const size_t tiling_factor) {
  constexpr auto rank = a.rank();
  const auto tiling = get_tiling(a,tiling_factor);
  Kokkos::parallel_for(
      "set", 
      Policy<rank>(make_repeated_sequence<rank>(0), make_repeated_sequence<rank>(a.extent(0)), tiling),
      KOKKOS_LAMBDA(const StreamIndex i, const StreamIndex j, const StreamIndex k, const StreamIndex l)
      { a(i,j,k,l) = scalar; });

  Kokkos::fence();
}

void perform_copy(const constStreamDeviceArray a, StreamDeviceArray b, const size_t tiling_factor) {
  constexpr auto rank = a.rank();
  const auto tiling = get_tiling(a,tiling_factor);
  Kokkos::parallel_for(
      "copy",
      Policy<rank>(make_repeated_sequence<rank>(0), make_repeated_sequence<rank>(a.extent(0)), tiling),
      KOKKOS_LAMBDA(const StreamIndex i, const StreamIndex j, const StreamIndex k, const StreamIndex l)
      { b(i,j,k,l) = a(i,j,k,l); });

  Kokkos::fence();
}

void perform_scale(StreamDeviceArray a, const constStreamDeviceArray b,
                   const real_t scalar, const size_t tiling_factor) {
  constexpr auto rank = b.rank();
  const auto tiling = get_tiling(a,tiling_factor);
  Kokkos::parallel_for(
      "scale",
      Policy<rank>(make_repeated_sequence<rank>(0), make_repeated_sequence<rank>(a.extent(0)), tiling),
      KOKKOS_LAMBDA(const StreamIndex i, const StreamIndex j, const StreamIndex k, const StreamIndex l)
      { a(i,j,k,l) = scalar * b(i,j,k,l); });

  Kokkos::fence();
}

void perform_add(const constStreamDeviceArray a,
                 const constStreamDeviceArray b, StreamDeviceArray c,
                 const size_t tiling_factor) {
  constexpr auto rank = a.rank();
  const auto tiling = get_tiling(a,tiling_factor);
  Kokkos::parallel_for(
      "add",
      Policy<rank>(make_repeated_sequence<rank>(0), make_repeated_sequence<rank>(a.extent(0)),tiling),
      KOKKOS_LAMBDA(const StreamIndex i, const StreamIndex j, const StreamIndex k, const StreamIndex l)
      { c(i,j,k,l) = a(i,j,k,l) + b(i,j,k,l); });

  Kokkos::fence();
}

void perform_triad(StreamDeviceArray a, const constStreamDeviceArray b,
                   const constStreamDeviceArray c, const real_t scalar,
                   const size_t tiling_factor) {
  constexpr auto rank = a.rank();
  const auto tiling = get_tiling(a,tiling_factor);
  Kokkos::parallel_for(
      "triad", 
      Policy<rank>(make_repeated_sequence<rank>(0), make_repeated_sequence<rank>(a.extent(0)),tiling),
      KOKKOS_LAMBDA(const StreamIndex i, const StreamIndex j, const StreamIndex k, const StreamIndex l)
      { a(i,j,k,l) = b(i,j,k,l) + scalar * c(i,j,k,l); });

  Kokkos::fence();
}

int perform_validation(StreamHostArray &a, StreamHostArray &b,
                       StreamHostArray &c, const StreamIndex arraySize,
                       const real_t scalar) {
  real_t ai = ainit;
  real_t bi = binit;
  real_t ci = cinit;

  for (int i = 0; i < STREAM_NTIMES; ++i) {
    ci = ai;
    bi = scalar * ci;
    ci = ai + bi;
    ai = bi + scalar * ci;
  };

  std::cout << "ai: " << ai << "\n";
  std::cout << "a(0,0,0,0): " << a(0,0,0,0) << "\n";
  std::cout << "bi: " << bi << "\n";
  std::cout << "b(0,0,0,0): " << b(0,0,0,0) << "\n";
  std::cout << "ci: " << ci << "\n";
  std::cout << "c(0,0,0,0): " << c(0,0,0,0) << "\n";
 
  const double nelem = (double)arraySize*
                       (double)arraySize*
                       (double)arraySize*
                       (double)arraySize;

  const double epsilon = 2*4*STREAM_NTIMES*std::numeric_limits<real_t>::epsilon();

  double aError = 0.0;
  double bError = 0.0;
  double cError = 0.0;

  #pragma omp parallel reduction(+:aError,bError,cError)
  {
    double err = 0.0;
    #pragma omp for collapse(2)
    for (StreamIndex i = 0; i < arraySize; ++i) {
      for (StreamIndex j = 0; j < arraySize; ++j) {
        for (StreamIndex k = 0; k < arraySize; ++k) {
          for (StreamIndex l = 0; l < arraySize; ++l) {
            err = std::abs(a(i,j,k,l) - ai);
            if( err > epsilon ){
              aError += err;
            }
            err = std::abs(b(i,j,k,l) - bi);
            if( err > epsilon ){
              bError += err;
            }
            err = std::abs(c(i,j,k,l) - ci);
            if( err > epsilon ){
              cError += err;
            }
          }
        }
      }
    }
  }

  std::cout << "aError = " << aError << "\n";
  std::cout << "bError = " << bError << "\n";
  std::cout << "cError = " << cError << "\n";

  real_t aAvgError = aError / nelem;
  real_t bAvgError = bError / nelem;
  real_t cAvgError = cError / nelem;

  std::cout << "aAvgErr = " << aAvgError << "\n";
  std::cout << "bAvgError = " << bAvgError << "\n";
  std::cout << "cAvgError = " << cAvgError << "\n";

  int errorCount       = 0;

  if (std::abs(aAvgError / ai) > epsilon) {
    fprintf(stderr, "Error: validation check on View a failed.\n");
    errorCount++;
  }

  if (std::abs(bAvgError / bi) > epsilon) {
    fprintf(stderr, "Error: validation check on View b failed.\n");
    errorCount++;
  }

  if (std::abs(cAvgError / ci) > epsilon) {
    fprintf(stderr, "Error: validation check on View c failed.\n");
    errorCount++;
  }

  if (errorCount == 0) {
    printf("All solutions checked and verified.\n");
  }

  return errorCount;
}

int run_benchmark(const StreamIndex stream_array_size, const size_t tiling_factor) {
  printf("Reports fastest timing per kernel\n");
  printf("Creating Views...\n");

  const double nelem = (double)stream_array_size*
                       (double)stream_array_size*
                       (double)stream_array_size*
                       (double)stream_array_size;

  printf("Memory Sizes:\n");
  printf("- Array Size:    %" PRIu64 "^4\n",
         static_cast<uint64_t>(stream_array_size));
  printf("- Per Array:     %12.2f MB\n",
         1.0e-6 * nelem * (double)sizeof(real_t));
  printf("- Total: %12.2f MB\n",
         3.0e-6 * nelem * (double)sizeof(real_t));
  printf("- Tiling Factor: %lu\n", tiling_factor);

  printf("Benchmark kernels will be performed for %d iterations.\n",
         STREAM_NTIMES);

  printf(HLINE);

  // WithoutInitializing to circumvent first touch bug on arm systems
  StreamDeviceArray dev_a(Kokkos::view_alloc(Kokkos::WithoutInitializing, "a"),
                          stream_array_size,stream_array_size,stream_array_size,stream_array_size);
  StreamDeviceArray dev_b(Kokkos::view_alloc(Kokkos::WithoutInitializing, "b"),
                          stream_array_size,stream_array_size,stream_array_size,stream_array_size);
  StreamDeviceArray dev_c(Kokkos::view_alloc(Kokkos::WithoutInitializing, "c"),
                          stream_array_size,stream_array_size,stream_array_size,stream_array_size);

  StreamHostArray a = Kokkos::create_mirror_view(dev_a);
  StreamHostArray b = Kokkos::create_mirror_view(dev_b);
  StreamHostArray c = Kokkos::create_mirror_view(dev_c);

  const double scalar = 1.1;

  double setTime   = std::numeric_limits<double>::max();
  double copyTime  = std::numeric_limits<double>::max();
  double scaleTime = std::numeric_limits<double>::max();
  double addTime   = std::numeric_limits<double>::max();
  double triadTime = std::numeric_limits<double>::max();

  printf("Initializing Views...\n");

  auto tiling = get_tiling(a,tiling_factor);
  Kokkos::parallel_for(
      "init",
      Kokkos::MDRangePolicy<Kokkos::Rank<a.rank()>,
                            Kokkos::DefaultHostExecutionSpace>(make_repeated_sequence<a.rank()>(0),
                                                               make_repeated_sequence<a.rank()>(stream_array_size),
                                                               tiling),
      KOKKOS_LAMBDA(const StreamIndex i, const StreamIndex j, const StreamIndex k, const StreamIndex l) {
        a(i,j,k,l) = ainit;
        b(i,j,k,l) = binit;
        c(i,j,k,l) = cinit;
      });
  Kokkos::fence();
  

  const auto dev_policy = Policy<dev_a.rank()>(make_repeated_sequence<dev_a.rank()>(0),
                                               make_repeated_sequence<dev_a.rank()>(stream_array_size));
  const auto recommended_tiling = dev_policy.tile_size_recommended();
  tiling = get_tiling(dev_a,tiling_factor);

  std::cout << "Recommended tiling: [";
  for( size_t i = 0; i < recommended_tiling.size(); ++i){
      std::cout << recommended_tiling[i];
      if( i < recommended_tiling.size()-1 ) std::cout << ",";
  }
  std::cout << "]\n";

  std::cout << "Used tiling: [";
  for( size_t i = 0; i < tiling.size(); ++i ){
    std::cout << tiling[i];
    if( i < tiling.size()-1 ) std::cout << ",";
  }
  std::cout << "]\n";

  Kokkos::parallel_for(
      "init_dev",
      Policy<dev_a.rank()>(make_repeated_sequence<dev_a.rank()>(0),
                           make_repeated_sequence<dev_a.rank()>(stream_array_size),
                           tiling),
      KOKKOS_LAMBDA(const StreamIndex i, const StreamIndex j, const StreamIndex k, const StreamIndex l) {
        dev_a(i,j,k,l) = ainit;
        dev_b(i,j,k,l) = binit;
        dev_c(i,j,k,l) = cinit;
      });
  Kokkos::fence();

  printf("Starting benchmarking...\n");

  Kokkos::Timer timer;

  for (int k = 0; k < STREAM_NTIMES; ++k) {
    timer.reset();
    perform_set(dev_c, 1.5, tiling_factor);
    setTime = std::min(setTime, timer.seconds());

    timer.reset();
    perform_copy(dev_a, dev_c, tiling_factor);
    copyTime = std::min(copyTime, timer.seconds());

    timer.reset();
    perform_scale(dev_b, dev_c, scalar, tiling_factor);
    scaleTime = std::min(scaleTime, timer.seconds());

    timer.reset();
    perform_add(dev_a, dev_b, dev_c, tiling_factor);
    addTime = std::min(addTime, timer.seconds());

    timer.reset();
    perform_triad(dev_a, dev_b, dev_c, scalar, tiling_factor);
    triadTime = std::min(triadTime, timer.seconds());
  }

  Kokkos::deep_copy(a, dev_a);
  Kokkos::deep_copy(b, dev_b);
  Kokkos::deep_copy(c, dev_c);

  printf("Performing validation...\n");
  int rc = perform_validation(a, b, c, stream_array_size, scalar);

  printf(HLINE);

  printf("Set             %11.4f GB/s\n",
         1.0e-09 * 1.0 * (double)sizeof(real_t) * nelem / setTime);

  printf("Copy            %11.4f GB/s\n",
         1.0e-09 * 2.0 * (double)sizeof(real_t) * nelem / copyTime);

  printf("Scale           %11.4f GB/s\n",
         1.0e-09 * 2.0 * (double)sizeof(real_t) * nelem / scaleTime);

  printf("Add             %11.4f GB/s\n",
         1.0e-09 * 3.0 * (double)sizeof(real_t) * nelem / addTime);

  printf("Triad           %11.4f GB/s\n",
         1.0e-09 * 3.0 * (double)sizeof(real_t) * nelem / triadTime);

  printf(HLINE);

  return rc;
}

int main(int argc, char *argv[]) {
  printf(HLINE);
  printf("Kokkos 4D MDRangePolicy Tiling Scan STREAM Benchmark\n");
  printf(HLINE);

  Kokkos::initialize(argc, argv);
  int rc;
  StreamIndex stream_array_size;
  size_t tiling_factor;
  rc = parse_args(argc, argv, stream_array_size, tiling_factor);
  if (rc == 0) {
    rc = run_benchmark(stream_array_size, tiling_factor);
  } else if (rc == -2) {
    // Don't return error code when called with "-h"
    rc = 0;
  }
  Kokkos::finalize();

  return rc;
}
