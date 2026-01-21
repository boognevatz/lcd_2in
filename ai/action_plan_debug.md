# Debug Print Action Plan for Deadlock Investigation

## Strategy
Add timestamped debug prints at every critical point in the execution path to identify the exact freeze location. Use a consistent format for easy log analysis.

---

## Phase 1: Python Application Layer (`main_test04_svg_50kb.py`)

### Main Loop - Request Handling
```python
print(f"[MAIN] Listening on {nic.ifconfig()[0]}:80")

while True:
    try:
        print("[MAIN] ====== WAITING FOR CONNECTION ======")
        cl, addr = s.accept()
        print(f"[MAIN] accept() returned, addr={addr}")
        
        print(f"[MAIN] Calling recv() for request data")
        request = cl.recv(16384)
        print(f"[MAIN] recv() returned {len(request)} bytes")
        
        # Parse request
        print(f"[MAIN] Starting request parsing")
        request_str = request.decode()
        print(f"[MAIN] Request decoded, length={len(request_str)}")
        
        request_lines = request_str.split('\r\n')
        print(f"[MAIN] Request split into {len(request_lines)} lines")
        
        request_line = request_lines[0]
        print(f"[MAIN] First line: {request_line[:50]}...")
        
        parts = request_line.split()
        print(f"[MAIN] Split into {len(parts)} parts")
        
        if len(parts) >= 2:
            method, path = parts[0], parts[1]
            print(f"[MAIN] Parsed: method={method}, path={path}")
        else:
            path = '/'
            print(f"[MAIN] Using default path=/")
        
        print(f"[MAIN] Request path: {path}")
        
        # Generate response
        if path == '/svg':
            print("[MAIN] Calling generate_svg_response()")
            response_body = generate_svg_response()
            print(f"[MAIN] SVG generated, size={len(response_body)}")
            content_type = 'image/svg+xml'
        else:
            print("[MAIN] Calling generate_large_html_response()")
            response_body = generate_large_html_response()
            print(f"[MAIN] HTML generated, size={len(response_body)}")
            content_type = 'text/html; charset=UTF-8'
        
        print(f"[MAIN] Building HTTP headers")
        response_header = b'HTTP/1.1 200 OK\r\nContent-Type: ' + content_type.encode() + b'\r\nContent-Length: ' + str(len(response_body)).encode() + b'\r\nConnection: close\r\nCache-Control: no-cache\r\nServer: W5500-Test/1.0\r\n\r\n'
        print(f"[MAIN] Header size={len(response_header)}")
        
        full_response = response_header + response_body
        print(f"[MAIN] Full response size: {len(full_response)} bytes")
        
        print(f"[MAIN] Calling cl.send()")
        cl.send(full_response)
        print(f"[MAIN] send() returned successfully")
        
        print(f"[MAIN] Calling cl.close()")
        cl.close()
        print(f"[MAIN] close() returned successfully")
        
        print(f"[MAIN] Request handling complete")
        
    except Exception as e:
        print(f"[MAIN] Exception caught: {type(e).__name__}: {e}")
        import sys
        sys.print_exception(e)
```

---

## Phase 2: Socket Layer (`modsocket.c`)

### socket_accept()
```c
static mp_obj_t socket_accept(mp_obj_t self_in) {
    mod_network_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    DEBUG_PRINT("socket_accept: ENTRY, socket=%d\n", self->fileno);

    if (self->nic == MP_OBJ_NULL) {
        DEBUG_PRINT("socket_accept: ERROR - not bound\n");
        mp_raise_OSError(MP_EINVAL);
    }

    DEBUG_PRINT("socket_accept: creating new socket object\n");
    mod_network_socket_obj_t *socket2 = mp_obj_malloc_with_finaliser(mod_network_socket_obj_t, &socket_type);
    socket2->nic = MP_OBJ_NULL;
    socket2->nic_protocol = NULL;
    DEBUG_PRINT("socket_accept: socket2 allocated at %p\n", socket2);

    socket2->domain = self->domain;
    socket2->type = self->type;
    socket2->proto = self->proto;
    socket2->bound = false;
    socket2->fileno = -1;
    socket2->timeout = -1;
    socket2->callback = MP_OBJ_NULL;
    socket2->state = MOD_NETWORK_SS_NEW;
    #if MICROPY_PY_SOCKET_EXTENDED_STATE
    socket2->_private = NULL;
    #endif
    DEBUG_PRINT("socket_accept: socket2 initialized\n");

    uint8_t ip[MOD_NETWORK_IPADDR_BUF_SIZE];
    mp_uint_t port;
    int _errno;
    
    DEBUG_PRINT("socket_accept: calling nic_protocol->accept()\n");
    if (self->nic_protocol->accept(self, socket2, ip, &port, &_errno) != 0) {
        DEBUG_PRINT("socket_accept: accept() failed, errno=%d\n", _errno);
        mp_raise_OSError(_errno);
    }
    DEBUG_PRINT("socket_accept: accept() returned successfully\n");

    socket2->nic = self->nic;
    socket2->nic_protocol = self->nic_protocol;
    DEBUG_PRINT("socket_accept: socket2 NIC assigned\n");

    DEBUG_PRINT("socket_accept: creating tuple for return\n");
    mp_obj_tuple_t *client = MP_OBJ_TO_PTR(mp_obj_new_tuple(2, NULL));
    DEBUG_PRINT("socket_accept: tuple allocated\n");
    
    client->items[0] = MP_OBJ_FROM_PTR(socket2);
    DEBUG_PRINT("socket_accept: tuple[0] = socket2\n");
    
    client->items[1] = netutils_format_inet_addr(ip, port, NETUTILS_BIG);
    DEBUG_PRINT("socket_accept: tuple[1] = address (%d.%d.%d.%d:%d)\n", 
                ip[0], ip[1], ip[2], ip[3], port);

    DEBUG_PRINT("socket_accept: EXIT, returning tuple\n");
    return MP_OBJ_FROM_PTR(client);
}
```

### socket_recv()
```c
static mp_uint_t socket_recv(mod_network_socket_obj_t *socket, byte *buf, mp_uint_t len, int *_errno) {
    uint8_t sn = (uint8_t)socket->fileno;
    DEBUG_PRINT("socket_recv: ENTRY, socket=%d, len=%d, timeout=%d\n",
                sn, len, socket->timeout);

    uint32_t start_time = mp_hal_ticks_ms();
    int loop_counter = 0;

    while (1) {
        loop_counter++;
        
        if (loop_counter % 50 == 1) {
            DEBUG_PRINT("socket_recv: loop %d, checking RX_RSR\n", loop_counter);
        }
        
        uint16_t rx_rsr = getSn_RX_RSR(sn);
        
        if (loop_counter % 50 == 1) {
            DEBUG_PRINT("socket_recv: RX_RSR=%d\n", rx_rsr);
        }

        if (rx_rsr > 0) {
            DEBUG_PRINT("socket_recv: data available (%d bytes), calling WIZCHIP recv\n", rx_rsr);

            MP_THREAD_GIL_EXIT();
            mp_int_t ret = WIZCHIP_EXPORT(recv)(sn, buf, len);
            MP_THREAD_GIL_ENTER();

            DEBUG_PRINT("socket_recv: WIZCHIP recv returned %d\n", ret);

            if (ret < 0) {
                DEBUG_PRINT("socket_recv: ERROR, ret=%d\n", ret);
                wiznet5k_socket_close(socket);
                *_errno = -ret;
                return -1;
            }

            DEBUG_PRINT("socket_recv: EXIT, received %d bytes\n", ret);
            return ret;
        }

        uint8_t sr = getSn_SR(sn);
        if (loop_counter % 50 == 1) {
            DEBUG_PRINT("socket_recv: socket state=0x%02x (%s)\n",
                       sr, socket_state_str(sr));
        }

        if (sr != SOCK_ESTABLISHED && sr != SOCK_CLOSE_WAIT) {
            DEBUG_PRINT("socket_recv: ERROR - invalid state 0x%02x\n", sr);
            *_errno = MP_ENOTCONN;
            return -1;
        }

        if (socket->timeout > 0) {
            uint32_t elapsed = mp_hal_ticks_ms() - start_time;
            if (elapsed >= socket->timeout) {
                DEBUG_PRINT("socket_recv: TIMEOUT after %d ms\n", elapsed);
                *_errno = MP_ETIMEDOUT;
                return -1;
            }
        } else if (socket->timeout == 0) {
            DEBUG_PRINT("socket_recv: non-blocking, no data available\n");
            *_errno = MP_EAGAIN;
            return -1;
        }

        mp_hal_delay_ms(10);
    }
}
```

### socket_send()
```c
static mp_uint_t wiznet5k_socket_send(mod_network_socket_obj_t *socket, const byte *buf, mp_uint_t len, int *_errno) {
    uint8_t sn = (uint8_t)socket->fileno;
    DEBUG_PRINT("socket_send: ENTRY, socket=%d, len=%d\n", sn, len);

    if (len < 1024) {
        DEBUG_PRINT("socket_send: small transfer, using blocking send\n");
        MP_THREAD_GIL_EXIT();
        mp_int_t ret = WIZCHIP_EXPORT(send)(socket->fileno, (byte *)buf, len);
        MP_THREAD_GIL_ENTER();

        DEBUG_PRINT("socket_send: blocking send returned %d\n", ret);
        if (ret < 0) {
            DEBUG_PRINT("socket_send: ERROR, ret=%d\n", ret);
            wiznet5k_socket_close(socket);
            *_errno = -ret;
            return -1;
        }
        DEBUG_PRINT("socket_send: EXIT (small), sent %d bytes\n", ret);
        return ret;
    }

    DEBUG_PRINT("socket_send: large transfer (%d bytes), using DMA\n", len);

    DEBUG_PRINT("socket_send: getting transfer descriptor\n");
    w5500_tx_transfer_t* transfer = w5500_get_transfer();
    if (!transfer) {
        DEBUG_PRINT("socket_send: transfer pool exhausted, falling back\n");
        MP_THREAD_GIL_EXIT();
        mp_int_t ret = WIZCHIP_EXPORT(send)(socket->fileno, (byte *)buf, len);
        MP_THREAD_GIL_ENTER();

        if (ret < 0) {
            wiznet5k_socket_close(socket);
            *_errno = -ret;
            return -1;
        }
        return ret;
    }
    DEBUG_PRINT("socket_send: transfer descriptor allocated\n");

    transfer->data_ptr = buf;
    transfer->total_size = len;
    transfer->bytes_sent = 0;
    transfer->socket_num = sn;
    transfer->state = TX_WAITING_BUFFER_SPACE;
    DEBUG_PRINT("socket_send: transfer initialized\n");

    uint16_t free_space = getSn_TX_FSR(sn);
    uint16_t max_chunk = getSn_TxMAX(sn);
    transfer->chunk_size = (len < free_space) ? len : free_space;
    if (transfer->chunk_size > max_chunk) {
        transfer->chunk_size = max_chunk;
    }
    DEBUG_PRINT("socket_send: initial chunk_size=%d (free_space=%d, max_chunk=%d)\n",
                transfer->chunk_size, free_space, max_chunk);

    DEBUG_PRINT("socket_send: queueing transfer\n");
    w5500_queue_transfer(transfer);
    DEBUG_PRINT("socket_send: transfer queued\n");

    uint32_t timeout_ms = (socket->timeout > 0) ? socket->timeout : 30000;
    DEBUG_PRINT("socket_send: waiting for completion (timeout=%d ms)\n", timeout_ms);

    int result = w5500_wait_transfer_complete(transfer, timeout_ms);
    DEBUG_PRINT("socket_send: wait_transfer_complete returned %d\n", result);

    if (result < 0 || transfer->state == TX_ERROR) {
        DEBUG_PRINT("socket_send: transfer failed\n");
        *_errno = MP_EIO;
        return -1;
    }

    uint32_t bytes_sent = transfer->bytes_sent;
    DEBUG_PRINT("socket_send: EXIT, sent %d bytes\n", bytes_sent);
    return bytes_sent;
}
```

### socket_close()
```c
static void wiznet5k_socket_close(mod_network_socket_obj_t *socket) {
    uint8_t sn = (uint8_t)socket->fileno;
    DEBUG_PRINT("socket_close: ENTRY, socket %d\n", sn);

    if (sn < _WIZCHIP_SOCK_NUM_) {
        DEBUG_PRINT("socket_close: cancelling active transfers\n");
        w5500_tx_transfer_t* transfer = active_transfers;
        w5500_tx_transfer_t* prev = NULL;

        while (transfer) {
            w5500_tx_transfer_t* next = transfer->next;

            if (transfer->socket_num == sn) {
                DEBUG_PRINT("socket_close: found transfer for socket %d, cancelling\n", sn);

                if (prev) {
                    prev->next = next;
                } else {
                    active_transfers = next;
                }

                transfer->state = TX_ERROR;
                w5500_release_transfer(transfer);
            } else {
                prev = transfer;
            }

            transfer = next;
        }
        DEBUG_PRINT("socket_close: active transfers cancelled\n");

        uint8_t sr_before = getSn_SR(sn);
        DEBUG_PRINT("socket_close: state before close = 0x%02x (%s)\n",
                   sr_before, socket_state_str(sr_before));

        uint16_t saved_port = wiznet5k_obj.socket_states[sn].port;
        uint8_t saved_type = wiznet5k_obj.socket_states[sn].type;
        bool was_listening = wiznet5k_obj.socket_states[sn].is_listening;
        bool is_established = (sr_before == SOCK_ESTABLISHED);
        bool can_relisten = was_listening && (is_established || sr_before == SOCK_CLOSE_WAIT);

        DEBUG_PRINT("socket_close: saved_port=%d, saved_type=0x%02x, was_listening=%d, can_relisten=%d\n",
                   saved_port, saved_type, was_listening, can_relisten);

        wiznet5k_obj.socket_used &= ~(1 << sn);
        DEBUG_PRINT("socket_close: marked socket as unused\n");
        
        mp_hal_delay_ms(50);
        
        if (getSn_SR(sn) == SOCK_ESTABLISHED) {
            DEBUG_PRINT("socket_close: calling disconnect()\n");
            WIZCHIP_EXPORT(disconnect)(sn);
            
            uint32_t start = mp_hal_ticks_ms();
            while (getSn_SR(sn) != SOCK_CLOSED && mp_hal_ticks_ms() - start < 250) {
                mp_hal_delay_ms(5);
            }
            DEBUG_PRINT("socket_close: disconnect wait complete, state=0x%02x\n", getSn_SR(sn));
        }
        
        DEBUG_PRINT("socket_close: calling WIZCHIP close()\n");
        WIZCHIP_EXPORT(close)(sn);
        mp_hal_delay_ms(50);

        uint8_t sr_after = getSn_SR(sn);
        DEBUG_PRINT("socket_close: state after close = 0x%02x (%s)\n",
                   sr_after, socket_state_str(sr_after));

        if (can_relisten && saved_port != 0) {
            DEBUG_PRINT("socket_close: re-listening on port %d\n", saved_port);

            mp_int_t ret = WIZCHIP_EXPORT(socket)(sn, saved_type, saved_port, 0);
            DEBUG_PRINT("socket_close: socket() returned %d\n", ret);

            if (ret >= 0) {
                ret = WIZCHIP_EXPORT(listen)(sn);
                DEBUG_PRINT("socket_close: listen() returned %d\n", ret);

                if (ret >= 0) {
                    wiznet5k_obj.socket_used |= (1 << sn);
                    wiznet5k_obj.socket_states[sn].is_listening = true;

                    uint8_t sr_final = getSn_SR(sn);
                    DEBUG_PRINT("socket_close: final state = 0x%02x (%s)\n",
                               sr_final, socket_state_str(sr_final));
                    
                    if (sr_final == SOCK_LISTEN) {
                        DEBUG_PRINT("socket_close: successfully re-listened\n");
                    } else {
                        DEBUG_PRINT("socket_close: WARNING - not in LISTEN state\n");
                        wiznet5k_obj.socket_states[sn].is_listening = false;
                    }
                } else {
                    DEBUG_PRINT("socket_close: listen() failed\n");
                    wiznet5k_obj.socket_states[sn].is_listening = false;
                }
            } else {
                DEBUG_PRINT("socket_close: socket() failed\n");
            }
        } else {
            DEBUG_PRINT("socket_close: not re-listening\n");
            if (!is_established) {
                wiznet5k_obj.socket_states[sn].is_listening = false;
                wiznet5k_obj.socket_states[sn].port = 0;
            }
        }
    } else {
        DEBUG_PRINT("socket_close: invalid socket number %d\n", sn);
    }
    
    DEBUG_PRINT("socket_close: EXIT\n");
}
```

---

## Phase 3: W5500 Driver Layer (`network_wiznet5k.c`)

### wiznet5k_socket_accept()
```c
static int wiznet5k_socket_accept(mod_network_socket_obj_t *socket, mod_network_socket_obj_t *socket2, byte *ip, mp_uint_t *port, int *_errno) {
    uint8_t sn = (uint8_t)socket->fileno;
    DEBUG_PRINT("wiznet5k_socket_accept: ENTRY, socket=%d, timeout=%d\n", sn, socket->timeout);

    uint8_t sr_initial = getSn_SR(sn);
    DEBUG_PRINT("wiznet5k_socket_accept: initial state = 0x%02x (%s)\n",
                sr_initial, socket_state_str(sr_initial));

    if (sr_initial != SOCK_LISTEN && sr_initial != SOCK_ESTABLISHED) {
        DEBUG_PRINT("wiznet5k_socket_accept: not in LISTEN/ESTABLISHED state\n");

        if (sr_initial == SOCK_CLOSED && socket->domain == 1) {
            DEBUG_PRINT("wiznet5k_socket_accept: attempting to re-listen\n");

            uint16_t saved_port = wiznet5k_obj.socket_states[sn].port;
            uint8_t saved_type = wiznet5k_obj.socket_states[sn].type;

            if (saved_port != 0) {
                DEBUG_PRINT("wiznet5k_socket_accept: re-listening on port %d\n", saved_port);

                mp_int_t ret = WIZCHIP_EXPORT(socket)(sn, saved_type, saved_port, 0);
                DEBUG_PRINT("wiznet5k_socket_accept: socket() returned %d\n", ret);
                
                if (ret >= 0) {
                    ret = WIZCHIP_EXPORT(listen)(sn);
                    DEBUG_PRINT("wiznet5k_socket_accept: listen() returned %d\n", ret);
                    
                    if (ret >= 0) {
                        uint8_t sr = getSn_SR(sn);
                        DEBUG_PRINT("wiznet5k_socket_accept: new state = 0x%02x\n", sr);
                        
                        if (sr == SOCK_LISTEN) {
                            DEBUG_PRINT("wiznet5k_socket_accept: successfully re-listened\n");
                            sr_initial = sr;
                        } else {
                            DEBUG_PRINT("wiznet5k_socket_accept: failed to reach LISTEN\n");
                            *_errno = MP_EINVAL;
                            return -1;
                        }
                    } else {
                        DEBUG_PRINT("wiznet5k_socket_accept: listen() failed\n");
                        *_errno = -ret;
                        return -1;
                    }
                } else {
                    DEBUG_PRINT("wiznet5k_socket_accept: socket() failed\n");
                    *_errno = -ret;
                    return -1;
                }
            }
        } else {
            DEBUG_PRINT("wiznet5k_socket_accept: cannot accept in this state\n");
            *_errno = MP_EINVAL;
            return -1;
        }
    }

    uint8_t ir = getSn_IR(sn);
    uint8_t sr = getSn_SR(sn);
    DEBUG_PRINT("wiznet5k_socket_accept: IR=0x%02x, SR=0x%02x\n", ir, sr);

    if (sr == SOCK_ESTABLISHED) {
        DEBUG_PRINT("wiznet5k_socket_accept: already ESTABLISHED, accepting immediately\n");

        getSn_DIPR(sn, ip);
        *port = getSn_DPORT(sn);

        DEBUG_PRINT("wiznet5k_socket_accept: client IP: %d.%d.%d.%d:%d\n",
                   ip[0], ip[1], ip[2], ip[3], *port);

        socket2->domain = socket->domain;
        socket2->type = socket->type;
        socket2->fileno = sn;
        socket2->timeout = socket->timeout;

        DEBUG_PRINT("wiznet5k_socket_accept: EXIT (immediate success)\n");
        return 0;
    }

    if (socket->timeout == 0) {
        DEBUG_PRINT("wiznet5k_socket_accept: non-blocking, no connection pending\n");
        *_errno = MP_EAGAIN;
        return -1;
    }

    DEBUG_PRINT("wiznet5k_socket_accept: entering wait loop\n");
    uint32_t start_time = mp_hal_ticks_ms();
    uint32_t timeout_ms = socket->timeout;
    if (timeout_ms == -1) {
        timeout_ms = 0x7FFFFFFF;
    }

    int loop_counter = 0;

    for (;;) {
        loop_counter++;

        if (socket->timeout != -1 && (mp_hal_ticks_ms() - start_time) >= timeout_ms) {
            DEBUG_PRINT("wiznet5k_socket_accept: timeout after %d ms\n", timeout_ms);
            *_errno = MP_ETIMEDOUT;
            return -1;
        }

        ir = getSn_IR(sn);
        sr = getSn_SR(sn);

        if (loop_counter % 100 == 0) {
            DEBUG_PRINT("wiznet5k_socket_accept: loop %d - IR=0x%02x, SR=0x%02x (%s)\n",
                    loop_counter, ir, sr, socket_state_str(sr));
        }

        if ((ir & Sn_IR_CON) || sr == SOCK_ESTABLISHED) {
            DEBUG_PRINT("wiznet5k_socket_accept: connection detected\n");
            
            if (ir & Sn_IR_CON) {
                DEBUG_PRINT("wiznet5k_socket_accept: clearing CON interrupt\n");
                setSn_IR(sn, Sn_IR_CON);
            }

            mp_hal_delay_ms(10);

            sr = getSn_SR(sn);
            DEBUG_PRINT("wiznet5k_socket_accept: state after delay = 0x%02x\n", sr);
            
            if (sr == SOCK_ESTABLISHED) {
                DEBUG_PRINT("wiznet5k_socket_accept: socket ESTABLISHED!\n");

                getSn_DIPR(sn, ip);
                *port = getSn_DPORT(sn);

                DEBUG_PRINT("wiznet5k_socket_accept: client IP: %d.%d.%d.%d:%d\n",
                           ip[0], ip[1], ip[2], ip[3], *port);

                socket2->domain = socket->domain;
                socket2->type = socket->type;
                socket2->fileno = sn;
                socket2->timeout = socket->timeout;

                DEBUG_PRINT("wiznet5k_socket_accept: EXIT (success)\n");
                return 0;
            } else {
                DEBUG_PRINT("wiznet5k_socket_accept: not yet ESTABLISHED, continuing\n");
            }
        }

        if (sr == SOCK_CLOSED) {
            DEBUG_PRINT("wiznet5k_socket_accept: socket closed unexpectedly\n");
            *_errno = MP_ENOTCONN;
            return -1;
        }

        if (loop_counter % 100 == 0) {
            DEBUG_PRINT("wiznet5k_socket_accept: still waiting (loop %d)\n", loop_counter);
        }

        mp_hal_delay_ms(50);
        mpy_wiznet_yield();
    }
}
```

### mpy_wiznet_yield()
```c
void mpy_wiznet_yield(void) {
    DEBUG_PRINT("mpy_wiznet_yield: ENTRY\n");
    
    #if MICROPY_PY_THREAD
    MICROPY_THREAD_YIELD();
    #else
    mp_handle_pending(true);
    #endif
    
    DEBUG_PRINT("mpy_wiznet_yield: calling w5500_tx_service()\n");
    w5500_tx_service();
    DEBUG_PRINT("mpy_wiznet_yield: EXIT\n");
}
```

### w5500_tx_service() - State Machine
```c
static void w5500_tx_service(void) {
    w5500_tx_transfer_t* transfer = active_transfers;
    
    if (!transfer) {
        return; // No active transfers
    }
    
    DEBUG_PRINT("w5500_tx_service: ENTRY, active transfers exist\n");

    while (transfer) {
        w5500_tx_transfer_t* next = transfer->next;
        
        DEBUG_PRINT("w5500_tx_service: processing transfer sn=%d, state=%d, bytes=%d/%d\n",
                    transfer->socket_num, transfer->state, 
                    transfer->bytes_sent, transfer->total_size);

        uint8_t sr = getSn_SR(transfer->socket_num);
        if (sr != SOCK_ESTABLISHED && sr != SOCK_CLOSE_WAIT) {
            DEBUG_PRINT("w5500_tx_service: socket %d invalid state 0x%02x, aborting\n",
                        transfer->socket_num, sr);
            transfer->state = TX_ERROR;
            w5500_dequeue_transfer(transfer);
            w5500_release_transfer(transfer);
            transfer = next;
            continue;
        }
        
        switch (transfer->state) {
            case TX_WAITING_BUFFER_SPACE: {
                DEBUG_PRINT("w5500_tx_service: state=TX_WAITING_BUFFER_SPACE\n");
                
                uint16_t free_space = getSn_TX_FSR(transfer->socket_num);
                DEBUG_PRINT("w5500_tx_service: free_space=%d, chunk_size=%d\n",
                           free_space, transfer->chunk_size);

                if (free_space >= transfer->chunk_size) {
                    DEBUG_PRINT("w5500_tx_service: buffer space available, starting DMA\n");
                    
                    uint16_t tx_wr = getSn_TX_WR(transfer->socket_num);
                    const uint8_t* data_ptr = transfer->data_ptr + transfer->bytes_sent;

                    DEBUG_PRINT("w5500_tx_service: calling w5500_send_chunk()\n");
                    w5500_send_chunk(transfer->socket_num, data_ptr, transfer->chunk_size, tx_wr);
                    
                    transfer->state = TX_TRANSFERRING_CHUNK;
                    DEBUG_PRINT("w5500_tx_service: state -> TX_TRANSFERRING_CHUNK\n");
                }
                break;
            }

            case TX_TRANSFERRING_CHUNK: {
                DEBUG_PRINT("w5500_tx_service: state=TX_TRANSFERRING_CHUNK\n");
                
                uint16_t new_tx_wr = getSn_TX_WR(transfer->socket_num) + transfer->chunk_size;
                DEBUG_PRINT("w5500_tx_service: updating TX_WR to %d\n", new_tx_wr);
                setSn_TX_WR(transfer->socket_num, new_tx_wr);
                
                DEBUG_PRINT("w5500_tx_service: issuing SEND command\n");
                setSn_CR(transfer->socket_num, Sn_CR_SEND);

                DEBUG_PRINT("w5500_tx_service: waiting for CR to clear\n");
                while (getSn_CR(transfer->socket_num)) {
                    mpy_wiznet_yield();
                }
                DEBUG_PRINT("w5500_tx_service: CR cleared\n");

                transfer->state = TX_SENDING_PACKET;
                DEBUG_PRINT("w5500_tx_service: state -> TX_SENDING_PACKET\n");
                break;
            }

            case TX_SENDING_PACKET: {
                DEBUG_PRINT("w5500_tx_service: state=TX_SENDING_PACKET\n");
                
                uint8_t ir = getSn_IR(transfer->socket_num);
                DEBUG_
