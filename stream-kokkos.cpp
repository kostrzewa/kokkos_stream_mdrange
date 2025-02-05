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
// Modifications by Simon Schlepphorst (Uni Bonn)
//
//@HEADER
*/

#include <Kokkos_Core.hpp>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <getopt.h>

#include <sys/time.h>

#define STREAM_ARRAY_SIZE 10000000
#define STREAM_NTIMES 20
using real_t = float;

#define HLINE "-------------------------------------------------------------\n"

using StreamDeviceArray =
    Kokkos::View<real_t *, Kokkos::MemoryTraits<Kokkos::Restrict>>;
#if defined(KOKKOS_ENABLE_CUDA)
using constStreamDeviceArray =
    Kokkos::View<const real_t *, Kokkos::MemoryTraits<Kokkos::RandomAccess>>;
#else
using constStreamDeviceArray =
    Kokkos::View<const real_t *, Kokkos::MemoryTraits<Kokkos::Restrict>>;
#endif
using StreamHostArray = typename StreamDeviceArray::HostMirror;

using StreamIndex = int;
using Policy      = Kokkos::RangePolicy<Kokkos::IndexType<StreamIndex>>;

int parse_args(int argc, char **argv, StreamIndex &stream_array_size) {
  // Defaults
  stream_array_size = 1 << 26;

  const std::string help_string =
      "  -n <N>, --nelements <N>\n"
      "     Create stream arrays containing <N> elements.\n"
      "     Default: 1<<26\n"
      "  -h, --help\n"
      "     Prints this message.\n"
      "     Hint: use --kokkos-help to see command line options provided by "
      "Kokkos.\n";

  static struct option long_options[] = {
      {"nelements", required_argument, NULL, 'n'},
      {"help", no_argument, NULL, 'h'},
      {NULL, 0, NULL, 0}};

  int c;
  int option_index = 0;
  while ((c = getopt_long(argc, argv, "n:h", long_options, &option_index)) !=
         -1)
    switch (c) {
      case 'n': stream_array_size = atoi(optarg); break;
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

void perform_set(StreamDeviceArray &a, const real_t scalar) {
  Kokkos::parallel_for(
      "set", Policy(0, a.extent(0)),
      KOKKOS_LAMBDA(const StreamIndex i) { a[i] = scalar; });

  Kokkos::fence();
}

void perform_copy(const constStreamDeviceArray &a, StreamDeviceArray &b) {
  Kokkos::parallel_for(
      "copy", Policy(0, a.extent(0)),
      KOKKOS_LAMBDA(const StreamIndex i) { b[i] = a[i]; });

  Kokkos::fence();
}

void perform_scale(StreamDeviceArray &b, const constStreamDeviceArray &c,
                   const real_t scalar) {
  Kokkos::parallel_for(
      "scale", Policy(0, b.extent(0)),
      KOKKOS_LAMBDA(const StreamIndex i) { b[i] = scalar * c[i]; });

  Kokkos::fence();
}

void perform_add(const constStreamDeviceArray &a,
                 const constStreamDeviceArray &b, StreamDeviceArray &c) {
  Kokkos::parallel_for(
      "add", Policy(0, a.extent(0)),
      KOKKOS_LAMBDA(const StreamIndex i) { c[i] = a[i] + b[i]; });

  Kokkos::fence();
}

void perform_triad(StreamDeviceArray &a, const constStreamDeviceArray &b,
                   const constStreamDeviceArray &c, const real_t scalar) {
  Kokkos::parallel_for(
      "triad", Policy(0, a.extent(0)),
      KOKKOS_LAMBDA(const StreamIndex i) { a[i] = b[i] + scalar * c[i]; });

  Kokkos::fence();
}

int perform_validation(StreamHostArray &a, StreamHostArray &b,
                       StreamHostArray &c, const StreamIndex arraySize,
                       const real_t scalar) {
  real_t ai = 1.0;
  real_t bi = 2.0;
  real_t ci = 0.0;

  for (StreamIndex i = 0; i < arraySize; ++i) {
    ci = ai;
    bi = scalar * ci;
    ci = ai + bi;
    ai = bi + scalar * ci;
  };

  real_t aError = 0.0;
  real_t bError = 0.0;
  real_t cError = 0.0;

  for (StreamIndex i = 0; i < arraySize; ++i) {
    aError = std::abs(a[i] - ai);
    bError = std::abs(b[i] - bi);
    cError = std::abs(c[i] - ci);
  }

  real_t aAvgError = aError / (real_t)arraySize;
  real_t bAvgError = bError / (real_t)arraySize;
  real_t cAvgError = cError / (real_t)arraySize;

  const real_t epsilon = 1.0e-13;
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

int run_benchmark(const StreamIndex stream_array_size) {
  printf("Reports fastest timing per kernel\n");
  printf("Creating Views...\n");

  printf("Memory Sizes:\n");
  printf("- Array Size:    %" PRIu64 "\n",
         static_cast<uint64_t>(stream_array_size));
  printf("- Per Array:     %12.2f MB\n",
         1.0e-6 * (double)stream_array_size * (double)sizeof(real_t));
  printf("- Total:         %12.2f MB\n",
         3.0e-6 * (double)stream_array_size * (double)sizeof(real_t));

  printf("Benchmark kernels will be performed for %d iterations.\n",
         STREAM_NTIMES);

  printf(HLINE);

  // WithoutInitializing to circumvent first touch bug on arm systems
  StreamDeviceArray dev_a(Kokkos::view_alloc(Kokkos::WithoutInitializing, "a"),
                          stream_array_size);
  StreamDeviceArray dev_b(Kokkos::view_alloc(Kokkos::WithoutInitializing, "b"),
                          stream_array_size);
  StreamDeviceArray dev_c(Kokkos::view_alloc(Kokkos::WithoutInitializing, "c"),
                          stream_array_size);

  StreamHostArray a = Kokkos::create_mirror_view(dev_a);
  StreamHostArray b = Kokkos::create_mirror_view(dev_b);
  StreamHostArray c = Kokkos::create_mirror_view(dev_c);

  const double scalar = 3.0;

  double setTime   = std::numeric_limits<double>::max();
  double copyTime  = std::numeric_limits<double>::max();
  double scaleTime = std::numeric_limits<double>::max();
  double addTime   = std::numeric_limits<double>::max();
  double triadTime = std::numeric_limits<double>::max();

  printf("Initializing Views...\n");

  Kokkos::parallel_for(
      "init",
      Kokkos::RangePolicy<Kokkos::DefaultHostExecutionSpace>(0,
                                                             stream_array_size),
      KOKKOS_LAMBDA(const int i) {
        a[i] = 1.0;
        b[i] = 2.0;
        c[i] = 0.0;
      });

  // Copy contents of a (from the host) to the dev_a (device)
  Kokkos::deep_copy(dev_a, a);
  Kokkos::deep_copy(dev_b, b);
  Kokkos::deep_copy(dev_c, c);

  printf("Starting benchmarking...\n");

  Kokkos::Timer timer;

  for (StreamIndex k = 0; k < STREAM_NTIMES; ++k) {
    timer.reset();
    perform_set(dev_c, 1.5);
    setTime = std::min(setTime, timer.seconds());

    timer.reset();
    perform_copy(dev_a, dev_c);
    copyTime = std::min(copyTime, timer.seconds());

    timer.reset();
    perform_scale(dev_b, dev_c, scalar);
    scaleTime = std::min(scaleTime, timer.seconds());

    timer.reset();
    perform_add(dev_a, dev_b, dev_c);
    addTime = std::min(addTime, timer.seconds());

    timer.reset();
    perform_triad(dev_a, dev_b, dev_c, scalar);
    triadTime = std::min(triadTime, timer.seconds());
  }

  Kokkos::deep_copy(a, dev_a);
  Kokkos::deep_copy(b, dev_b);
  Kokkos::deep_copy(c, dev_c);

  printf("Performing validation...\n");
  int rc = perform_validation(a, b, c, stream_array_size, scalar);

  printf(HLINE);

  printf("Set             %11.4f GB/s\n",
         (1.0e-09 * 1.0 * (double)sizeof(real_t) * (double)stream_array_size) /
             setTime);
  printf("Copy            %11.4f GB/s\n",
         real_t(1.0e-09 * 2.0 * (double)sizeof(real_t) *
                (double)stream_array_size) /
             copyTime);
  printf("Scale           %11.4f GB/s\n",
         real_t(1.0e-09 * 2.0 * (double)sizeof(real_t) *
                (double)stream_array_size) /
             scaleTime);
  printf("Add             %11.4f GB/s\n",
         real_t(1.0e-09 * 3.0 * (double)sizeof(real_t) *
                (double)stream_array_size) /
             addTime);
  printf("Triad           %11.4f GB/s\n",
         real_t(1.0e-09 * 3.0 * (double)sizeof(real_t) *
                (double)stream_array_size) /
             triadTime);

  printf(HLINE);

  return rc;
}

int main(int argc, char *argv[]) {
  printf(HLINE);
  printf("Kokkos STREAM Benchmark\n");
  printf(HLINE);

  Kokkos::initialize(argc, argv);
  int rc;
  StreamIndex stream_array_size;
  rc = parse_args(argc, argv, stream_array_size);
  if (rc == 0) {
    rc = run_benchmark(stream_array_size);
  } else if (rc == -2) {
    // Don't return error code when called with "-h"
    rc = 0;
  }
  Kokkos::finalize();

  return rc;
}
