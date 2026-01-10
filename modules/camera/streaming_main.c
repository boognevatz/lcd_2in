#include <stdint.h>
#include <stdbool.h>
#include "cam.h"    // Your OV5640 PIO+DMA driver

#define FRAME_WIDTH 320
#define FRAME_HEIGHT 240
#define FRAME_SIZE (FRAME_WIDTH * FRAME_HEIGHT * 2)  // Assuming RGB565

// Assuming dest_ip and dest_port are defined elsewhere or passed
uint8_t dest_ip[4] = {192, 168, 1, 100};  // Example IP
uint16_t dest_port = 12345;

int main(void) {
    init_streaming(dest_ip, dest_port);

    // This loop never ends
    streaming_loop();

    return 0;
}