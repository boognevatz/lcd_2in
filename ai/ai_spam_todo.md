Done - Fixed spammy debug prints in socket_recv()

Added loop_counter to reduce debug output from every loop iteration to every 100 iterations (~1 second).

Changes made to micropython/extmod/network_wiznet5k.c:
- Line 883: Added int loop_counter = 0;
- Lines 888-890: Wrapped RX_RSR debug print with if (loop_counter % 100 == 0)
- Lines 915-918: Wrapped socket state debug prints with if (loop_counter % 100 == 0)
- Line 943: Added loop_counter++ increment
