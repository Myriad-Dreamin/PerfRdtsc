
## PerfRdtsc

Use tsc on x86 Linux / Linux Kernel / Windows, based on [tscns](https://github.com/MengRao/tscns).

#### Install

The library is header only. You can also link PerfRdtsc as a cmake interface library.

```cmake
add_subdirectory(vendor/rdtsc)
```

#### API Usage

```c++
#include <perfApi/rdtsc.h>

int runApp() {
  int64_t cpu_cycle = perfApiRdtsc(); // or precisely perfApiRdCycle()
  int64_t cpu_cycle = perfApiRdCycle();

  int64_t nanosecond = perfApiTsc2ns(cpu_cycle); // offline translation
  int64_t nanosecond = perfApiRdns();    // sampled by tsc instruction
  int64_t nanosecond = perfApiRdsysns(); // sampled by kernel syscall

  return 0;
}

int main() { perfApiTscInit(0); return runApp(); }
```
