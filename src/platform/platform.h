/**
 * @file platform.h
 * @brief Compile-time platform target selection for the Zion HAL
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC
 *
 * Selects the concrete platform constants header at compile time and exposes a
 * single canonical HAL_TARGET_NAME string for diagnostics. Available targets:
 *
 *   - HAL_PLATFORM_KV260   AMD Kria KV260 Vision AI Starter Kit (K26 SOM)
 *   - HAL_PLATFORM_ZION    Zion custom Zynq UltraScale+ MPSoC carrier (legacy default)
 *
 * Selection happens via the CMake cache variable HAL_PLATFORM (kv260|zion);
 * `make`/Docker builds default to kv260. Runtime board detection
 * (platform/board_detect.h) complements this: modules probe actual hardware
 * and report SKIPPED instead of assuming anything beyond these constants.
 */

#pragma once

#if defined(HAL_PLATFORM_KV260)
  #include "platform_kv260.h"
  #define HAL_TARGET_NAME "Kria KV260 (AMD XCK26 Zynq UltraScale+ MPSoC, K26 SOM)"
#elif defined(HAL_PLATFORM_ZION)
  #include "platform_zynq.h"
  #define HAL_TARGET_NAME "Zion Zynq UltraScale+ MPSoC"
#else
  // Legacy default keeps existing builds green; new deployments set HAL_PLATFORM=kv260.
  #include "platform_zynq.h"
  #define HAL_TARGET_NAME "Zion (Zynq UltraScale+ MPSoC, legacy default)"
#endif

/// Canonical target description for logs and gRPC payloads.
static constexpr const char* HAL_PLATFORM_TARGET = HAL_TARGET_NAME;