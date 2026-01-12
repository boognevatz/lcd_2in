
## The Problem:

The `wiznet5k_socket_accept()` function only checks `getSn_SR()` (socket status register) but doesn't:
1. Check the interrupt register (`Sn_IR`) for connection pending
2. Send the `ACCEPT` command to the W5500
3. Handle the socket state transition properly

## The Fix - Update `wiznet5k_socket_accept()`:

Here's the corrected version with minimal changes (keeping your debug prints):

```c
static int wiznet5k_socket_accept(mod_network_socket_obj_t *socket, mod_network_socket_obj_t *socket2, byte *ip, mp_uint_t *port, int *_errno) {
    uint8_t sn = (uint8_t)socket->fileno;
    DEBUG_PRINT("socket_accept: socket=%d, timeout=%d\n", sn, socket->timeout);
    
    // Check initial socket state
    uint8_t sr_initial = getSn_SR(sn);
    DEBUG_PRINT("socket_accept: initial state - socket %d: 0x%02x (%s)\n",
                sn, sr_initial, socket_state_str(sr_initial));
    
    uint32_t start_time = mp_hal_ticks_ms();
    uint32_t timeout = socket->timeout;
    if (timeout == -1) {
        timeout = 0xFFFFFFFF; // Wait forever
    }
    
    int loop_counter = 0;
    
    for (;;) {
        loop_counter++;
        
        // Check for timeout
        if (timeout != 0xFFFFFFFF && (mp_hal_ticks_ms() - start_time) > timeout) {
            DEBUG_PRINT("socket_accept: timeout after %d ms\n", timeout);
            *_errno = MP_ETIMEDOUT;
            return -1;
        }
        
        // 1. First check interrupt register for connection pending
        uint8_t ir = getSn_IR(sn);
        DEBUG_PRINT("socket_accept: loop %d - socket %d IR: 0x%02x, SR: 0x%02x (%s)\n",
                    loop_counter, sn, ir, getSn_SR(sn), socket_state_str(getSn_SR(sn)));
        
        if (ir & Sn_IR_CON) {
            DEBUG_PRINT("socket_accept: CONNECT interrupt detected! Sending ACCEPT command...\n");
            
            // Clear the CON interrupt by writing 1 to it
            setSn_IR(sn, Sn_IR_CON);
            
            // Send ACCEPT command to W5500
            setSn_CR(sn, Sn_CR_ACCEPT);
            
            // Wait for command to complete
            int cmd_timeout = 100; // 100ms max
            while (getSn_CR(sn) != 0 && cmd_timeout-- > 0) {
                mp_hal_delay_ms(1);
            }
            
            if (getSn_CR(sn) != 0) {
                DEBUG_PRINT("socket_accept: ACCEPT command timeout\n");
                *_errno = MP_ETIMEDOUT;
                return -1;
            }
            
            DEBUG_PRINT("socket_accept: ACCEPT command sent, checking new state...\n");
            
            // Give it a moment to transition
            mp_hal_delay_ms(10);
            
            // Check new state
            uint8_t new_sr = getSn_SR(sn);
            DEBUG_PRINT("socket_accept: new state: 0x%02x (%s)\n",
                       new_sr, socket_state_str(new_sr));
            
            if (new_sr == SOCK_ESTABLISHED) {
                DEBUG_PRINT("socket_accept: socket ESTABLISHED!\n");
                
                // Get client info
                getSn_DIPR(sn, ip);
                *port = getSn_PORT(sn);
                
                DEBUG_PRINT("socket_accept: client IP: %d.%d.%d.%d, port: %d\n",
                           ip[0], ip[1], ip[2], ip[3], *port);
                
                // socket2 gets the connected socket
                socket2->domain = socket->domain;
                socket2->type = socket->type;
                socket2->fileno = sn;
                
                // WIZnet turns the listening socket into the client socket, so we
                // need to re-bind and re-listen on another socket for the server.
                DEBUG_PRINT("socket_accept: creating new listening socket...\n");
                
                // TODO handle errors, especially no-more-sockets error
                socket->domain = MOD_NETWORK_AF_INET;
                socket->fileno = -1;
                int _errno2;
                if (wiznet5k_socket_socket(socket, &_errno2) != 0) {
                    DEBUG_PRINT("socket_accept: failed to create new socket: %d\n", _errno2);
                } else if (wiznet5k_socket_bind(socket, NULL, *port, &_errno2) != 0) {
                    DEBUG_PRINT("socket_accept: failed to bind new socket: %d\n", _errno2);
                } else if (wiznet5k_socket_listen(socket, 0, &_errno2) != 0) {
                    DEBUG_PRINT("socket_accept: failed to listen on new socket: %d\n", _errno2);
                } else {
                    DEBUG_PRINT("socket_accept: new listening socket created successfully\n");
                }

                DEBUG_PRINT("socket_accept: success\n");
                return 0;
            } else {
                DEBUG_PRINT("socket_accept: socket not in ESTABLISHED state after ACCEPT\n");
                // Continue waiting
            }
        }
        
        // Also check if socket somehow became ESTABLISHED without interrupt
        uint8_t sr = getSn_SR(sn);
        if (sr == SOCK_ESTABLISHED) {
            DEBUG_PRINT("socket_accept: socket already ESTABLISHED (unexpected)!\n");
            
            // Same handling as above...
            getSn_DIPR(sn, ip);
            *port = getSn_PORT(sn);
            
            DEBUG_PRINT("socket_accept: client IP: %d.%d.%d.%d, port: %d\n",
                       ip[0], ip[1], ip[2], ip[3], *port);
            
            socket2->domain = socket->domain;
            socket2->type = socket->type;
            socket2->fileno = sn;
            
            socket->domain = MOD_NETWORK_AF_INET;
            socket->fileno = -1;
            int _errno2;
            if (wiznet5k_socket_socket(socket, &_errno2) != 0) {
                DEBUG_PRINT("socket_accept: failed to create new socket: %d\n", _errno2);
            } else if (wiznet5k_socket_bind(socket, NULL, *port, &_errno2) != 0) {
                DEBUG_PRINT("socket_accept: failed to bind new socket: %d\n", _errno2);
            } else if (wiznet5k_socket_listen(socket, 0, &_errno2) != 0) {
                DEBUG_PRINT("socket_accept: failed to listen on new socket: %d\n", _errno2);
            } else {
                DEBUG_PRINT("socket_accept: new listening socket created successfully\n");
            }

            DEBUG_PRINT("socket_accept: success\n");
            return 0;
        }
        
        if (sr == SOCK_CLOSED || sr == SOCK_CLOSE_WAIT) {
            DEBUG_PRINT("socket_accept: socket closed or close_wait, closing\n");
            wiznet5k_socket_close(socket);
            *_errno = MP_ENOTCONN;
            return -1;
        }
        
        // Only print every 100 loops to reduce noise
        if (loop_counter % 100 == 0) {
            DEBUG_PRINT("socket_accept: still waiting... (loop %d)\n", loop_counter);
        }
        
        mp_hal_delay_ms(10); // Wait 10ms instead of 1ms to reduce CPU usage
    }
}
```

## Also Fix `wiznet5k_socket_ioctl()` for Polling:

Update to check for connection interrupts:

```c
static int wiznet5k_socket_ioctl(mod_network_socket_obj_t *socket, mp_uint_t request, mp_uint_t arg, int *_errno) {
    DEBUG_PRINT("socket_ioctl: request=0x%x, arg=0x%x, socket=%d\n", 
                request, arg, socket->fileno);
    
    if (request == MP_STREAM_POLL) {
        int ret = 0;
        uint8_t sn = (uint8_t)socket->fileno;
        uint8_t sr = getSn_SR(sn);
        
        DEBUG_PRINT("socket_ioctl: socket %d - state=0x%02x (%s)\n",
                   sn, sr, socket_state_str(sr));
        
        if (arg & MP_STREAM_POLL_RD) {
            if (sr == SOCK_LISTEN) {
                // For listening sockets, check interrupt register
                uint8_t ir = getSn_IR(sn);
                DEBUG_PRINT("socket_ioctl: listening socket IR=0x%02x\n", ir);
                if (ir & Sn_IR_CON) {
                    ret |= MP_STREAM_POLL_RD;
                    DEBUG_PRINT("socket_ioctl: connection pending!\n");
                }
            } else if (sr == SOCK_ESTABLISHED || sr == SOCK_CLOSE_WAIT) {
                uint16_t rx_rsr = getSn_RX_RSR(sn);
                if (rx_rsr > 0) {
                    ret |= MP_STREAM_POLL_RD;
                    DEBUG_PRINT("socket_ioctl: data available (RX_RSR=%d)\n", rx_rsr);
                }
            }
        }
        
        if (arg & MP_STREAM_POLL_WR) {
            if (sr == SOCK_ESTABLISHED || sr == SOCK_CLOSE_WAIT) {
                uint16_t tx_fsr = getSn_TX_FSR(sn);
                if (tx_fsr > 0) {
                    ret |= MP_STREAM_POLL_WR;
                    DEBUG_PRINT("socket_ioctl: can write (TX_FSR=%d)\n", tx_fsr);
                }
            } else if (sr == SOCK_INIT || sr == SOCK_LISTEN) {
                ret |= MP_STREAM_POLL_WR;
                DEBUG_PRINT("socket_ioctl: socket ready for connect/listen\n");
            }
        }
        
        DEBUG_PRINT("socket_ioctl: returning 0x%x\n", ret);
        return ret;
    } else {
        DEBUG_PRINT("socket_ioctl: unsupported request 0x%x\n", request);
        *_errno = MP_EINVAL;
        return MP_STREAM_ERROR;
    }
}
```


## Summary:

The key changes are:
1. **Check interrupt register (`Sn_IR`)** for `CON` flag when socket is in `LISTEN` state
2. **Send `ACCEPT` command** (`Sn_CR = ACCEPT`) when connection is detected
3. **Wait for state transition** to `ESTABLISHED` after sending `ACCEPT`
4. **Update `ioctl()`** to check interrupts for listening sockets

This is the standard W5500 flow: `LISTEN` → (connection arrives, `CON` interrupt) → `ACCEPT` command → `ESTABLISHED`.

**Make these two changes in `network_wiznet5k.c` and recompile.** The telnet connection should now work properly!


