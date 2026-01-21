#Goal

Make a detailed action plan, and save it `to action_plan_double_send.md`,
study in details the "Problem found?" chapter, and base your action plan on it.

## Application description


It is a webpage serving application, where it serves a very big (~53kB) .svg file 
at the /svg endpoint. It is clearly bigger then the TX buffer (16kB).

Give extra attention to DMA handling. The w5500 should use DMA. 


## Problem Summary

First 172.16.1.1/svg is successful, I receive the whole .svg file nothing truncated.
The second call (loading again) fails.

The MCU hard locks with no timeout messages or error output.


# Architecture

| Component | Details |
|-----------|---------|
| MCU | RP2350A (Raspberry Pi Pico 2) |
| Ethernet | W5500 via SPI0 @ 20MHz |
| Camera | OV5640 via PIO + DMA |
| Frame size | 240x320 RGB565 = 153.6KB |
| TX buffer | 16KB (Socket 0) |
| RX buffer | 16KB (Socket 0) |

# Problem found?

**FOUND IT!** Look at lines 668-671:

```c
case TX_TRANSFERRING_CHUNK: {
    // ...
    setSn_CR(transfer->socket_num, Sn_CR_SEND);

    // Wait for command to register
    while (getSn_CR(transfer->socket_num)) {
        mpy_wiznet_yield();  // <-- THIS CALLS w5500_tx_service() RECURSIVELY!
    }

    transfer->state = TX_SENDING_PACKET;  // State change is AFTER the while loop
    break;
}
```

And `mpy_wiznet_yield()` at line 252-254:
```c
void mpy_wiznet_yield(void) {
    // ...
    w5500_tx_service();  // <-- RECURSIVE CALL!
}
```

## The Reentrance Bug

**During the first request**, this happens:
1. `w5500_tx_service()` enters `TX_TRANSFERRING_CHUNK` case
2. Issues `setSn_CR(sn, Sn_CR_SEND)`
3. Enters `while (getSn_CR(sn))` loop
4. Calls `mpy_wiznet_yield()` which calls `w5500_tx_service()` **recursively**
5. The recursive call sees the **same transfer still in TX_TRANSFERRING_CHUNK state**
6. It issues **ANOTHER** `setSn_CR(sn, Sn_CR_SEND)` - a double SEND command!
7. It updates `TX_WR` pointer again - corrupting the W5500's internal pointer tracking!

This corrupts the **W5500 chip's internal state** (not MCU state), which persists until power cycle.

## Let me verify the 4 angles you asked for:

### Angle 1: What state persists after 10 minutes?
- **W5500 chip internal state**: The TX buffer pointers are corrupted by the double SEND commands
- The chip's TCP state machine may be in an inconsistent state
- This persists because the W5500 has its own internal CPU/memory

### Angle 2: What runs in parallel with the program?
The W5500 chip runs autonomously. When you issue a `SEND` command, it starts reading from its TX buffer and transmitting. If you issue **two** SEND commands with corrupted `TX_WR` pointers:
- The W5500's internal state machine gets confused
- It might be waiting for something that never happens
- Or its buffer pointers are now invalid

### Angle 3: Why does the second request fail?
The first request "works" because the data still gets sent (even if via corrupted path). But the W5500's internal state is now broken. When the socket is closed and re-opened, the chip's internal structures are not fully reset. The second request encounters this corrupted state.

### Angle 4: Why does RX_RSR=333 show correct data?
The **RX side** of the W5500 is independent from the TX side. The chip can still:
- Accept TCP connections (SYN/SYN-ACK/ACK)
- Receive data into RX buffer
- Report correct RX_RSR

But the **SPI communication** or **chip state machine** may hang when the MCU tries to do certain operations after the TX corruption.

