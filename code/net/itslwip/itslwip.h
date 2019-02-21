#pragma once

/*
ITS kernel LWIP port main file.
*/

/* INCLUDES */

#include <stdbool.h>
#include <stdint.h>


/* TYPES */

// Type of a connection handle.
typedef int conn_handle_t;
#define CONN_HANDLE_INVALID -1


/* DECLARATIONS */

// ITS kernel LWIP thread main function.
// args contains three char pointers to the IP address, the subnet mask and the gateway, respectively.
void itslwip_run(void *args);

// Creates a TCP or UDP connection to the given target address and port, and returns a handle for that connection (or -1 on errors).
conn_handle_t itslwip_connect(const char *targetAddress, int targetPort, bool useUdp);

// Closes the given connection.
void itslwip_disconnect(conn_handle_t connHandle);

// Sends data over the given connection.
// This function blocks until all data was sent and acknowledged by the target.
void itslwip_send(conn_handle_t connHandle, uint8_t *data, int dataLength);

// Sends string data over the given connection.
// This function blocks until all data was sent and acknowledged by the target.
void itslwip_send_string(conn_handle_t connHandle, char *string, int stringLength);

// Receives the given amount of bytes.
// This function blocks until the desired amount of bytes was received.
void itslwip_receive_data(conn_handle_t connHandle, uint8_t *dataBuffer, int dataLength);

// Receives a text line.
// This function blocks until a new line character (\n) was encountered.
// The resulting string is \0 terminated; the new line character itself is not returned.
void itslwip_receive_line(conn_handle_t connHandle, char *lineBuffer, int lineBufferLength);
