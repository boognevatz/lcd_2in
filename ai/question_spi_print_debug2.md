Take the `micropython/extmod/network_wiznet5k.c`, study it from at least 3 different angles. And
create an action plan named `action_plan_spi_sniff.md`, and put a step
by step instruction, what everyone can follow to implement the change.

After each spi communication there is a detailed print of the registers.
However I suspect some of the register printout are not working right.

Your job is implement the following:

Step 1 — Define Ground Truth Equations

For a 16 KB TX buffer:
Used bytes = (TX_WR − TX_RD) & 0x3FFF
Free bytes = 16384 − Used
⚠️ Never print TX_WR − TX_RD without masking.

Step 2 — Fix TX “Used” Debug Print
In dump_w5500_state() you currently print:
TX Buffer Used = (uint16_t)(tx_wr - tx_rd)
❌ This breaks when tx_wr < tx_rd.
✅ Replace with:

Mask with buffer size (0x3FFF)
Or compute via 16384 - FSR

Recommended (hardware‑truth):
Used = 16384 - getSn_TX_FSR(0)
This alone may explain the “looks wrong but works” symptom.


Please be  aware, the w5500 is a heavily modified driver, it does not use
lwIP, rather hardware tcp/ip. And w5500 uses a single 16kB RX 16kB TX on
socket  0, it is a single connection client-server architecture (the mcu,
ie. the  w550 is the server, and the client is a webbrowser).


---

FILELIST:
micropython/extmod/network_wiznet5k.c
micropython/extmod/machine_spi.c

