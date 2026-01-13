## Detailed Step-by-Step Action Plan to Fix W5500 Driver Issues

Based on the analysis, the core issues are in socket management for servers, bugs in accept logic, and deviations from the W5500 hardware model. Since the requirement is to support **only one client** (for maximum throughput, leveraging the 16kB RX/TX buffers), we can simplify the implementation to avoid multi-client complexities. The plan focuses on aligning with the architecture (no lwIP, direct register/SPI access, polling for state changes) while fixing bugs.

### Prerequisites
- Understand the W5500 model: No `accept()` syscall; `listen()` sets the socket to LISTEN state; acceptance happens asynchronously, transitioning to ESTABLISHED with interrupt.
- Target: `WIZNET5K_PROVIDED_STACK` mode in `network_wiznet5k.c`.
- Testing: After changes, run lint/format (`tools/codeformat.py`, `ruff check .`, `ruff format .`), and test with a single TCP client connection.
- No builds: Per project policy, do not attempt to build/compile.

### Step 1: Simplify Socket Management for Single-Client Server
**Rationale**: The current code tries to recreate listening sockets for multi-client support, which is buggy and unnecessary for one client. For one client, use the listening socket directly for the connection (as per W5500: listening socket becomes connected).

- **Action 1.1**: In `wiznet5k_socket_accept()`, remove the logic to create a new listening socket (lines ~734-746). Instead, after detecting ESTABLISHED, simply mark the socket as connected and return it. Do not call `wiznet5k_socket_socket()`, `wiznet5k_socket_bind()`, or `wiznet5k_socket_listen()` again.
- **Action 1.2**: Update the socket state tracking: After accept, set the socket's state to connected (e.g., via a flag in `mod_network_socket_obj_t` or by checking `getSn_SR(sn) == SOCK_ESTABLISHED`).
- **Action 1.3**: Ensure the server socket cannot accept again after one connection. In `wiznet5k_socket_accept()`, if the socket is already connected or closed, return an error (e.g., `MP_ENOTCONN`).
- **Action 1.4**: In `wiznet5k_socket_close()`, ensure proper cleanup, but since only one client, no need to manage multiple sockets.

### Step 2: Fix Non-Blocking Accept Bug
**Rationale**: For `timeout == 0`, the loop runs indefinitely instead of checking once and returning `MP_EAGAIN`.

- **Action 2.1**: In `wiznet5k_socket_accept()` (around lines 675-690), add a check before the loop: If `socket->timeout == 0` and no connection pending (i.e., `getSn_IR(sn) & Sn_IR_CON` is false and `getSn_SR(sn) != SOCK_ESTABLISHED`), immediately return `MP_EAGAIN`.
- **Action 2.2**: Ensure the loop only runs if `timeout != 0` or for blocking mode (`timeout == -1`).

### Step 3: Improve Timeout Handling in Accept
**Rationale**: The current timeout logic may overflow for long blocking waits and doesn't handle non-blocking properly.

- **Action 3.1**: For blocking (`timeout == -1`), set `timeout` to a large value (e.g., `UINT32_MAX / 2`) instead of `0xFFFFFFFF` to avoid potential overflow.
- **Action 3.2**: Use `mp_hal_ticks_ms()` consistently, and calculate elapsed time inside the loop for accuracy.
- **Action 3.3**: Add a small yield (`mpy_wiznet_yield()`) in the loop to prevent busy-waiting, especially for blocking accepts.

### Step 4: Fix Error Handling and Propagation
**Rationale**: Failures in socket recreation (now removed) and other ops are logged but not propagated, leading to inconsistent states.

- **Action 4.1**: In `wiznet5k_socket_accept()`, ensure all error paths set `*errno` and return -1. For example, if the socket is not in LISTEN or already connected, return `MP_EINVAL` or `MP_ENOTCONN`.
- **Action 4.2**: In `wiznet5k_socket_listen()`, add checks: If the socket is already connected, return an error.
- **Action 4.3**: In `wiznet5k_socket_bind()`, ensure it fails gracefully if the socket is already bound or in an invalid state.

### Step 5: Clean Up Debug and Performance Issues
**Rationale**: Debug prints are always on, causing overhead.

- **Action 5.1**: Keep `W5500_DEBUG`, debugging should be extensive in this implementation. 
- **Action 5.2**: Reduce polling frequency in `wiznet5k_socket_accept()`: Increase the `mp_hal_delay_ms(10)` to `mp_hal_delay_ms(50)` or more to reduce CPU usage, as 10ms is aggressive.
- **Action 5.3**: In `wiznet5k_socket_recv()`, ensure the delay is consistent (currently 10ms).

### Step 6: Align with Architecture for RX/TX Data Flow
**Rationale**: The data flow is mostly correct, but ensure no assumptions about headers or packet boundaries.

- **Action 6.1**: Confirm in `wiznet5k_socket_recv()` and `wiznet5k_socket_send()` that they treat data as a byte stream (no message boundaries).
- **Action 6.2**: Ensure `WIZCHIP_EXPORT(recv)` and `WIZCHIP_EXPORT(send)` handle buffer pointers correctly (they do, per library).
- **Action 6.3**: In `wiznet5k_socket_recv()`, after reading data, verify `RECV` command is issued (via `WIZCHIP_EXPORT(recv)`).

### Step 7: Update Socket State Tracking
**Rationale**: The code uses `socket->domain` as a flag for "not opened", but it's hacky.

- **Action 7.1**: Add a proper state field to `mod_network_socket_obj_t` (if possible) or use `getSn_SR(sn)` for state checks.
- **Action 7.2**: In `wiznet5k_socket_socket()`, initialize state properly.
- **Action 7.3**: Ensure `wiznet5k_socket_close()` resets state and marks the socket as unused.

### Step 8: Testing and Validation Plan
**Rationale**: Ensure changes work for single-client TCP server.

- **Action 8.1**: Test `socket()`, `bind()`, `listen()`, `accept()` sequence with a client connecting. Verify only one connection is handled.
- **Action 8.2**: Test non-blocking `accept()`: Should return `MP_EAGAIN` if no client.
- **Action 8.3**: Test `recv()` and `send()` for byte-stream data transfer.
- **Action 8.4**: Test `close()` and reopening the socket.
- **Action 8.5**: Run lint/format checks post-changes.
- **Action 8.6**: If issues, check register dumps (`wiznet5k_regs()`) to verify W5500 state.

### Step 9: Documentation Updates
**Rationale**: Ensure the code reflects the single-client design.

- **Action 9.1**: Add comments in `wiznet5k_socket_accept()` explaining it's for single-client only.
- **Action 9.2**: Update any TODOs or comments about multi-client support to note it's not implemented.

This plan fixes the bugs, simplifies for single-client use, and aligns with the architecture while maintaining the no-lwIP, SPI-based flow. Total estimated changes: ~50-100 lines modified/removed, focused on `wiznet5k_socket_accept()` and related functions.



