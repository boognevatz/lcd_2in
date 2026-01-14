# Detailed Step-by-Step Action Plan

## Overview
Fix W5500 socket accept/close cycle bug in `micropython/extmod/network_wiznet5k.c`. Socket 0 gets stuck in CLOSED state after accepting and closing a connection, and subsequent `accept()` calls fail.

## Root Cause Analysis

### Issue 1: Re-listening Condition is Too Restrictive
Line 621: `if (sr_before == SOCK_ESTABLISHED && saved_port != 0)` - only re-listens if state is exactly `SOCK_ESTABLISHED` when close() is called. But when client disconnects, socket transitions to `SOCK_CLOSE_WAIT` (0x1C), so re-listening never happens.

### Issue 2: Single-Client Server Model Prevents Re-listening
Line 886 in `wiznet5k_socket_accept`: `socket->domain = 0` marks the listening socket as "not opened" after first accept. This prevents the socket from accepting new connections.

### Issue 3: accept() Doesn't Handle CLOSED Sockets
Lines 787-791: If socket is in CLOSED state, accept() returns error instead of re-listening.

---

## Step-by-Step Fix Procedure

### Step 1: Fix Re-listening Condition (Line 621)
**File:** `micropython/extmod/network_wiznet5k.c:621`

Change:
```c
if (sr_before == SOCK_ESTABLISHED && saved_port != 0)
```

To:
```c
if ((sr_before == SOCK_ESTABLISHED || sr_before == SOCK_CLOSE_WAIT) && saved_port != 0)
```

**Rationale:** When a client closes the connection, the socket transitions to `SOCK_CLOSE_WAIT` before going to CLOSED. We need to re-listen in both cases to accept new connections.

### Step 2: Add State Reset Logic After Re-listen
**File:** `micropython/extmod/network_wiznet5k.c:636-641`

After successful re-listening (line 636-637), add:
```c
// Mark socket as available for accepts again
wiznet5k_obj.socket_used |= (1 << sn);
```

**Rationale:** After re-listening, the socket needs to be marked as "in use" so the socket tracking is consistent.

### Step 3: Remove Socket Domain Marking in accept()
**File:** `micropython/extmod/network_wiznet5k.c:816-817` and `886-887`

Remove these lines:
```c
socket->domain = 0; // Mark as "not opened"
```

**Rationale:** In single-client server mode, marking the listening socket as unavailable prevents it from being reused. After close() and re-listening, the socket should be able to accept new connections.

### Step 4: Add Re-listen Logic in accept()
**File:** `micropython/extmod/network_wiznet5k.c:787-791`

Change:
```c
if (sr_initial != SOCK_LISTEN && sr_initial != SOCK_ESTABLISHED) {
    DEBUG_PRINT("socket_accept: socket not in LISTEN or ESTABLISHED state\n");
    *_errno = MP_EINVAL;
    return -1;
}
```

To:
```c
if (sr_initial != SOCK_LISTEN && sr_initial != SOCK_ESTABLISHED) {
    DEBUG_PRINT("socket_accept: socket not in LISTEN or ESTABLISHED state (0x%02x)\n", sr_initial);
    
    // If socket is in CLOSED state but we have a saved port from previous listening,
    // try to re-listen on the same port
    if (sr_initial == SOCK_CLOSED && socket->domain == 1) {
        DEBUG_PRINT("socket_accept: socket in CLOSED state, attempting to re-listen\n");
        
        uint16_t saved_port = getSn_PORT(sn);
        uint8_t saved_type = socket->type & 0x0F;
        
        if (saved_port != 0) {
            DEBUG_PRINT("socket_accept: re-listening on port %d\n", saved_port);
            
            mp_int_t ret = WIZCHIP_EXPORT(socket)(sn, saved_type, saved_port, 0);
            if (ret >= 0) {
                ret = WIZCHIP_EXPORT(listen)(sn);
                if (ret >= 0) {
                    uint8_t sr = getSn_SR(sn);
                    if (sr == SOCK_LISTEN) {
                        DEBUG_PRINT("socket_accept: successfully re-listened\n");
                        sr_initial = sr; // Update state and continue
                    } else {
                        DEBUG_PRINT("socket_accept: failed to re-listen, state=0x%02x\n", sr);
                        *_errno = MP_EINVAL;
                        return -1;
                    }
                } else {
                    DEBUG_PRINT("socket_accept: listen() failed: %d\n", ret);
                    *_errno = -ret;
                    return -1;
                }
            } else {
                DEBUG_PRINT("socket_accept: socket() failed: %d\n", ret);
                *_errno = -ret;
                return -1;
            }
        }
    } else {
        *_errno = MP_EINVAL;
        return -1;
    }
}
```

**Rationale:** If accept() is called on a CLOSED socket that was previously listening, attempt to re-listen on the saved port before failing.

### Step 5: Add Socket State Tracking Structure
**File:** `micropython/extmod/network_wiznet5k.c` - after line 153 (before socket_used)

Add to `wiznet5k_obj_t` structure:
```c
typedef struct _wiznet5k_socket_state_t {
    uint16_t port;
    uint8_t type;
    bool is_listening;
} wiznet5k_socket_state_t;

typedef struct _wiznet5k_obj_t {
    mp_obj_base_t base;
    mp_uint_t cris_state;
    mp_obj_base_t *spi;
    void (*spi_transfer)(mp_obj_base_t *obj, size_t len, const uint8_t *src, uint8_t *dest);
    mp_hal_pin_obj_t cs;
    mp_hal_pin_obj_t rst;
    #if WIZNET5K_WITH_LWIP_STACK
    // ... existing lwIP fields ...
    #else // WIZNET5K_PROVIDED_STACK
    wiz_NetInfo netinfo;
    uint8_t socket_used;
    wiznet5k_socket_state_t socket_states[_WIZCHIP_SOCK_NUM_];  // Add this line
    bool active;
    #endif
} wiznet5k_obj_t;
```

**Rationale:** Track the listening state for each socket separately to enable proper re-listening.

### Step 6: Initialize Socket State Tracking
**File:** `micropython/extmod/network_wiznet5k.c` - line 512 in `wiznet5k_init()`

After line 512, add:
```c
// Initialize socket state tracking
for (int i = 0; i < _WIZCHIP_SOCK_NUM_; i++) {
    wiznet5k_obj.socket_states[i].port = 0;
    wiznet5k_obj.socket_states[i].type = 0;
    wiznet5k_obj.socket_states[i].is_listening = false;
}
```

### Step 7: Update `wiznet5k_socket_listen` to Save State
**File:** `micropython/extmod/network_wiznet5k.c:770`

After line 770 (`DEBUG_PRINT("socket_listen: success\n");`), add:
```c
// Save socket state for potential re-listening
wiznet5k_obj.socket_states[sn].port = getSn_PORT(sn);
wiznet5k_obj.socket_states[sn].type = socket->type & 0x0F;
wiznet5k_obj.socket_states[sn].is_listening = true;
DEBUG_PRINT("socket_listen: saved state - port=%d, type=0x%02x\n",
           wiznet5k_obj.socket_states[sn].port, wiznet5k_obj.socket_states[sn].type);
```

### Step 8: Update `wiznet5k_socket_close` to Use Saved State
**File:** `micropython/extmod/network_wiznet5k.c:599-609`

Replace the state saving logic (lines 599-609) with:
```c
// Get saved socket state if available
uint16_t saved_port = wiznet5k_obj.socket_states[sn].port;
uint8_t saved_type = wiznet5k_obj.socket_states[sn].type;
bool was_listening = wiznet5k_obj.socket_states[sn].is_listening;
bool is_established = (sr_before == SOCK_ESTABLISHED);
bool can_relisten = was_listening && (is_established || sr_before == SOCK_CLOSE_WAIT);

DEBUG_PRINT("socket_close: saved state - port=%d, type=0x%02x, was_listening=%d\n",
           saved_port, saved_type, was_listening);
```

### Step 9: Simplify Re-listen Logic in close()
**File:** `micropython/extmod/network_wiznet5k.c:621-643`

Replace lines 621-643 with:
```c
// Re-listen if this was a listening socket that accepted a connection
if (can_relisten && saved_port != 0) {
    DEBUG_PRINT("socket_close: re-listening on port %d\n", saved_port);
    
    mp_int_t ret = WIZCHIP_EXPORT(socket)(sn, saved_type, saved_port, 0);
    DEBUG_PRINT("socket_close: WIZCHIP_EXPORT(socket) returned %d\n", ret);
    
    if (ret >= 0) {
        ret = WIZCHIP_EXPORT(listen)(sn);
        DEBUG_PRINT("socket_close: WIZCHIP_EXPORT(listen) returned %d\n", ret);
        
        if (ret >= 0) {
            // Mark socket as in-use and listening again
            wiznet5k_obj.socket_used |= (1 << sn);
            wiznet5k_obj.socket_states[sn].is_listening = true;
            
            uint8_t sr_final = getSn_SR(sn);
            if (sr_final == SOCK_LISTEN) {
                DEBUG_PRINT("socket_close: successfully re-listened\n");
            } else {
                DEBUG_PRINT("socket_close: failed to reach LISTEN state, got 0x%02x\n", sr_final);
                wiznet5k_obj.socket_states[sn].is_listening = false;
            }
        } else {
            DEBUG_PRINT("socket_close: listen failed, error=%d\n", ret);
            wiznet5k_obj.socket_states[sn].is_listening = false;
        }
    } else {
        DEBUG_PRINT("socket_close: socket failed, error=%d\n", ret);
    }
} else {
    // Clear listening state if we're not re-listening
    if (!is_established) {
        wiznet5k_obj.socket_states[sn].is_listening = false;
        wiznet5k_obj.socket_states[sn].port = 0;
    }
}
```

---

## Verification Checklist

- [ ] Socket accepts first connection
- [ ] After closing first connection, socket transitions to LISTEN state (not CLOSED)
- [ ] Socket accepts second connection without errors
- [ ] Multiple sequential connections work correctly
- [ ] Debug logs show proper state transitions
- [ ] No duplicate code remains
- [ ] socket_used flag is properly maintained


