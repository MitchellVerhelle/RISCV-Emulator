#pragma once

#include "mmio_window.hpp"  // for MmioWindow
#include "riscv.hpp"        // for rv::RiscV

namespace rv {
  /// Hook up DRAM, cache, and CPU, and draw the initial splash into the MMIO FB.
  /// @param io_out  will be set to point at your MmioWindow
  /// @param cpu_out will be set to point at your freshly‐created RiscV
  void build_system(MmioWindow*& io_out, RiscV*& cpu_out);
}