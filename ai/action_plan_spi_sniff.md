# Action Plan: W5500 SPI Sniffing & State Dump

This plan details the steps to modify `micropython/extmod/network_wiznet5k.c` to implement a "sniffer" that dumps the W5500 register state and data after every SPI communication.

## Objective
To debug the RP2350 - W5500 communication by dumping the W5500 register state and the first 4 bytes of SPI data after each interaction.

## Files to Modify
- `micropython/extmod/network_wiznet5k.c`

## Step-by-Step Instructions

1.  **Define the Dump Function & Recursion Guard**
    -   Locate the debug configuration section (around line 135).
    -   Add a static volatile boolean flag `static volatile bool in_dump = false;` to prevent infinite recursion.
    -   Implement the `dump_w5500_state(const uint8_t *data, size_t len, bool is_write)` function.
    -   Add a recursion check at the start of the function: `if (in_dump) return;`.

2.  **Integrate into SPI Read/Write Functions**
    -   **`wiz_spi_read(uint8_t *buf, uint16_t len)`**:
        -   Call `dump_w5500_state(buf, len, false)` at the end.
    -   **`wiz_spi_writeburst(const uint8_t* pBuf, uint16_t len)`**:
        -   Call `dump_w5500_state(pBuf, len, true)` at the end.
    -   **`wiz_spi_readbyte()`**:
        -   Store result in `buf`, call `dump_w5500_state(&buf, 1, false)`, then return `buf`.
    -   **`wiz_spi_writebyte(const uint8_t buf)`**:
        -   Call `dump_w5500_state(&buf, 1, true)` at the end.

3.  **Integrate into DMA Write Function**
    -   **`w5500_dma_write_to_txbuf(uint8_t sn, uint16_t offset, const uint8_t* data, uint16_t len)`**:
        -   Call `dump_w5500_state(data, len, true)` at the end of the function.

## Code Snippets

### 1. Dump Function (Insert ~line 148)

```c
static volatile bool in_dump = false;

static void dump_w5500_state(const uint8_t *data, size_t len, bool is_write) {
    if (in_dump) return;
    in_dump = true;

    printf("\n=== SPI %s (%d bytes) ===\n", is_write ? "WRITE" : "READ", len);
    if (data && len > 0) {
        printf("Data: ");
        for (size_t i = 0; i < len && i < 4; i++) {
            printf("%02X ", data[i]);
        }
        if (len > 4) printf("...");
        printf("\n");
    }

    printf("--- W5500 STATE ---\n");
    printf("Mode Register: 0x%02X\n", WIZCHIP_READ(MR));
    printf("Socket 0 Status: 0x%02X\n", getSn_SR(0));
    printf("Socket 0 Interrupt: 0x%02X\n", getSn_IR(0));
    printf("Socket 0 TX_FSR: 0x%04X\n", getSn_TX_FSR(0));
    printf("Socket 0 RX_RSR: 0x%04X\n", getSn_RX_RSR(0));
    printf("Socket 0 TX_WR: 0x%04X\n", getSn_TX_WR(0));
    printf("Socket 0 TX_RD: 0x%04X\n", getSn_TX_RD(0));
    printf("Socket 0 RX_RD: 0x%04X\n", getSn_RX_RD(0));
    printf("======================\n\n");

    in_dump = false;
}
```

### 2. Modification Points

**`wiz_spi_read`**:
```c
static void wiz_spi_read(uint8_t *buf, uint16_t len) {
    wiznet5k_obj.spi_transfer(wiznet5k_obj.spi, len, buf, buf);
    dump_w5500_state(buf, len, false);
}
```

**`wiz_spi_writeburst`**:
```c
static void wiz_spi_writeburst(const uint8_t* pBuf, uint16_t len) {
    // ...
    dump_w5500_state(pBuf, len, true);
}
```

**`wiz_spi_readbyte`**:
```c
static uint8_t wiz_spi_readbyte() {
    uint8_t buf = 0;
    wiznet5k_obj.spi_transfer(wiznet5k_obj.spi, 1, &buf, &buf);
    dump_w5500_state(&buf, 1, false);
    return buf;
}
```

**`w5500_dma_write_to_txbuf`**:
```c
static void w5500_dma_write_to_txbuf(...) {
    // ...
    dump_w5500_state(data, len, true);
}
```
