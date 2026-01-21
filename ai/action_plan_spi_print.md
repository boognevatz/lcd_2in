# Action Plan: W5500 SPI Communication Sniffing & Register State Dumping

## Project Overview

This action plan provides step-by-step instructions to implement SPI
communication monitoring between RP2350 and W5500 Ethernet controller. After
each SPI transaction, the system will dump all relevant register states and
display data payloads (first 4 bytes in hex, with truncation indicators for
larger transfers).

**Target System:**
- MCU: RP2350
- Ethernet Controller: W5500 (hardware TCP/IP)
- Architecture: Single connection client-server (MCU as server, web browser as client)
- Memory: Single socket (Socket 0) with 16kB RX + 16kB TX buffers
- Driver: Modified MicroPython WizNet5k driver (no lwIP)

---

## Analysis from Three Angles

### 1. **Architectural Analysis**

The `network_wiznet5k.c` driver implements:
- SPI-based communication layer between RP2350 and W5500
- Register access functions (read/write operations)
- Socket management for TCP/IP operations
- Buffer management for TX/RX operations
- Interrupt handling for network events

Key components:
- `WIZCHIP_READ()` / `WIZCHIP_WRITE()` macros for register access
- `wiznet5k_spi_transfer()` for low-level SPI communication
- Socket functions: `getSn_SR()`, `getSn_TX_FSR()`, `getSn_RX_RSR()`, etc.
- Data transfer functions for reading/writing socket buffers

### 2. **Data Flow Analysis**

SPI Communication Flow:
```
Application Layer (MicroPython)
    ↓
Driver API (network_wiznet5k.c)
    ↓
Register/Buffer Operations
    ↓
SPI Transaction Layer
    ↓
Hardware SPI Controller (RP2350)
    ↓
W5500 Ethernet Controller
```

Critical Transaction Points:
- Register reads/writes (control, status, configuration)
- TX buffer writes (outgoing data)
- RX buffer reads (incoming data)
- Command executions (OPEN, CONNECT, SEND, RECV, etc.)

### 3. **Debugging & Monitoring Analysis**

Current limitations:
- No visibility into SPI transactions
- No register state tracking between operations
- Difficult to diagnose communication failures
- No data payload inspection capability

Benefits of implementing SPI sniffing:
- Real-time visibility into W5500 state transitions
- Ability to correlate SPI commands with register changes
- Data integrity verification
- Protocol-level debugging support
- Performance bottleneck identification

---

## Implementation Action Plan

## Very important key considerations

Since the driver (`network_wiznet5k.c`) is heavily modified, do not uses from 
the internet/github files, always uses this directory and its subdirectory 
the single source of truth.

Please be  aware, the w5500 is a heavily modified driver, it does not use
lwIP, rather hardware tcp/ip. And w5500 uses a single 16kB RX 16kB TX on
socket  0, it is a single connection client-server architecture (the mcu,
ie. the  w550 is the server, and the client is a webbrowser).



### Phase 1: Core Implementation

#### Step 1.1: Add Debug Configuration Flag

**File:** `micropython/extmod/network_wiznet5k.c`

Add at the top of the file (after includes):

```c
// W5500 SPI Sniffing Configuration
#ifndef W5500_SPI_DEBUG
#define W5500_SPI_DEBUG 1  // Set to 1 to enable, 0 to disable
#endif

#define W5500_MAX_DATA_PREVIEW 4  // Number of data bytes to show in hex
```

#### Step 1.2: Implement Register State Dump Function

**File:** `micropython/extmod/network_wiznet5k.c`

Add this function after the includes section and before the main driver code:

```c
#if W5500_SPI_DEBUG

// Forward declarations of WizChip functions (if not already included)
// These should be available through wizchip_conf.h and w5500.h
// uint8_t WIZCHIP_READ(uint32_t addr);
// uint8_t getSn_SR(uint8_t sn);
// uint8_t getSn_IR(uint8_t sn);
// uint16_t getSn_TX_FSR(uint8_t sn);
// uint16_t getSn_RX_RSR(uint8_t sn);
// uint16_t getSn_TX_WR(uint8_t sn);
// uint16_t getSn_TX_RD(uint8_t sn);
// uint16_t getSn_RX_RD(uint8_t sn);

static void dump_w5500_state(const char *context) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║           W5500 REGISTER STATE DUMP                    ║\n");
    printf("╠════════════════════════════════════════════════════════╣\n");
    printf("║ Context: %-46s║\n", context);
    printf("╠════════════════════════════════════════════════════════╣\n");
    
    // Common registers
    printf("║ [COMMON REGISTERS]                                     ║\n");
    printf("║   Mode Register (MR):           0x%02X                    ║\n", WIZCHIP_READ(0x0000));
    printf("║   Gateway Address:              %d.%d.%d.%d", 
           WIZCHIP_READ(0x0001), WIZCHIP_READ(0x0002), 
           WIZCHIP_READ(0x0003), WIZCHIP_READ(0x0004));
    printf("       ║\n");
    printf("║   Subnet Mask:                  %d.%d.%d.%d", 
           WIZCHIP_READ(0x0005), WIZCHIP_READ(0x0006), 
           WIZCHIP_READ(0x0007), WIZCHIP_READ(0x0008));
    printf("       ║\n");
    printf("║   Interrupt Register (IR):      0x%02X                    ║\n", WIZCHIP_READ(0x0015));
    printf("║   Interrupt Mask (IMR):         0x%02X                    ║\n", WIZCHIP_READ(0x0016));
    printf("║   PHY Config (PHYCFGR):         0x%02X                    ║\n", WIZCHIP_READ(0x002E));
    
    // Socket 0 registers (primary socket for single connection architecture)
    printf("╠════════════════════════════════════════════════════════╣\n");
    printf("║ [SOCKET 0 REGISTERS]                                   ║\n");
    printf("║   Mode (Sn_MR):                 0x%02X                    ║\n", getSn_MR(0));
    printf("║   Command (Sn_CR):              0x%02X                    ║\n", getSn_CR(0));
    printf("║   Status (Sn_SR):               0x%02X ", getSn_SR(0));
    
    // Decode status
    uint8_t status = getSn_SR(0);
    const char *status_str;
    switch(status) {
        case 0x00: status_str = "[CLOSED]"; break;
        case 0x13: status_str = "[INIT]"; break;
        case 0x14: status_str = "[LISTEN]"; break;
        case 0x17: status_str = "[ESTABLISHED]"; break;
        case 0x1C: status_str = "[CLOSE_WAIT]"; break;
        case 0x22: status_str = "[UDP]"; break;
        case 0x32: status_str = "[MACRAW]"; break;
        default:   status_str = "[UNKNOWN]"; break;
    }
    printf("%-13s ║\n", status_str);
    
    printf("║   Interrupt (Sn_IR):            0x%02X                    ║\n", getSn_IR(0));
    printf("║   Interrupt Mask (Sn_IMR):      0x%02X                    ║\n", getSn_IMR(0));
    
    // Port numbers
    uint16_t port = getSn_PORT(0);
    printf("║   Source Port (Sn_PORT):        %5d (0x%04X)        ║\n", port, port);
    
    uint16_t dport = getSn_DPORT(0);
    printf("║   Dest Port (Sn_DPORT):         %5d (0x%04X)        ║\n", dport, dport);
    
    // Buffer pointers and sizes
    printf("╠════════════════════════════════════════════════════════╣\n");
    printf("║ [TX BUFFER STATE]                                      ║\n");
    uint16_t tx_fsr = getSn_TX_FSR(0);
    uint16_t tx_wr = getSn_TX_WR(0);
    uint16_t tx_rd = getSn_TX_RD(0);
    printf("║   TX Free Size (FSR):           %5d bytes (0x%04X)   ║\n", tx_fsr, tx_fsr);
    printf("║   TX Write Pointer (WR):        0x%04X                  ║\n", tx_wr);
    printf("║   TX Read Pointer (RD):         0x%04X                  ║\n", tx_rd);
    printf("║   TX Buffer Used:               %5d bytes            ║\n", (uint16_t)(tx_wr - tx_rd));
    printf("║   TX Buffer Capacity:           16384 bytes            ║\n");
    
    printf("╠════════════════════════════════════════════════════════╣\n");
    printf("║ [RX BUFFER STATE]                                      ║\n");
    uint16_t rx_rsr = getSn_RX_RSR(0);
    uint16_t rx_rd = getSn_RX_RD(0);
    uint16_t rx_wr = getSn_RX_WR(0);
    printf("║   RX Received Size (RSR):       %5d bytes (0x%04X)   ║\n", rx_rsr, rx_rsr);
    printf("║   RX Read Pointer (RD):         0x%04X                  ║\n", rx_rd);
    printf("║   RX Write Pointer (WR):        0x%04X                  ║\n", rx_wr);
    printf("║   RX Buffer Available:          %5d bytes            ║\n", rx_rsr);
    printf("║   RX Buffer Capacity:           16384 bytes            ║\n");
    
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

static void dump_data_preview(const char *label, const uint8_t *data, size_t len) {
    if (len == 0) {
        printf("[%s] No data\n", label);
        return;
    }
    
    printf("[%s] Length: %zu bytes | Data: ", label, len);
    
    // Show first W5500_MAX_DATA_PREVIEW bytes in hex
    size_t preview_len = (len < W5500_MAX_DATA_PREVIEW) ? len : W5500_MAX_DATA_PREVIEW;
    for (size_t i = 0; i < preview_len; i++) {
        printf("%02X ", data[i]);
    }
    
    // Indicate if there's more data
    if (len > W5500_MAX_DATA_PREVIEW) {
        printf("... [+%zu bytes]", len - W5500_MAX_DATA_PREVIEW);
    }
    
    printf("\n");
}

#endif // W5500_SPI_DEBUG
```

#### Step 1.3: Identify SPI Transaction Points

**File:** `micropython/extmod/network_wiznet5k.c`

Locate these key functions that perform SPI transactions:
1. `wiznet5k_spi_transfer()` - Low-level SPI transfer
2. `WIZCHIP_READ()` / `WIZCHIP_WRITE()` - Register access
3. `wiz_send_data()` - TX buffer write
4. `wiz_recv_data()` - RX buffer read
5. Socket command functions: `wizchip_socket()`, `wizchip_connect()`, etc.

#### Step 1.4: Instrument SPI Transaction Wrapper

**File:** `micropython/extmod/network_wiznet5k.c`

Find the low-level SPI transfer function (typically named `wiznet5k_spi_transfer` or similar). 

Wrap it with debugging:

```c
#if W5500_SPI_DEBUG
static void log_spi_transaction(const char *operation, uint32_t addr, 
                                 const uint8_t *tx_data, uint8_t *rx_data, 
                                 size_t len) {
    printf("[SPI] %s | Addr: 0x%08X | Len: %zu\n", operation, addr, len);
    
    if (tx_data && len > 0) {
        dump_data_preview("TX", tx_data, len);
    }
    
    if (rx_data && len > 0) {
        dump_data_preview("RX", rx_data, len);
    }
}
#endif

// Modify existing SPI transfer function
static void wiznet5k_spi_transfer(wiznet5k_obj_t *self, size_t len, 
                                   const uint8_t *tx_data, uint8_t *rx_data) {
    #if W5500_SPI_DEBUG
    static uint32_t transaction_count = 0;
    printf("\n>>> SPI Transaction #%lu\n", ++transaction_count);
    #endif
    
    // Original SPI transfer code here
    // ... (existing implementation) ...
    
    #if W5500_SPI_DEBUG
    log_spi_transaction("Transfer", 0, tx_data, rx_data, len);
    #endif
}
```

#### Step 1.5: Instrument High-Level Operations

Add state dumps after key operations:

```c
// Example: After socket command execution
static void wizchip_socket_command(uint8_t sn, uint8_t cmd) {
    setSn_CR(sn, cmd);
    while (getSn_CR(sn)) {
        // Wait for command completion
    }
    
    #if W5500_SPI_DEBUG
    char context[64];
    snprintf(context, sizeof(context), "After Socket Command: 0x%02X", cmd);
    dump_w5500_state(context);
    #endif
}

// Example: After data transmission
static int wiznet5k_send_data(wiznet5k_obj_t *self, const uint8_t *buf, size_t len) {
    #if W5500_SPI_DEBUG
    dump_data_preview("SEND DATA", buf, len);
    #endif
    
    // Original send implementation
    // ... (existing code) ...
    
    #if W5500_SPI_DEBUG
    dump_w5500_state("After Send Data");
    #endif
    
    return result;
}

// Example: After data reception
static int wiznet5k_recv_data(wiznet5k_obj_t *self, uint8_t *buf, size_t len) {
    #if W5500_SPI_DEBUG
    dump_w5500_state("Before Recv Data");
    #endif
    
    // Original recv implementation
    // ... (existing code) ...
    
    #if W5500_SPI_DEBUG
    dump_data_preview("RECV DATA", buf, actual_len);
    dump_w5500_state("After Recv Data");
    #endif
    
    return actual_len;
}
```

---

### Phase 2: Strategic Instrumentation Points

#### Step 2.1: Critical Operation Points to Instrument

Add `dump_w5500_state()` calls at these locations:

1. **Initialization:**
   - After `wiznet5k_init()`
   - After PHY configuration
   - After network configuration (IP, subnet, gateway)

2. **Socket Operations:**
   - After `socket()` (socket creation)
   - After `listen()` (server mode)
   - After `connect()` (client mode)
   - After connection established (status = ESTABLISHED)
   - After `close()` (socket closure)

3. **Data Transfer:**
   - Before/after `send()`
   - Before/after `recv()`
   - When TX buffer becomes full
   - When RX buffer has data

4. **Interrupt Handling:**
   - After reading interrupt register
   - After processing each interrupt type
   - After clearing interrupts

5. **Error Conditions:**
   - On timeout
   - On connection failure
   - On buffer overflow
   - On unexpected status

#### Step 2.2: Example Instrumentation Pattern

```c
// Pattern for instrumenting any function
int wiznet5k_some_operation(wiznet5k_obj_t *self, ...) {
    #if W5500_SPI_DEBUG
    dump_w5500_state("Before: some_operation");
    #endif
    
    // ... original function code ...
    
    #if W5500_SPI_DEBUG
    dump_w5500_state("After: some_operation");
    #endif
    
    return result;
}
```

---



