Take the `micropython/extmod/network_wiznet5k.c`, study it from at least 3 different angles. And
create an action plan named `action_plan_spi_sniff.md`, and put a step
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

