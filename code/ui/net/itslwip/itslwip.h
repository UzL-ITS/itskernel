#pragma once

/*
ITS kernel LWIP port main file.
*/

/* INCLUDES */

#include <stdint.h>


/* TYPES */

// Type of a TCP connection handle.
typedef int tcp_handle_t;


/* DECLARATIONS */

// ITS kernel LWIP thread main function.
void itslwip_run(void *args);

// Creates a TCP connection to the given target address and port, and returns a handle for that connection (or -1 on errors).
tcp_handle_t itslwip_connect(const char *targetAddress, int targetPort);

// Closes the given TCP connection.
void itslwip_disconnect(tcp_handle_t tcpHandle);

// Sends data over the given TCP connection.
// This function blocks until all data was sent and acknowledged by the target.
void itslwip_send(tcp_handle_t tcpHandle, uint8_t *data, int dataLength);

// Receives the given amount of bytes.
// This function blocks until the desired amount of bytes was received.
void itslwip_receive_data(tcp_handle_t tcpHandle, uint8_t *dataBuffer, int dataLength);

// Receives a text line.
// This function blocks until a new line character (\n) was encountered. The new line itself is not returned.
void itslwip_receive_line(tcp_handle_t tcpHandle, char *lineBuffer, int lineBufferLength);
