Take the `micropython/extmod/network_wiznet5k.c`, study it from at least 3 different angles. And
create an action plan named `action_plan_spi_only.md`, and put a step
by step instruction, what everyone can follow to implement the change.

After each spi communication between rp2350 - w5500, spit out all the 
register state, along with the first 4 bytes of data in hex (the others can be shorted like 4kB of data). 
Here is a proof of concept function to do it:
void dump_w5500_state(void) {
    printf("\n=== W5500 STATE DUMP ===\n");
    printf("Mode Register: 0x%02X\n", WIZCHIP_READ(MR));
    printf("Socket 0 Status: 0x%02X\n", getSn_SR(0));
    printf("Socket 0 Interrupt: 0x%02X\n", getSn_IR(0));
    printf("Socket 0 TX_FSR: 0x%04X\n", getSn_TX_FSR(0));
    printf("Socket 0 RX_RSR: 0x%04X\n", getSn_RX_RSR(0));
    printf("Socket 0 TX_WR: 0x%04X\n", getSn_TX_WR(0));
    printf("Socket 0 TX_RD: 0x%04X\n", getSn_TX_RD(0));
    printf("Socket 0 RX_RD: 0x%04X\n", getSn_RX_RD(0));
    printf("======================\n\n");
}

Please be  aware, the w5500 is a heavily modified driver, it does not use
lwIP, rather hardware tcp/ip. And w5500 uses a single 16kB RX 16kB TX on
socket  0, it is a single connection client-server architecture (the mcu,
ie. the  w550 is the server, and the client is a webbrowser).

Do not touch the whitespacing in the code, only touch code, which is absolutely
necessary and related, eg. do not rewrite comments in unrelated part of the code, 
do not wrap existing code. Only touch what is necessary.

Also print out without freezing the device, mind it is only an mcu with
micropython.

Be mindful about the minimal code invasion, somebody needs to proofread it.

---

FILELIST:
micropython/extmod/network_wiznet5k.c
micropython/extmod/machine_spi.c


