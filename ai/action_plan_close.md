The main driver file is `micropython/extmod/network_wiznet5k.c`, it has still some bugs.
Make a detailed step-by-step action plan how to fix it.


 The bug is clear from the logs: socket 0 is stuck in CLOSED state (0x00) but accept() is
  ┃  being called on it repeatedly. This happens after a client connection is closed and the next accept()
  ┃  tries to run on the same socket without the socket being re-listened.
  ┃
  ┃  Based on my analysis of network_wiznet5k.c from before and my attempted edit that corrupted the file,
  ┃  here's an ACTION PLAN to fix this bug:
  ┃
  ┃  Action Plan: Fix W5500 Socket Accept/Close Cycle Bug
  ┃
  ┃  Problem Analysis
  ┃  1. Socket State Management Issue: When close() is called on a listening socket that has accepted a
  ┃  connection, the hardware socket goes to CLOSED state but the driver has no mechanism to re-listen on it
  ┃  2. Accept Loop: The application loops calling accept() repeatedly on CLOSED socket, causing spam of
  ┃  debug messages and no actual connection handling



