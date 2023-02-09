//
// Created by Myriad Dreamin on 2022/07/07.
//
/*
MIT License
Copyright (c) 2019 Meng Rao <raomeng1@gmail.com>
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef PERF_API_RDTSC_H
#define PERF_API_RDTSC_H

#ifdef __cplusplus
#ifndef mRdtscExternC
#define mRdtscExternC extern "C"
#endif
#else
#define mRdtscExternC
#endif

#ifndef mRdtscSilentInit
#define mRdtscSilentInit 1
#endif

#define mRdtscPlatformWindows 1
#define mRdtscPlatformLinuxUser 2
#define mRdtscPlatformLinuxKernel 3

#if defined(_MSC_VER) // Windows
#define mRdtscPlatformLit mRdtscPlatformWindows
#elif !defined(__KERNEL__) // Linux User Space
#define mRdtscPlatformLit mRdtscPlatformLinuxUser
#else // Linux Kernel Space
#define mRdtscPlatformLit mRdtscPlatformLinuxKernel
#endif

#if (mRdtscPlatformLit == mRdtscPlatformWindows)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define RDTSC_CUSTOM_WIN32_LEAN_AND_MEAN
#endif

#ifndef NOSERVICE
#define NOSERVICE
#define RDTSC_CUSTOM_NOSERVICE
#endif

#ifdef __cplusplus // we disable min/max macro in c++ context
#ifndef NOMINMAX
#define NOMINMAX
#define RDTSC_CUSTOM_NOMINMAX
#endif
#endif

#include <Windows.h>

#ifdef RDTSC_CUSTOM_WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#undef RDTSC_CUSTOM_WIN32_LEAN_AND_MEAN
#endif
#ifdef RDTSC_CUSTOM_NOSERVICE
#undef NOSERVICE
#undef RDTSC_CUSTOM_NOSERVICE
#endif
#ifdef RDTSC_CUSTOM_NOMINMAX
#undef NOMINMAX
#undef RDTSC_CUSTOM_NOMINMAX
#endif
#endif

#if (mRdtscPlatformLit == mRdtscPlatformWindows) ||                            \
  (mRdtscPlatformLit == mRdtscPlatformLinuxUser)
#ifdef __cplusplus
#include <cstdint>
#include <cstdio>
#include <ctime>
#else
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#endif
#elif mRdtscPlatformLit == mRdtscPlatformLinuxKernel
#include <linux/printk.h>
#include <linux/timekeeping.h>
#include <linux/types.h>
#endif

#if mRdtscPlatformLit == mRdtscPlatformWindows
#define mGlobalTscPerf()
mRdtscExternC uint64_t __rdtsc(void);
#elif mRdtscPlatformLit == mRdtscPlatformLinuxUser
#define mGlobalTscPerf()                                                       \
  static __attribute__((constructor)) void perfApiTscGlobalInit_(void) {       \
    perfApiTscInit(0);                                                         \
  }
#elif mRdtscPlatformLit == mRdtscPlatformLinuxKernel
#define mGlobalTscPerf()
#else
#error "unknown platform"
#endif

#if mRdtscPlatformLit == mRdtscPlatformWindows
inline int clock_get_monotonic(struct timespec *tv) {

  static LARGE_INTEGER g_ticks_per_sec{};
  LARGE_INTEGER ticks;
  double seconds;

  if (!g_ticks_per_sec.QuadPart) { // todo: atomic
    QueryPerformanceFrequency(&g_ticks_per_sec);
    if (!g_ticks_per_sec.QuadPart) {
      errno = ENOTSUP;
      return -1;
    }
  }

  QueryPerformanceCounter(&ticks);

  seconds = (double)ticks.QuadPart / (double)g_ticks_per_sec.QuadPart;
  tv->tv_sec = (time_t)seconds;
  tv->tv_nsec = (long)((uint64_t)(seconds * 1000000000ULL) % 1000000000ULL);

  return 0;
}
#endif

struct alignas(64) GlobalTscPerf {
#if (mRdtscPlatformLit == mRdtscPlatformWindows) ||                            \
  (mRdtscPlatformLit == mRdtscPlatformLinuxUser)
  double tsc_ghz_inv; // make sure tsc_ghz_inv and ns_offset are on the same
#define m_tsc_ghz_inv global_tsc_perf_.tsc_ghz_inv
#elif (mRdtscPlatformLit == mRdtscPlatformLinuxKernel)
  /// to make kernel floating computation happy
  int64_t tsc_ghz_x; // make sure tsc_ghz_inv and ns_offset are on the same
  int64_t tsc_ghz_y;
#define m_tsc_ghz_inv                                                          \
  (((double)(global_tsc_perf_.tsc_ghz_x)) / global_tsc_perf_.tsc_ghz_y)
#else
#error "unknown platform"
#endif
  int64_t ns_offset;
  int64_t base_tsc;
  int64_t base_ns;
  // cache line
};

inline struct GlobalTscPerf global_tsc_perf_;

static inline int64_t perfApiTsc2ns(int64_t tsc) {
  return global_tsc_perf_.ns_offset + (int64_t)(tsc * m_tsc_ghz_inv);
}

static inline int64_t perfApiRdtsc(void) {
#if (mRdtscPlatformLit == mRdtscPlatformWindows)
  return __rdtsc();
#elif defined(__i386__) || defined(__x86_64__) || defined(__amd64__)
  return __builtin_ia32_rdtsc();
#else
  return rdsysns();
#endif
}

static inline int64_t perfApiRdns(void) {
  return perfApiTsc2ns(perfApiRdtsc());
}

static inline int64_t perfApiRdCycle(void) { return perfApiRdtsc(); }

// If you want cross-platform, use std::chrono as below which incurs one more
// function call: return
// std::chrono::high_resolution_clock::now().time_since_epoch().count();
static int64_t perfApiRdsysns(void) {
#if (mRdtscPlatformLit == mRdtscPlatformWindows)
  struct timespec ts;
  clock_get_monotonic(&ts);
#elif (mRdtscPlatformLit == mRdtscPlatformLinuxUser)
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
#else
  struct timespec64 ts;
  ktime_get_ts64(&ts);
#endif
  return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

// For checking purposes, see test.cc
// int64_t rdoffset() { return global_tsc_perf_.ns_offset; }

// Linux kernel sync time by finding the first try with tsc diff < 50000
// We do better: we find the try with the mininum tsc diff
static inline void tscInternalSyncTime(int64_t *tsc, int64_t *ns) {
#if mRdtscPlatformLit == mRdtscPlatformWindows
  const int tsc_sync_times = 15;
#else
  const int tsc_sync_times = 3;
#endif
  int i, j, best = 1;
  int64_t tscs[tsc_sync_times + 1];
  int64_t nses[tsc_sync_times + 1];

  tscs[0] = perfApiRdtsc();
  for (i = 1; i <= tsc_sync_times; i++) {
    nses[i] = perfApiRdsysns();
    tscs[i] = perfApiRdtsc();
  }

#if mRdtscPlatformLit == mRdtscPlatformWindows
  j = 1;
  for (i = 2; i <= tsc_sync_times; i++) {
    if (nses[i] == nses[i - 1])
      continue;
    tscs[j - 1] = tscs[i - 1];
    nses[j++] = nses[i];
  }
  j--;
#else
  j = N + 1;
#endif

  for (i = 2; i < j; i++) {
    if (tscs[i] - tscs[i - 1] < tscs[best] - tscs[best - 1]) {
      best = i;
    }
  }
  *tsc = (tscs[best] + tscs[best - 1]) >> 1;
  *ns = nses[best];
}

static inline void tscInternalAdjustOffset(void) {
#ifndef __KERNEL__
  global_tsc_perf_.ns_offset =
    global_tsc_perf_.base_ns -
    (int64_t)(global_tsc_perf_.base_tsc * m_tsc_ghz_inv);
#endif
}

// If you haven't calibrated tsc_ghz on this machine, set tsc_ghz as 0.0 and it
// will auto wait 10 ms and calibrate. Of course you can calibrate again
// later(e.g. after system init is done) and the longer you wait the more
// precise tsc_ghz calibrate can get. It's a good idea that user waits as long
// as possible(more than 1 min) once, and save the resultant tsc_ghz returned
// from calibrate() somewhere(e.g. config file) on this machine for future use.
// Or you can cheat, see README and cheat.cc for details.
//
// If you have calibrated/cheated before on this machine as above, set tsc_ghz
// and skip calibration.
//
// One more thing: you can re-init and calibrate TSCNS at later times if you
// want to re-sync with system time in case of NTP or manual time changes.
static inline void perfApiTscInit(double tsc_ghz) {
  tscInternalSyncTime(&global_tsc_perf_.base_tsc, &global_tsc_perf_.base_ns);
  if (tsc_ghz > 0) {
#ifndef __KERNEL__
    global_tsc_perf_.tsc_ghz_inv = 1.0 / tsc_ghz;
#else
    global_tsc_perf_.tsc_ghz_x = 1;
    global_tsc_perf_.tsc_ghz_y = tsc_ghz;
#endif
    tscInternalAdjustOffset();
  } else {
    // tsc_ghz = (10000000);
    int64_t delayed_tsc, delayed_ns;
    do {
      tscInternalSyncTime(&delayed_tsc, &delayed_ns);
    } while ((delayed_ns - global_tsc_perf_.base_ns) < 10000000);
#ifndef __KERNEL__
    global_tsc_perf_.tsc_ghz_inv =
      ((double)(delayed_ns - global_tsc_perf_.base_ns)) /
      (delayed_tsc - global_tsc_perf_.base_tsc);
#else
    global_tsc_perf_.tsc_ghz_x = delayed_ns - global_tsc_perf_.base_ns;
    global_tsc_perf_.tsc_ghz_y = delayed_tsc - global_tsc_perf_.base_tsc;
#endif
    tscInternalAdjustOffset();
  }

#if !mRdtscSilentInit
#if (mRdtscPlatformLit == mRdtscPlatformWindows)
  printf(
    "{ \"event\": \"init-tsc\",  \"ns-offset\": %I64d, \"factor\": %lf }\n",
    global_tsc_perf_.ns_offset, global_tsc_perf_.tsc_ghz_inv);
#elif (mRdtscPlatformLit == mRdtscPlatformLinuxUser)
  printf(
    "{ \"event\": \"init-tsc\",  \"ns-offset\": %ld, \"factor\": %lf }\n",
    global_tsc_perf_.ns_offset, global_tsc_perf_.tsc_ghz_inv);
#else
  printk(
    "{ \"event\": \"init-tsc\",  \"ns-offset\": %lld, \"factor_x\": %lld, "
    "\"factor_y\": %lld }\n",
    global_tsc_perf_.ns_offset, global_tsc_perf_.tsc_ghz_x,
    global_tsc_perf_.tsc_ghz_y);
#endif
#endif
  return;
}

#endif // PERF_API_RDTSC_H
