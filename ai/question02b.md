### Detailed Step-by-Step Action Plan to Fix W5500 Compilation Error in MicroPython RP2 Port

Based on the analysis of `question02.md` and the codebase (`network_wiznet5k.c`, `machine_spi.c`), the issue arises because when `MICROPY_PY_LWIP` is set to 0 (enabling `WIZNET5K_PROVIDED_STACK`), the code attempts to use RP2-specific SPI and DMA types/functions (e.g., `machine_spi_obj_t`, `dma_claim_unused_channel`) without including the necessary headers or defining the types. These are only available in the lwIP path (`WIZNET5K_WITH_LWIP_STACK`).

The goal is to make minimal, targeted changes to restore compilation without redesigning the architecture, preserving the high-throughput DMA SPI burst for ~150kB transfers.

#### Prerequisites
- Confirm board config: Ensure `set(MICROPY_PY_LWIP 0)` and `set(MICROPY_PY_NETWORK_WIZNET5K 1)` are set in your `mpconfigboard.cmake` or equivalent.
- Backup: Create a git branch or backup of `micropython/extmod/network_wiznet5k.c` before changes.
- Test environment: Have a build setup ready to verify after changes.

#### Step 1: Add RP2 Hardware Headers Unconditionally
**File:** `micropython/extmod/network_wiznet5k.c`  
**Location:** Immediately after the standard includes (around line 36, after `#include "py/mphal.h"`).  
**Action:** Add the following includes to make RP2 hardware SPI and DMA functions/types available in both build paths.  
**Code to Add:**
```c
// RP2-specific: needed for SPI DMA burst (W5500, no lwIP)
#include "hardware/spi.h"
#include "hardware/dma.h"
```
**Rationale:** 
- The DMA burst code in `wiz_spi_writeburst` (lines ~167-210) and SPI access in `wiznet5k_active` (line ~914) require these headers.
- This is minimal and file-specific; no broader changes.
- Matches the pattern already used in `machine_spi.c`.

#### Step 2: Define `machine_spi_obj_t` Unconditionally
**File:** `micropython/extmod/network_wiznet5k.c`  
**Location:** Replace the conditional typedef (around lines 55-66) with an unconditional one.  
**Current Code (to Remove):**
```c
#if WIZNET5K_WITH_LWIP_STACK
// SPI object structure for RP2
typedef struct _machine_spi_obj_t {
    mp_obj_base_t base;
    spi_inst_t *spi_inst;
} machine_spi_obj_t;
#include "lib/wiznet5k/Ethernet/socket.h"
```
**New Code (to Add):**
```c
// Minimal local mirror of rp2 machine.SPI object (needed for DMA burst in provided stack)
typedef struct _machine_spi_obj_t {
    mp_obj_base_t base;
    spi_inst_t *spi_inst;
} machine_spi_obj_t;

#if WIZNET5K_WITH_LWIP_STACK
#include "lib/wiznet5k/Ethernet/socket.h"
```
**Rationale:**
- The struct is identical to the one in `ports/rp2/machine_spi.c` (confirmed via read).
- Makes `machine_spi_obj_t` available in `WIZNET5K_PROVIDED_STACK` for casting `wiznet5k_obj.spi` to access `spi_inst`.
- No architectural change; just ensures type visibility.
- Preserves existing lwIP includes.

#### Step 3: Verify No Other Changes Needed
**Action:** Do not modify, guard, or refactor the DMA burst code in `wiz_spi_writeburst` or `wiznet5k_active`. Leave as-is.  
**Rationale:** 
- The code is already correct for RP2 DMA SPI bursts.
- Changes in Steps 1-2 will resolve the undefined symbols (`machine_spi_obj_t`, `dma_*`, `spi_*`).
- Avoids introducing new bugs or redesigns.

#### Step 4: Build and Test
IT's the user's job. Dont do it now.

#### Potential Risks and Mitigations
- **Risk:** Type mismatch if RP2 SPI struct changes.  
  **Mitigation:** Verified struct matches `machine_spi.c`; low risk.
- **Risk:** Header conflicts.  
  **Mitigation:** Includes are standard RP2 SDK; tested in lwIP path.
- **Risk:** Performance impact.  
  **Mitigation:** Zero impact; only adds compile-time visibility.
- **Fallback:** If issues arise, revert changes and investigate alternative (e.g., include guards), but avoid redesign.

This plan achieves the goal of minimal fixes for compilation while preserving DMA throughput. Total changes: ~10 lines. If you encounter issues, provide build output for refinement.
