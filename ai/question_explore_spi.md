Study the following action plan in detail. We will discuss it. 


# FOCUSED TODO: TX_RD/TX_WR Only - Random Values Debug
## Critical Insight: Only TX Pointers Broken, RX Pointers Work Fine

**Key Observation:**
- ✅ RX_RD and RX_WR: Work perfectly, show correct values
- ❌ TX_RD and TX_WR: Random garbage values that change each boot
- 🤔 **Why only TX registers are affected?**

This suggests the issue is specifically in:
1. The TX register read implementation (not general SPI)
2. The TX register address calculation
3. The TX register control byte/BSB

**If general SPI or buffer clearing were the issue, RX registers would also fail!**

---

## 🎯 Root Cause: TX Register Address/Control Byte Calculation Bug

Since RX registers work but TX registers don't, the problem is NOT:
- ❌ General SPI buffer contamination (would affect RX too)
- ❌ General SPI timing issues (would affect RX too)
- ❌ Uncleared RX buffer (would affect RX too)

The problem IS:
- ✅ **Wrong address/offset calculation specific to TX registers**
- ✅ **Wrong control byte/BSB for TX register block**
- ✅ **TX registers reading from uninitialized memory location**

---

## 📍 W5500 Register Map Reference

```
Socket 0 Register Offsets (from datasheet):
  Sn_TX_FSR    0x0020-0x0021  (TX Free Size) - This works!
  Sn_TX_RD     0x0022-0x0023  (TX Read Pointer) - BROKEN!
  Sn_TX_WR     0x0024-0x0025  (TX Write Pointer) - BROKEN!
  
  Sn_RX_RSR    0x0026-0x0027  (RX Received Size)
  Sn_RX_RD     0x0028-0x0029  (RX Read Pointer) - WORKS!
  Sn_RX_WR     0x002A-0x002B  (RX Write Pointer) - WORKS!
```

**Notice:** TX_RD and TX_WR are at 0x0022 and 0x0024, while RX_RD and RX_WR are at 0x0028 and 0x002A.

**Hypothesis:** The macro/function for TX_RD/TX_WR uses wrong offset, but
RX_RD/RX_WR uses correct offset.

---

## 🔍 STEP 1: Compare TX vs RX Register Read Implementations

### Find the macro definitions

**Search in your codebase:**
```bash
grep -n "getSn_TX_WR\|getSn_TX_RD\|getSn_RX_WR\|getSn_RX_RD" \
  micropython/lib/wiznet5k/ micropython/extmod/ -r
```

**You're looking for definitions like:**
```c
#define getSn_TX_WR(sn)  WIZCHIP_READ_WORD(WIZCHIP_SREG_ADDR(sn, Sn_TX_WR))
#define getSn_TX_RD(sn)  WIZCHIP_READ_WORD(WIZCHIP_SREG_ADDR(sn, Sn_TX_RD))
#define getSn_RX_WR(sn)  WIZCHIP_READ_WORD(WIZCHIP_SREG_ADDR(sn, Sn_RX_WR))
#define getSn_RX_RD(sn)  WIZCHIP_READ_WORD(WIZCHIP_SREG_ADDR(sn, Sn_RX_RD))
```

**Or constant definitions:**
```c
#define Sn_TX_WR  0x0024
#define Sn_TX_RD  0x0022
#define Sn_RX_WR  0x002A
#define Sn_RX_RD  0x0028
```

### ADD COMPARISON LOGGING

**Location: After line 815 in dump_w5500_state()**

**PASTE THIS CODE:**

```c

// ===== TX vs RX REGISTER COMPARISON =====
static void compare_tx_rx_registers(uint8_t sn) {
  printf("\n[TX_RX_COMPARE] Comparing TX vs RX register reads for socket %d\n",
         sn);
  
  // Read RX registers (these work!)
  uint16_t rx_rd = getSn_RX_RD(sn);
  uint16_t rx_wr = getSn_RX_WR(sn);
  
  // Read TX registers (these are broken!)
  uint16_t tx_rd = getSn_TX_RD(sn);
  uint16_t tx_wr = getSn_TX_WR(sn);
  
  printf("[TX_RX_COMPARE] RX_RD: 0x%04X (GOOD)\n", rx_rd);
  printf("[TX_RX_COMPARE] RX_WR: 0x%04X (GOOD)\n", rx_wr);
  printf("[TX_RX_COMPARE] TX_RD: 0x%04X (BAD)\n", tx_rd);
  printf("[TX_RX_COMPARE] TX_WR: 0x%04X (BAD)\n", tx_wr);
  
  // Check if TX values look like they're from wrong memory region
  if (tx_rd == rx_rd || tx_wr == rx_wr) {
    printf("[TX_RX_COMPARE] *** TX pointer matches RX pointer! ***\n");
    printf("[TX_RX_COMPARE] *** This means TX macros use RX offsets! ***\n");
  }
  
  // Check for boundary violations
  if (rx_rd < 0x4000 && rx_wr < 0x4000 && 
      (tx_rd >= 0x4000 || tx_wr >= 0x4000)) {
    printf("[TX_RX_COMPARE] *** RX values valid, TX values exceed 16KB ***\n");
    printf("[TX_RX_COMPARE] *** TX macros read from wrong address ***\n");
  }
  
  printf("\n");
}
```

---

## 🔍 STEP 2: Manual Read of ONLY TX Registers with Explicit Addressing

**PASTE THIS CODE after compare_tx_rx_registers:**

```c

// Manual read of TX_WR using explicit address (bypass macros)
static uint16_t manual_read_tx_wr(uint8_t sn) {
  printf("\n[MANUAL_TX] Reading TX_WR manually for socket %d\n", sn);
  
  // W5500 Socket 0 register block control byte
  // BSB = 0x08 for socket 0 registers
  // Control = (BSB << 3) | (RWB << 2) | OM
  //         = (0x08 << 3) | (0 << 2) | 0b00
  //         = 0x40 | 0x00 | 0x00 = 0x40
  uint8_t control = 0x40;
  
  // TX_WR register offset for socket N
  uint16_t tx_wr_offset = 0x0024;
  
  // Build SPI transaction
  uint8_t txbuf[5];
  uint8_t rxbuf[5];
  
  txbuf[0] = control;
  txbuf[1] = (tx_wr_offset >> 8) & 0xFF;  // Address high byte
  txbuf[2] = tx_wr_offset & 0xFF;         // Address low byte
  txbuf[3] = 0x00;  // Dummy for data high byte
  txbuf[4] = 0x00;  // Dummy for data low byte
  
  memset(rxbuf, 0x00, sizeof(rxbuf));
  
  printf("[MANUAL_TX] Control: 0x%02X, Address: 0x%04X\n", 
         control, tx_wr_offset);
  printf("[MANUAL_TX] TX: [%02X %02X %02X %02X %02X]\n",
         txbuf[0], txbuf[1], txbuf[2], txbuf[3], txbuf[4]);
  
  // Execute SPI transaction
  mp_hal_pin_low(wiznet5k_obj.cs);
  mp_hal_delay_us(1);
  wiznet5k_obj.spi_transfer(wiznet5k_obj.spi, 5, txbuf, rxbuf);
  mp_hal_delay_us(1);
  mp_hal_pin_high(wiznet5k_obj.cs);
  
  printf("[MANUAL_TX] RX: [%02X %02X %02X %02X %02X]\n",
         rxbuf[0], rxbuf[1], rxbuf[2], rxbuf[3], rxbuf[4]);
  
  // W5500 returns data starting at byte 3
  uint16_t manual_value = (rxbuf[3] << 8) | rxbuf[4];
  printf("[MANUAL_TX] Manual TX_WR value: 0x%04X\n", manual_value);
  
  return manual_value;
}

// Manual read of TX_RD using explicit address
static uint16_t manual_read_tx_rd(uint8_t sn) {
  printf("\n[MANUAL_TX] Reading TX_RD manually for socket %d\n", sn);
  
  uint8_t control = 0x40;
  uint16_t tx_rd_offset = 0x0022;  // TX_RD offset
  
  uint8_t txbuf[5];
  uint8_t rxbuf[5];
  
  txbuf[0] = control;
  txbuf[1] = (tx_rd_offset >> 8) & 0xFF;
  txbuf[2] = tx_rd_offset & 0xFF;
  txbuf[3] = 0x00;
  txbuf[4] = 0x00;
  
  memset(rxbuf, 0x00, sizeof(rxbuf));
  
  printf("[MANUAL_TX] Control: 0x%02X, Address: 0x%04X\n",
         control, tx_rd_offset);
  printf("[MANUAL_TX] TX: [%02X %02X %02X %02X %02X]\n",
         txbuf[0], txbuf[1], txbuf[2], txbuf[3], txbuf[4]);
  
  mp_hal_pin_low(wiznet5k_obj.cs);
  mp_hal_delay_us(1);
  wiznet5k_obj.spi_transfer(wiznet5k_obj.spi, 5, txbuf, rxbuf);
  mp_hal_delay_us(1);
  mp_hal_pin_high(wiznet5k_obj.cs);
  
  printf("[MANUAL_TX] RX: [%02X %02X %02X %02X %02X]\n",
         rxbuf[0], rxbuf[1], rxbuf[2], rxbuf[3], rxbuf[4]);
  
  uint16_t manual_value = (rxbuf[3] << 8) | rxbuf[4];
  printf("[MANUAL_TX] Manual TX_RD value: 0x%04X\n", manual_value);
  
  return manual_value;
}

// Compare manual vs macro reads
static void validate_tx_register_reads(uint8_t sn) {
  printf("\n[VALIDATE_TX] Validating TX register reads for socket %d\n", sn);
  printf("[VALIDATE_TX] ============================================\n");
  
  // Manual reads (correct)
  uint16_t manual_tx_wr = manual_read_tx_wr(sn);
  uint16_t manual_tx_rd = manual_read_tx_rd(sn);
  
  // Macro reads (possibly wrong)
  uint16_t macro_tx_wr = getSn_TX_WR(sn);
  uint16_t macro_tx_rd = getSn_TX_RD(sn);
  
  printf("\n[VALIDATE_TX] COMPARISON:\n");
  printf("[VALIDATE_TX]   TX_WR: manual=0x%04X, macro=0x%04X %s\n",
         manual_tx_wr, macro_tx_wr,
         (manual_tx_wr == macro_tx_wr) ? "[MATCH]" : "[MISMATCH!]");
  printf("[VALIDATE_TX]   TX_RD: manual=0x%04X, macro=0x%04X %s\n",
         manual_tx_rd, macro_tx_rd,
         (manual_tx_rd == macro_tx_rd) ? "[MATCH]" : "[MISMATCH!]");
  
  if (manual_tx_wr != macro_tx_wr || manual_tx_rd != macro_tx_rd) {
    printf("\n[VALIDATE_TX] *** CONFIRMED: Macro reads are WRONG! ***\n");
    printf("[VALIDATE_TX] Manual reads (correct) should be used.\n");
    printf("[VALIDATE_TX] The getSn_TX_WR/getSn_TX_RD macros are broken!\n");
  } else {
    printf("\n[VALIDATE_TX] Macros match manual reads (both correct or both wrong)\n");
  }
  
  // Sanity check manual values
  if (manual_tx_wr < 0x4000 && manual_tx_rd < 0x4000) {
    printf("[VALIDATE_TX] Manual values look valid (< 16KB)\n");
  } else {
    printf("[VALIDATE_TX] WARNING: Even manual reads return invalid values!\n");
    printf("[VALIDATE_TX] This suggests deeper SPI or hardware issue.\n");
  }
  
  printf("[VALIDATE_TX] ============================================\n\n");
}
```

---

## 🔍 STEP 3: Add Validation Call to dump_w5500_state

### Location: Line 741 (after "Context" printf)

**FIND:**
```c
  printf(" Context: %s\n", context);
```

**REPLACE with:**
```c
  printf(" Context: %s\n", context);
  
  // Validate TX register reads
  compare_tx_rx_registers(0);
  validate_tx_register_reads(0);
```

---

## 🔍 STEP 4: Use Manual Reads in Register Dump

### Location: Lines 791-793

**FIND:**
```c
  uint16_t tx_fsr = getSn_TX_FSR(0);
  uint16_t tx_wr = getSn_TX_WR(0);
  uint16_t tx_rd = getSn_TX_RD(0);
```

**REPLACE with:**
```c
  uint16_t tx_fsr = getSn_TX_FSR(0);
  uint16_t tx_wr = manual_read_tx_wr(0);
  uint16_t tx_rd = manual_read_tx_rd(0);
  printf("  [NOTE] Using manual TX reads (macros are broken)\n");
```

---

## 📊 Expected Output Analysis

### Scenario 1: Macro Uses Wrong Offset (Most Likely)

```
[TX_RX_COMPARE] RX_RD: 0x0000 (GOOD)
[TX_RX_COMPARE] RX_WR: 0x014A (GOOD)
[TX_RX_COMPARE] TX_RD: 0xB45B (BAD)
[TX_RX_COMPARE] TX_WR: 0x560F (BAD)

[MANUAL_TX] Control: 0x40, Address: 0x0024
[MANUAL_TX] TX: [40 00 24 00 00]
[MANUAL_TX] RX: [00 00 00 00 00]  ← Correct! TX_WR is 0x0000
[MANUAL_TX] Manual TX_WR value: 0x0000

[VALIDATE_TX] COMPARISON:
[VALIDATE_TX]   TX_WR: manual=0x0000, macro=0x560F [MISMATCH!]
[VALIDATE_TX]   TX_RD: manual=0x0000, macro=0xB45B [MISMATCH!]
[VALIDATE_TX] *** CONFIRMED: Macro reads are WRONG! ***
```

**This proves:** The macros `getSn_TX_WR` and `getSn_TX_RD` are reading from wrong addresses!

**Next step:** Find and fix the macro definition.

### Scenario 2: Macro Uses Wrong Control Byte

```
[MANUAL_TX] Control: 0x40, Address: 0x0024
[MANUAL_TX] RX: [00 00 00 00 00]  ← Correct

[But if you try a different control byte:]
[MANUAL_TX] Control: 0x48, Address: 0x0024  ← Wrong BSB (0x09 instead of 0x08)
[MANUAL_TX] RX: [00 00 00 B4 5B]  ← Reading from wrong memory block!
```

This would mean the macro uses wrong Block Select Bits.

---

## 🎯 Root Cause Determination

After running the tests, you'll see one of these:

### Result A: Manual=0x0000, Macro=Random
**Root cause:** `getSn_TX_WR`/`getSn_TX_RD` macros use wrong offset or address calculation.

**Fix location:** Find the macro definition, compare with RX macro, fix the offset.

**Example fix:**
```c
// BEFORE (wrong):
#define getSn_TX_WR(sn)  WIZCHIP_READ_WORD(WIZCHIP_SREG_ADDR(sn, 0x002A))
                                                                    ^^^^^ Wrong!

// AFTER (correct):
#define getSn_TX_WR(sn)  WIZCHIP_READ_WORD(WIZCHIP_SREG_ADDR(sn, 0x0024))
                                                                    ^^^^^ Correct!
```

### Result B: Manual=Random, Macro=Random (both same)
**Root cause:** Deeper issue - either SPI problem or wrong control byte in BOTH manual and macro.

**Fix:** Check the control byte calculation, verify BSB is 0x08 for socket 0.

### Result C: Manual=0x0000, Macro=0x0000 (both work)
**Root cause:** ??? The macros work fine, issue is elsewhere (unlikely based on your evidence).

---

## 🔧 Likely Fix

Once you confirm manual reads work but macros don't, the fix is simple:

1. **Find the macro definition file** (likely `w5500.h` or `wizchip_conf.h`)
2. **Compare TX and RX macro definitions side by side**
3. **Copy the working RX macro structure to TX macros**

**Example:**
```c
// These work (RX):
#define getSn_RX_WR(sn)  WIZCHIP_READ_WORD(WIZCHIP_SREG_ADDR(sn, 0x002A))
#define getSn_RX_RD(sn)  WIZCHIP_READ_WORD(WIZCHIP_SREG_ADDR(sn, 0x0028))

// These are broken (TX) - probably wrong offsets:
#define getSn_TX_WR(sn)  WIZCHIP_READ_WORD(WIZCHIP_SREG_ADDR(sn, 0xXXXX))
#define getSn_TX_RD(sn)  WIZCHIP_READ_WORD(WIZCHIP_SREG_ADDR(sn, 0xXXXX))
                                                                    ^^^^^ Fix these!

// Should be:
#define getSn_TX_WR(sn)  WIZCHIP_READ_WORD(WIZCHIP_SREG_ADDR(sn, 0x0024))
#define getSn_TX_RD(sn)  WIZCHIP_READ_WORD(WIZCHIP_SREG_ADDR(sn, 0x0022))
```

---

## 📋 Quick Test Checklist

1. ✅ Apply Step 1-4 code changes
2. ✅ Compile and flash
3. ✅ Run test, capture output
4. ✅ Look for `[VALIDATE_TX]` section
5. ✅ Check if manual != macro (confirms broken macro)
6. ✅ Find macro definition file
7. ✅ Compare TX vs RX macro definitions
8. ✅ Fix TX macros to use correct offsets
9. ✅ Recompile and verify

---

## 💡 Why This Approach Works

**Key insight:** RX registers work, TX registers don't.

This PROVES the issue is in the TX-specific code path, not:
- ❌ General SPI functionality (RX works!)
- ❌ Buffer clearing (RX works!)
- ❌ Control byte base calculation (RX works!)
- ❌ Hardware issues (RX works!)

It MUST be:
- ✅ TX register offset/address wrong in macros
- ✅ TX-specific calculation bug
- ✅ Typo in TX macro definition

By comparing manual reads (known correct) vs macro reads (suspected wrong), we isolate the exact bug location.

---

Good luck! 🎯 This focused approach should nail it quickly.
