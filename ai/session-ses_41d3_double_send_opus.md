# question_double_send.md implementation

**Session ID:** ses_41d3d06f4ffeRHMSKfvYoPeUN3
**Created:** 1/21/2026, 11:53:07 PM
**Updated:** 1/21/2026, 11:57:38 PM

---

## Assistant (Build · antigravity-claude-opus-4-5-thinking · 160.6s)

---

## User

take the `question_double_send.md`, and do it.

---

## Assistant (Build · antigravity-claude-opus-4-5-thinking · 5.6s)

_Thinking:_

The user wants me to read a file called `question_double_send.md` and then do whatever task is described in it. Let me first read this file to understand what needs to be done.

```
Tool: read

**Input:**
```json
{
  "filePath": "/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/question_double_send.md"
}
```
**Output:**
```
<file>
00001| #Goal
00002| 
00003| Make a detailed action plan, and save it `to action_plan_double_send.md`,
00004| study in details the "Problem found?" chapter, and base your action plan on it.
00005| 
00006| ## Application description
00007| 
00008| 
00009| It is a webpage serving application, where it serves a very big (~53kB) .svg file 
00010| at the /svg endpoint. It is clearly bigger then the TX buffer (16kB).
00011| 
00012| Give extra attention to DMA handling. The w5500 should use DMA. 
00013| 
00014| 
00015| ## Problem Summary
00016| 
00017| First 172.16.1.1/svg is successful, I receive the whole .svg file nothing truncated.
00018| The second call (loading again) fails.
00019| 
00020| The MCU hard locks with no timeout messages or error output.
00021| 
00022| 
00023| # Architecture
00024| 
00025| | Component | Details |
00026| |-----------|---------|
00027| | MCU | RP2350A (Raspberry Pi Pico 2) |
00028| | Ethernet | W5500 via SPI0 @ 20MHz |
00029| | Camera | OV5640 via PIO + DMA |
00030| | Frame size | 240x320 RGB565 = 153.6KB |
00031| | TX buffer | 16KB (Socket 0) |
00032| | RX buffer | 16KB (Socket 0) |
00033| 
00034| # Problem found?
00035| 
00036| **FOUND IT!** Look at lines 668-671:
00037| 
00038| ```c
00039| case TX_TRANSFERRING_CHUNK: {
00040|     // ...
00041|     setSn_CR(transfer->socket_num, Sn_CR_SEND);
00042| 
00043|     // Wait for command to register
00044|     while (getSn_CR(transfer->socket_num)) {
00045|         mpy_wiznet_yield();  // <-- THIS CALLS w5500_tx_service() RECURSIVELY!
00046|     }
00047| 
00048|     transfer->state = TX_SENDING_PACKET;  // State change is AFTER the while loop
00049|     break;
00050| }
00051| ```
00052| 
00053| And `mpy_wiznet_yield()` at line 252-254:
00054| ```c
00055| void mpy_wiznet_yield(void) {
00056|     // ...
00057|     w5500_tx_service();  // <-- RECURSIVE CALL!
00058| }
00059| ```
00060| 
00061| ## The Reentrance Bug
00062| 
00063| **During the first request**, this happens:
00064| 1. `w5500_tx_service()` enters `TX_TRANSFERRING_CHUNK` case
00065| 2. Issues `setSn_CR(sn, Sn_CR_SEND)`
00066| 3. Enters `while (getSn_CR(sn))` loop
00067| 4. Calls `mpy_wiznet_yield()` which calls `w5500_tx_service()` **recursively**
00068| 5. The recursive call sees the **same transfer still in TX_TRANSFERRING_CHUNK state**
00069| 6. It issues **ANOTHER** `setSn_CR(sn, Sn_CR_SEND)` - a double SEND command!
00070| 7. It updates `TX_WR` pointer again - corrupting the W5500's internal pointer tracking!
00071| 
00072| This corrupts the **W5500 chip's internal state** (not MCU state), which persists until power cycle.
00073| 
00074| ## Let me verify the 4 angles you asked for:
00075| 
00076| ### Angle 1: What state persists after 10 minutes?
00077| - **W5500 chip internal state**: The TX buffer pointers are corrupted by the double SEND commands
00078| - The chip's TCP state machine may be in an inconsistent state
00079| - This persists because the W5500 has its own internal CPU/memory
00080| 
00081| ### Angle 2: What runs in parallel with the program?
00082| The W5500 chip runs autonomously. When you issue a `SEND` command, it starts reading from its TX buffer and transmitting. If you issue **two** SEND commands with corrupted `TX_WR` pointers:
00083| - The W5500's internal state machine gets confused
00084| - It might be waiting for something that never happens
00085| - Or its buffer pointers are now invalid
00086| 
00087| ### Angle 3: Why does the second request fail?
00088| The first request "works" because the data still gets sent (even if via corrupted path). But the W5500's internal state is now broken. When the socket is closed and re-opened, the chip's internal structures are not fully reset. The second request encounters this corrupted state.
00089| 
00090| ### Angle 4: Why does RX_RSR=333 show correct data?
00091| The **RX side** of the W5500 is independent from the TX side. The chip can still:
00092| - Accept TCP connections (SYN/SYN-ACK/ACK)
00093| - Receive data into RX buffer
00094| - Report correct RX_RSR
00095| 
00096| But the **SPI communication** or **chip state machine** may hang when the MCU tries to do certain operations after the TX corruption.
00097| 
00098| 

(End of file - total 98 lines)
</file>
```
```

---

## Assistant (Build · antigravity-claude-opus-4-5-thinking · 12.3s)

_Thinking:_

Now I understand the task. I need to:

1. Study the problem described in the file (a reentrance bug in W5500 TX handling)
2. Create a detailed action plan based on the analysis
3. Save it to `action_plan_double_send.md`

The problem is clear:
- When sending large files (>16KB TX buffer), the code has a recursive call issue
- In `TX_TRANSFERRING_CHUNK` state, after issuing `Sn_CR_SEND`, it waits in a while loop calling `mpy_wiznet_yield()`
- `mpy_wiznet_yield()` calls `w5500_tx_service()` recursively
- The recursive call sees the same transfer still in `TX_TRANSFERRING_CHUNK` state
- It issues ANOTHER SEND command, corrupting W5500's internal state
- First request works, second request fails due to corrupted chip state

Let me first look at the actual code to understand the implementation better before creating the action plan.

Let me study the actual code to understand the implementation details before creating the action plan.

```
Tool: grep

**Input:**
```json
{
  "pattern": "TX_TRANSFERRING_CHUNK|w5500_tx_service|mpy_wiznet_yield",
  "include": "*.c"
}
```
**Output:**
```
Found 22 matches
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:
  Line 193:     TX_TRANSFERRING_CHUNK,
  Line 216: static void w5500_tx_service(void);
  Line 245: void mpy_wiznet_yield(void) {
  Line 246:     // Used in socket.c via -DWIZCHIP_YIELD=mpy_wiznet_yield in make/cmake
  Line 254:     w5500_tx_service();
  Line 616: static void w5500_tx_service(void) {
  Line 625:             DEBUG_PRINT("w5500_tx_service: socket %d no longer valid (state=0x%02x), aborting transfer\n",
  Line 640:                     DEBUG_PRINT("w5500_tx_service: socket %d not connected, cancelling transfer\n", transfer->socket_num);
  Line 651:                     transfer->state = TX_TRANSFERRING_CHUNK;
  Line 653:                     DEBUG_PRINT("w5500_tx_service: started DMA for %d bytes on socket %d\n",
  Line 659:             case TX_TRANSFERRING_CHUNK: {
  Line 670:                     mpy_wiznet_yield();
  Line 674:                 DEBUG_PRINT("w5500_tx_service: DMA complete, SEND command issued on socket %d\n",
  Line 685:                     DEBUG_PRINT("w5500_tx_service: SENDOK, %d/%d bytes sent on socket %d\n", 
  Line 703:                     DEBUG_PRINT("w5500_tx_service: TIMEOUT on socket %d\n", transfer->socket_num);
  Line 710:                 DEBUG_PRINT("w5500_tx_service: Transfer complete on socket %d, %d bytes sent\n", 
  Line 717:                 DEBUG_PRINT("w5500_tx_service: Transfer error on socket %d\n", transfer->socket_num);
  Line 735:         w5500_tx_service();
  Line 1536:         mpy_wiznet_yield();
  Line 1639:         // Transfer already dequeued and released by w5500_tx_service
  Line 1645:     // Transfer already dequeued and released by w5500_tx_service
  Line 1888:             mpy_wiznet_yield();
```
```

```
Tool: grep

**Input:**
```json
{
  "include": "*.c",
  "pattern": "setSn_CR.*Sn_CR_SEND"
}
```
**Output:**
```
Found 8 matches
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:
  Line 666:                 setSn_CR(transfer->socket_num, Sn_CR_SEND);

/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/modules/camera/cam.c:
  Line 412:     setSn_CR(stream_socket_num, Sn_CR_SEND);
  Line 527:             setSn_CR(stream_socket_num, Sn_CR_SEND);
  Line 602:         setSn_CR(stream_socket_num, Sn_CR_SEND);

/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/lib/wiznet5k/Ethernet/socket.c:
  Line 366:                setSn_CR(sn,Sn_CR_SEND);
  Line 404:    setSn_CR(sn,Sn_CR_SEND);
  Line 594: 	setSn_CR(sn,Sn_CR_SEND);
  Line 887:             setSn_CR(sn,Sn_CR_SEND_KEEP);
```
```

---

## Assistant (Build · antigravity-claude-opus-4-5-thinking · 7.2s)

_Thinking:_

Let me read the relevant sections of the network_wiznet5k.c file to understand the full context of the bug.

```
Tool: read

**Input:**
```json
{
  "offset": 185,
  "limit": 150,
  "filePath": "/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c"
}
```
**Output:**
```
<file>
00186| // DMA channel for TX - CLAUDE FIX: Initialize to -1 to indicate unclaimed
00187| static int dma_tx_chan = -1;
00188| 
00189| // Transfer state machine for large async transfers
00190| typedef enum {
00191|     TX_IDLE,
00192|     TX_WAITING_BUFFER_SPACE,
00193|     TX_TRANSFERRING_CHUNK,
00194|     TX_SENDING_PACKET,
00195|     TX_COMPLETE,
00196|     TX_ERROR
00197| } tx_state_t;
00198| 
00199| // Zero-copy transfer descriptor
00200| typedef struct w5500_tx_transfer {
00201|     const uint8_t* data_ptr;      // Source data - NO COPY
00202|     uint32_t total_size;          // Total bytes to send
00203|     uint32_t bytes_sent;          // Progress tracking
00204|     uint16_t chunk_size;          // Current chunk size to send
00205|     uint8_t socket_num;           // Target socket
00206|     tx_state_t state;             // Current state
00207|     struct w5500_tx_transfer* next; // For linked list queue
00208| } w5500_tx_transfer_t;
00209| 
00210| // Transfer queue management
00211| static w5500_tx_transfer_t* active_transfers = NULL;
00212| static w5500_tx_transfer_t* transfer_pool = NULL;
00213| static w5500_tx_transfer_t transfer_pool_data[8]; // Support up to 8 concurrent transfers
00214| 
00215| // Forward declarations for transfer functions
00216| static void w5500_tx_service(void);
00217| static void w5500_dma_write_to_txbuf(uint8_t sn, uint16_t offset, const uint8_t* data, uint16_t len);
00218| static uint32_t w5500_drain_rx_buffer(uint8_t sn);
00219| 
00220| #if WIZNET5K_WITH_LWIP_STACK
00221| #define IS_ACTIVE(self) (self->netif.flags & NETIF_FLAG_UP)
00222| #else // WIZNET5K_PROVIDED_STACK
00223| #define IS_ACTIVE(self) (self->active)
00224| #endif
00225| 
00226| // Global object holding the Wiznet5k state
00227| static wiznet5k_obj_t wiznet5k_obj;
00228| 
00229| static void wiz_cris_enter(void) {
00230|     wiznet5k_obj.cris_state = MICROPY_BEGIN_ATOMIC_SECTION();
00231| }
00232| 
00233| static void wiz_cris_exit(void) {
00234|     MICROPY_END_ATOMIC_SECTION(wiznet5k_obj.cris_state);
00235| }
00236| 
00237| static void wiz_cs_select(void) {
00238|     mp_hal_pin_low(wiznet5k_obj.cs);
00239| }
00240| 
00241| static void wiz_cs_deselect(void) {
00242|     mp_hal_pin_high(wiznet5k_obj.cs);
00243| }
00244| 
00245| void mpy_wiznet_yield(void) {
00246|     // Used in socket.c via -DWIZCHIP_YIELD=mpy_wiznet_yield in make/cmake
00247|     #if MICROPY_PY_THREAD
00248|     MICROPY_THREAD_YIELD();
00249|     #else
00250|     mp_handle_pending(true);
00251|     #endif
00252|     
00253|     // Service zero-copy transfers
00254|     w5500_tx_service();
00255| }
00256| 
00257| static void wiz_spi_read(uint8_t *buf, uint16_t len) {
00258|     wiznet5k_obj.spi_transfer(wiznet5k_obj.spi, len, buf, buf);
00259| }
00260| 
00261| static void wiz_spi_writeburst(const uint8_t* pBuf, uint16_t len) {
00262|     if (len >= 1024 && dma_tx_chan >= 0) {
00263|         // printf("DMA3 used for %d bytes\n", len);
00264|         machine_spi_obj_t *spi_obj = (machine_spi_obj_t *)wiznet5k_obj.spi;
00265|         spi_inst_t *spi_inst = spi_obj->spi_inst;
00266| 
00267|         // Claim RX channel to drain SPI RX FIFO
00268|         int dma_rx_chan = dma_claim_unused_channel(true);
00269|         if (dma_rx_chan < 0) {
00270|             // Fallback to normal SPI if no RX channel available
00271|             wiznet5k_obj.spi_transfer(wiznet5k_obj.spi, len, pBuf, NULL);
00272|             return;
00273|         }
00274| 
00275|         uint8_t dev_null;  // Dummy variable for RX data
00276| 
00277|         // Configure TX channel
00278|         dma_channel_config c = dma_channel_get_default_config(dma_tx_chan);
00279|         channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
00280|         channel_config_set_read_increment(&c, true);
00281|         channel_config_set_write_increment(&c, false);
00282|         channel_config_set_dreq(&c, spi_get_dreq(spi_inst, true));
00283|         dma_channel_configure(dma_tx_chan, &c, &spi_get_hw(spi_inst)->dr, pBuf, len, false);
00284| 
00285|         // Configure RX channel to drain RX FIFO
00286|         c = dma_channel_get_default_config(dma_rx_chan);
00287|         channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
00288|         channel_config_set_read_increment(&c, false);
00289|         channel_config_set_write_increment(&c, false);
00290|         channel_config_set_dreq(&c, spi_get_dreq(spi_inst, false));
00291|         dma_channel_configure(dma_rx_chan, &c, &dev_null, &spi_get_hw(spi_inst)->dr, len, false);
00292| 
00293|         // Enable SPI DMA for transfer
00294|         spi_get_hw(spi_obj->spi_inst)->dmacr = (1 << 1) | (1 << 0);
00295|         __compiler_memory_barrier();
00296|         dma_start_channel_mask((1u << dma_rx_chan) | (1u << dma_tx_chan));
00297|         dma_channel_wait_for_finish_blocking(dma_rx_chan);
00298|         dma_channel_wait_for_finish_blocking(dma_tx_chan);
00299|         __compiler_memory_barrier();
00300|         // Disable SPI DMA
00301|         spi_get_hw(spi_obj->spi_inst)->dmacr = 0;
00302| 
00303|         dma_channel_unclaim(dma_rx_chan);
00304|         // printf("DMA is finished\n", len);
00305|     } else {
00306|         wiznet5k_obj.spi_transfer(wiznet5k_obj.spi, len, pBuf, NULL);
00307|     }
00308| }
00309| 
00310| static uint8_t wiz_spi_readbyte() {
00311|     uint8_t buf = 0;
00312|     wiznet5k_obj.spi_transfer(wiznet5k_obj.spi, 1, &buf, &buf);
00313|     return buf;
00314| }
00315| 
00316| static void wiz_spi_writebyte(const uint8_t buf) {
00317|     wiznet5k_obj.spi_transfer(wiznet5k_obj.spi, 1, &buf, NULL);
00318| }
00319| 
00320| static void wiznet5k_get_mac_address(wiznet5k_obj_t *self, uint8_t mac[6]) {
00321|     (void)self;
00322|     getSHAR(mac);
00323| }
00324| 
00325| // Initialize transfer pool only (DMA channel claimed earlier)
00326| static void w5500_tx_pool_init(void) {
00327|     // Initialize transfer pool
00328|     for (int i = 0; i < 8; i++) {
00329|         transfer_pool_data[i].next = transfer_pool;
00330|         transfer_pool = &transfer_pool_data[i];
00331|     }
00332|     DEBUG_PRINT("w5500_tx_pool_init: initialized transfer pool\n");
00333| }
00334| 
00335| // Claim DMA channel early, before any SPI operations

(File has more lines. Use 'offset' parameter to read beyond line 335)
</file>
```
```

---

## Assistant (Build · antigravity-claude-opus-4-5-thinking · 7.8s)

_Thinking:_

Now let me read the actual TX_TRANSFERRING_CHUNK case and surrounding code around lines 616-720 to understand the full state machine.

```
Tool: read

**Input:**
```json
{
  "filePath": "/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c",
  "limit": 130,
  "offset": 610
}
```
**Output:**
```
<file>
00611|     return 0;
00612| }
00613| */
00614| 
00615| // Background transfer service - called periodically
00616| static void w5500_tx_service(void) {
00617|     w5500_tx_transfer_t* transfer = active_transfers;
00618| 
00619|     while (transfer) {
00620|         w5500_tx_transfer_t* next = transfer->next; // Save next pointer in case transfer is removed
00621| 
00622|         // NEW: Validate socket is still in a valid state for transfers
00623|         uint8_t sr = getSn_SR(transfer->socket_num);
00624|         if (sr != SOCK_ESTABLISHED && sr != SOCK_CLOSE_WAIT) {
00625|             DEBUG_PRINT("w5500_tx_service: socket %d no longer valid (state=0x%02x), aborting transfer\n",
00626|                         transfer->socket_num, sr);
00627|             transfer->state = TX_ERROR;
00628|             w5500_dequeue_transfer(transfer);
00629|             w5500_release_transfer(transfer);
00630|             transfer = next;
00631|             continue;
00632|         }
00633|         
00634|         switch (transfer->state) {
00635|             case TX_WAITING_BUFFER_SPACE: {
00636|                 uint16_t free_space = getSn_TX_FSR(transfer->socket_num);
00637|                 uint8_t sr = getSn_SR(transfer->socket_num);
00638| 
00639|                 if ((sr != SOCK_ESTABLISHED) && (sr != SOCK_CLOSE_WAIT)) {
00640|                     DEBUG_PRINT("w5500_tx_service: socket %d not connected, cancelling transfer\n", transfer->socket_num);
00641|                     transfer->state = TX_ERROR;
00642|                     break;
00643|                 }
00644| 
00645|                 if (free_space >= transfer->chunk_size) {
00646|                     // Start DMA transfer
00647|                     uint16_t tx_wr = getSn_TX_WR(transfer->socket_num);
00648|                     const uint8_t* data_ptr = transfer->data_ptr + transfer->bytes_sent;
00649| 
00650|                     w5500_send_chunk(transfer->socket_num, data_ptr, transfer->chunk_size, tx_wr);
00651|                     transfer->state = TX_TRANSFERRING_CHUNK;
00652| 
00653|                     DEBUG_PRINT("w5500_tx_service: started DMA for %d bytes on socket %d\n",
00654|                                 transfer->chunk_size, transfer->socket_num);
00655|                 }
00656|                 break;
00657|             }
00658| 
00659|             case TX_TRANSFERRING_CHUNK: {
00660|                 // Transfer is now synchronous, so we can immediately proceed
00661|                 // DMA is completed before w5500_send_chunk returns
00662| 
00663|                 // Update write pointer and send command
00664|                 uint16_t new_tx_wr = getSn_TX_WR(transfer->socket_num) + transfer->chunk_size;
00665|                 setSn_TX_WR(transfer->socket_num, new_tx_wr);
00666|                 setSn_CR(transfer->socket_num, Sn_CR_SEND);
00667| 
00668|                 // Wait for command to register
00669|                 while (getSn_CR(transfer->socket_num)) {
00670|                     mpy_wiznet_yield();
00671|                 }
00672| 
00673|                 transfer->state = TX_SENDING_PACKET;
00674|                 DEBUG_PRINT("w5500_tx_service: DMA complete, SEND command issued on socket %d\n",
00675|                             transfer->socket_num);
00676|                 break;
00677|             }
00678| 
00679|             case TX_SENDING_PACKET: {
00680|                 uint8_t ir = getSn_IR(transfer->socket_num);
00681|                 if (ir & Sn_IR_SENDOK) {
00682|                     setSn_IR(transfer->socket_num, Sn_IR_SENDOK);
00683|                     
00684|                     transfer->bytes_sent += transfer->chunk_size;
00685|                     DEBUG_PRINT("w5500_tx_service: SENDOK, %d/%d bytes sent on socket %d\n", 
00686|                                 transfer->bytes_sent, transfer->total_size, transfer->socket_num);
00687|                     
00688|                     if (transfer->bytes_sent >= transfer->total_size) {
00689|                         transfer->state = TX_COMPLETE;
00690|                     } else {
00691|                         // Calculate next chunk size
00692|                         uint16_t remaining = transfer->total_size - transfer->bytes_sent;
00693|                         uint16_t buffer_free = getSn_TX_FSR(transfer->socket_num);
00694|                         transfer->chunk_size = (remaining < buffer_free) ? remaining : buffer_free;
00695|                         // Cap chunk size to buffer size
00696|                         uint16_t max_chunk = getSn_TxMAX(transfer->socket_num);
00697|                         if (transfer->chunk_size > max_chunk) {
00698|                             transfer->chunk_size = max_chunk;
00699|                         }
00700|                         transfer->state = TX_WAITING_BUFFER_SPACE;
00701|                     }
00702|                 } else if (ir & Sn_IR_TIMEOUT) {
00703|                     DEBUG_PRINT("w5500_tx_service: TIMEOUT on socket %d\n", transfer->socket_num);
00704|                     transfer->state = TX_ERROR;
00705|                 }
00706|                 break;
00707|             }
00708|             
00709|             case TX_COMPLETE:
00710|                 DEBUG_PRINT("w5500_tx_service: Transfer complete on socket %d, %d bytes sent\n", 
00711|                             transfer->socket_num, transfer->bytes_sent);
00712|                 w5500_dequeue_transfer(transfer);
00713|                 w5500_release_transfer(transfer);
00714|                 break;
00715|                 
00716|             case TX_ERROR:
00717|                 DEBUG_PRINT("w5500_tx_service: Transfer error on socket %d\n", transfer->socket_num);
00718|                 w5500_dequeue_transfer(transfer);
00719|                 w5500_release_transfer(transfer);
00720|                 break;
00721|                 
00722|             default:
00723|                 break;
00724|         }
00725|         
00726|         transfer = next;
00727|     }
00728| }
00729| 
00730| // CLAUDE FIX: Helper function to wait for a specific transfer to complete
00731| static int w5500_wait_transfer_complete(w5500_tx_transfer_t* transfer, uint32_t timeout_ms) {
00732|     uint32_t start = mp_hal_ticks_ms();
00733| 
00734|     while (transfer->state != TX_COMPLETE && transfer->state != TX_ERROR) {
00735|         w5500_tx_service();
00736| 
00737|         if (timeout_ms > 0 && (mp_hal_ticks_ms() - start) >= timeout_ms) {
00738|             DEBUG_PRINT("w5500_wait_transfer_complete: timeout\n");
00739|             return -1;  // Timeout
00740|         }

(File has more lines. Use 'offset' parameter to read beyond line 740)
</file>
```
```

---

## Assistant (Build · antigravity-claude-opus-4-5-thinking · 41.9s)

Now I have a complete understanding of the bug. Let me create a detailed action plan.

```
Tool: write

**Input:**
```json
{
  "filePath": "/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/action_plan_double_send.md",
  "content": "# Action Plan: Fix W5500 Reentrant Double-SEND Bug\n\n## Problem Summary\n\nThe `w5500_tx_service()` function is called recursively during the `TX_TRANSFERRING_CHUNK` state, causing:\n\n1. **Double SEND commands** to the W5500 chip\n2. **Corrupted TX_WR pointer** tracking\n3. **W5500 chip internal state corruption** that persists until power cycle\n4. **MCU hard lock** on the second HTTP request\n\n### Root Cause Location\n\nFile: `micropython/extmod/network_wiznet5k.c`\n\n- Lines 659-677: `TX_TRANSFERRING_CHUNK` case\n- Lines 668-671: The problematic `while` loop with `mpy_wiznet_yield()`\n- Lines 245-255: `mpy_wiznet_yield()` calls `w5500_tx_service()` recursively\n\n### The Bug Flow\n\n```\nw5500_tx_service() enters TX_TRANSFERRING_CHUNK\n    -> setSn_CR(sn, Sn_CR_SEND)                 // First SEND command\n    -> while (getSn_CR(sn)) loop\n        -> mpy_wiznet_yield()\n            -> w5500_tx_service()              // RECURSIVE CALL\n                -> sees same transfer in TX_TRANSFERRING_CHUNK\n                -> setSn_CR(sn, Sn_CR_SEND)    // SECOND SEND command!\n                -> updates TX_WR again         // POINTER CORRUPTION!\n```\n\n---\n\n## Fix Options\n\n### Option A: Add Reentrance Guard (Recommended)\n\nAdd a static flag to prevent recursive calls from processing the same transfer.\n\n**Pros:**\n- Minimal code change\n- Low risk of side effects\n- Preserves existing architecture\n\n**Cons:**\n- Doesn't fix the root design issue\n\n### Option B: Change State Before Yield\n\nMove the state transition to `TX_SENDING_PACKET` **before** entering the while loop.\n\n**Pros:**\n- Fixes root cause\n- Clean solution\n\n**Cons:**\n- Must ensure state semantics are correct\n\n### Option C: Don't Call w5500_tx_service() in mpy_wiznet_yield() During Active Transfer\n\nAdd a flag to skip the service call when already in a transfer operation.\n\n---\n\n## Recommended Fix: Option A + B Combined\n\nUse a reentrance guard AND move the state transition earlier.\n\n---\n\n## Detailed Implementation Steps\n\n### Step 1: Add Reentrance Guard Variable\n\nLocation: `network_wiznet5k.c` near line 211 (after transfer queue variables)\n\n```c\n// Reentrance guard for w5500_tx_service\nstatic volatile bool tx_service_active = false;\n```\n\n### Step 2: Modify w5500_tx_service() Entry\n\nLocation: `network_wiznet5k.c` at line 616\n\nAdd at the start of `w5500_tx_service()`:\n\n```c\nstatic void w5500_tx_service(void) {\n    // Prevent reentrant calls that cause double-SEND corruption\n    if (tx_service_active) {\n        return;\n    }\n    tx_service_active = true;\n    \n    // ... existing code ...\n```\n\n### Step 3: Add Guard Reset at All Exit Points\n\nAt the end of `w5500_tx_service()` (before the closing brace at line 728):\n\n```c\n    tx_service_active = false;\n}\n```\n\n### Step 4: Move State Transition in TX_TRANSFERRING_CHUNK\n\nLocation: Lines 659-677\n\nChange from:\n\n```c\ncase TX_TRANSFERRING_CHUNK: {\n    uint16_t new_tx_wr = getSn_TX_WR(transfer->socket_num) + transfer->chunk_size;\n    setSn_TX_WR(transfer->socket_num, new_tx_wr);\n    setSn_CR(transfer->socket_num, Sn_CR_SEND);\n\n    // Wait for command to register\n    while (getSn_CR(transfer->socket_num)) {\n        mpy_wiznet_yield();\n    }\n\n    transfer->state = TX_SENDING_PACKET;  // State change AFTER loop\n    // ...\n}\n```\n\nTo:\n\n```c\ncase TX_TRANSFERRING_CHUNK: {\n    uint16_t new_tx_wr = getSn_TX_WR(transfer->socket_num) + transfer->chunk_size;\n    setSn_TX_WR(transfer->socket_num, new_tx_wr);\n    setSn_CR(transfer->socket_num, Sn_CR_SEND);\n\n    // CRITICAL: Change state BEFORE yield to prevent double-SEND on reentry\n    transfer->state = TX_SENDING_PACKET;\n\n    // Wait for command to register\n    while (getSn_CR(transfer->socket_num)) {\n        mpy_wiznet_yield();\n    }\n\n    DEBUG_PRINT(\"w5500_tx_service: DMA complete, SEND command issued on socket %d\\n\",\n                transfer->socket_num);\n    break;\n}\n```\n\n---\n\n## Code Diff Summary\n\n### File: `micropython/extmod/network_wiznet5k.c`\n\n#### Change 1: Add reentrance guard (around line 211-213)\n\n```diff\n // Transfer queue management\n static w5500_tx_transfer_t* active_transfers = NULL;\n static w5500_tx_transfer_t* transfer_pool = NULL;\n static w5500_tx_transfer_t transfer_pool_data[8];\n+\n+// Reentrance guard to prevent double-SEND bug\n+static volatile bool tx_service_active = false;\n```\n\n#### Change 2: Guard at function entry (line 616-617)\n\n```diff\n static void w5500_tx_service(void) {\n+    // Prevent reentrant calls that cause W5500 double-SEND corruption\n+    if (tx_service_active) {\n+        return;\n+    }\n+    tx_service_active = true;\n+\n     w5500_tx_transfer_t* transfer = active_transfers;\n```\n\n#### Change 3: Guard reset at function exit (line 727-728)\n\n```diff\n         transfer = next;\n     }\n+\n+    tx_service_active = false;\n }\n```\n\n#### Change 4: Move state transition earlier (lines 666-673)\n\n```diff\n             case TX_TRANSFERRING_CHUNK: {\n                 uint16_t new_tx_wr = getSn_TX_WR(transfer->socket_num) + transfer->chunk_size;\n                 setSn_TX_WR(transfer->socket_num, new_tx_wr);\n                 setSn_CR(transfer->socket_num, Sn_CR_SEND);\n\n+                // CRITICAL: Change state BEFORE yield to prevent double-SEND on reentry\n+                transfer->state = TX_SENDING_PACKET;\n+\n                 // Wait for command to register\n                 while (getSn_CR(transfer->socket_num)) {\n                     mpy_wiznet_yield();\n                 }\n\n-                transfer->state = TX_SENDING_PACKET;\n                 DEBUG_PRINT(\"w5500_tx_service: DMA complete, SEND command issued on socket %d\\n\",\n                             transfer->socket_num);\n                 break;\n             }\n```\n\n---\n\n## Testing Plan\n\n1. **First request test**: Load `/svg` endpoint - should complete successfully\n2. **Second request test**: Reload `/svg` endpoint - should now work (previously caused hard lock)\n3. **Stress test**: Rapid multiple reloads of the `/svg` endpoint\n4. **Timeout test**: Verify timeout handling still works correctly\n5. **Socket close test**: Close socket mid-transfer and verify clean recovery\n\n---\n\n## Risk Assessment\n\n| Risk | Likelihood | Mitigation |\n|------|------------|------------|\n| Reentrance guard causes deadlock | Low | Guard is reset at function exit; no blocking operations after guard set |\n| State transition timing issue | Low | TX_SENDING_PACKET is the correct next state; just moving it earlier |\n| Other callers affected | Low | Only `mpy_wiznet_yield()` and `w5500_wait_transfer_complete()` call this function |\n\n---\n\n## Verification Checklist\n\n- [ ] Reentrance guard variable added\n- [ ] Guard check added at function entry\n- [ ] Guard reset added at function exit\n- [ ] State transition moved before while loop\n- [ ] First /svg request works\n- [ ] Second /svg request works\n- [ ] No timeout or error messages in debug output\n- [ ] DMA transfers complete correctly\n"
}
```
**Output:**
```
Wrote file successfully.

LSP errors detected in other files:
<diagnostics file="/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c">
ERROR [56:10] 'py/runtime.h' file not found
ERROR [85:5] Unknown type name 'mp_obj_base_t'
ERROR [86:5] Unknown type name 'spi_inst_t'
ERROR [92:10] 'shared/netutils/netutils.h' file not found
</diagnostics>
```
```

---

## Assistant (Build · antigravity-claude-opus-4-5-thinking · 8.7s)

Created the action plan at `action_plan_double_send.md`.

The plan details:

1. **Root cause**: `mpy_wiznet_yield()` calls `w5500_tx_service()` recursively while still in `TX_TRANSFERRING_CHUNK` state, causing double SEND commands to the W5500 chip

2. **Fix approach** (two-part):
   - Add a **reentrance guard** (`tx_service_active` flag) to prevent recursive calls
   - Move the **state transition** to `TX_SENDING_PACKET` **before** the while loop that calls `mpy_wiznet_yield()`

3. **4 specific code changes** with exact line numbers and diffs

4. **Testing plan** and **risk assessment**

The LSP errors shown are unrelated - they're due to MicroPython header dependencies not being resolved in the IDE, not issues with the action plan file.

---

