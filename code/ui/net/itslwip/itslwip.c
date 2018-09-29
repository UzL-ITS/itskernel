/*
ITS kernel LWIP port main file.
*/

/* INCLUDES */

#include "itslwip.h"
#include <stdbool.h>
#include <internal/syscall/syscalls.h>
#include <threading/lock.h>
#include <threading/thread.h>
#include "itsnetif.h"

#include "lwip/inet.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"
#include "lwip/init.h"
#include "lwip/stats.h"
#include "netif/etharp.h"
#include "lwip/timeouts.h"

/* TYPES */

// Represents an entry of the receive queue.
typedef struct receive_queue_entry_s
{
	// The next queue entry.
	struct receive_queue_entry_s *next;
	
	// The received data's size.
	int length;
	
	// The received data.
	uint8_t *data;
	
} receive_queue_entry_t;

// Contains state data of a TCP connection.
typedef struct
{
	// Error code of the last callback.
	err_t lastError;
	
	// The LWIP TCP object of this connection.
	struct tcp_pcb *tcpObj;
	
	// The state of the connect() operation (ERR_INPROGRESS until completed).
	err_t connectState;
	
	// Mutex for receive queue access.
	mutex_t receiveQueueMutex;
	
	// The first entry of the receive queue.
	receive_queue_entry_t *receiveQueueStart;
	
	// The last entry of the receive queue.
	receive_queue_entry_t *receiveQueueEnd;
} tcp_conn_data_t;


/* VARIABLES */

// LWIP start time in milliseconds.
static uint64_t startTime;

// LWIP network interface.
static struct netif lwipNetif;

// Addresses.
// TODO use DHCP?
static ip4_addr_t address;
static ip4_addr_t netmask;
static ip4_addr_t gatewayAddress;

// State data of the currently open connections.
// TODO currently only one simultaneous connection is supported.
static tcp_conn_data_t *tcpConnDataList;

// Mutex to ensure synchronized access to the TCP functions by this thread and the caller thread.
static mutex_t lwipMutex;


/* LWIP SYSTEM FUNCTIONS */

void sys_init()
{
	// Remember LWIP start time
	startTime = sys_get_elapsed_milliseconds();
}

uint32_t sys_now()
{
	// Calculate LWIP relative time
	return (uint32_t)(sys_get_elapsed_milliseconds() - startTime);
}


/* FUNCTIONS */

void itslwip_run(void *args)
{
	// Run on Core #0
	set_thread_affinity(0);
	
	// Initialize and acquire mutex
	mutex_init(&lwipMutex);
	mutex_acquire(&lwipMutex);
	
	// Initialize LWIP
	lwip_init();
	
	// Initialize LWIP network interface
	char **addressData = (char **)args;
	inet_aton(addressData[0], &address);
	inet_aton(addressData[1], &netmask);
	inet_aton(addressData[2], &gatewayAddress);
	if(!netif_add(&lwipNetif, &address, &netmask, &gatewayAddress, 0, &itsnetif_init, &ethernet_input))
	{
		printf_locked("Error in netif_add()\n");
		mutex_release(&lwipMutex);
		return;
	}
	netif_set_default(&lwipNetif);
	netif_set_up(&lwipNetif);
	
	// Initialization is done
	mutex_release(&lwipMutex);
	
	// Poll for new packets and send ticks to LWIP
	uint8_t packetBuffer[ETHERNET_PACKET_SIZE];
	while(true)
	{
		// Packet received?
		int packetLength = sys_receive_network_packet(packetBuffer);
		mutex_acquire(&lwipMutex);
		if(packetLength)
		{
			/*printf_locked("Received packet of length %d", packetLength);
			for(int i = 0; i < packetLength; ++i)
			{
				if(i % 16 == 0)
					printf_locked("\n    ");
				printf_locked("%02x ", packetBuffer[i]);
			}
			printf_locked("\n");*/
		
			itsnetif_input(&lwipNetif, packetBuffer, packetLength);
		}
		
		// Update timers
		sys_check_timeouts();
		mutex_release(&lwipMutex);
	}
}

static void handle_tcp_error(void *arg, err_t err)
{
	// Get TCP connection data container
	tcp_conn_data_t *tcpConnData = (tcp_conn_data_t *)arg;
	
	// Set error code
	//printf_locked("TCP error: %d\n", err);
	tcpConnData->lastError = err;
}

static err_t handle_tcp_connected(void *arg, struct tcp_pcb *tcpObj, err_t err)
{
	// Get TCP connection data container
	tcp_conn_data_t *tcpConnData = (tcp_conn_data_t *)arg;
	
	// Set error code
	//printf_locked("TCP connected with error code %d\n", err);
	tcpConnData->lastError = err;
	tcpConnData->connectState = err;
	
	return ERR_OK;
}

static err_t handle_tcp_sent(void *arg, struct tcp_pcb *tcpObj, u16_t len)
{
	// Get TCP connection data container
	//tcp_conn_data_t *tcpConnData = (tcp_conn_data_t *)arg;
	
	// Debug output
	//printf_locked("TCP successfully sent %d bytes\n", len);
	
	return ERR_OK;
}

static err_t handle_tcp_receive(void *arg, struct tcp_pcb *tcpObj, struct pbuf *p, err_t err)
{
	// Get TCP connection data container
	tcp_conn_data_t *tcpConnData = (tcp_conn_data_t *)arg;
	
	// Calculate length of received data
	int length = 0;
	for(struct pbuf *q = p; q != 0; q = q->next)
		if(q->payload)
			length += q->len;
	if(!length)
	{
		//printf_locked("TCP receive function call without data; error code is %d\n", err);
		return ERR_OK;
	}
	
	// Allocate memory for received data and copy
	uint8_t *data = (uint8_t *)malloc(length);
	int copiedBytesCount = 0;
	for(struct pbuf *q = p; q != 0; q = q->next)
	{
		// Valid pbuf?
		if(q->payload && q->len)
		{
			// Copy contents
			memcpy(data + copiedBytesCount, q->payload, q->len);
			copiedBytesCount += q->len;
		}
	}
	
	// Allocate a receive buffer list entry
	receive_queue_entry_t *queueEntry = (receive_queue_entry_t *)malloc(sizeof(receive_queue_entry_t));
	queueEntry->length = length;
	queueEntry->data = data;
	
	// Insert entry into list
	mutex_acquire(&tcpConnData->receiveQueueMutex);
	if(!tcpConnData->receiveQueueStart)
		tcpConnData->receiveQueueStart = queueEntry;
	if(tcpConnData->receiveQueueEnd)
		tcpConnData->receiveQueueEnd->next = queueEntry;
	tcpConnData->receiveQueueEnd = queueEntry;
	queueEntry->next = 0;
	mutex_release(&tcpConnData->receiveQueueMutex);
	
	/*printf_locked("TCP successfully received %d bytes\n", length);
	for(int i = 0; i < length; ++i)
	{
		if(i % 16 == 0)
			printf_locked("\n");
		printf_locked("%02x ", data[i]);
	}
	printf_locked("\n");*/
	
	// Done, notify LWIP that TCP data was processed
	pbuf_free(p);
	tcp_recved(tcpObj, length);
	return ERR_OK;
}

tcp_handle_t itslwip_connect(const char *targetAddress, int targetPort)
{
	// Convert IP address
	ip4_addr_t targetAddr;
	inet_aton(targetAddress, &targetAddr);
	
	// Ensure synchronized access to LWIP functions
	mutex_acquire(&lwipMutex);
	
	// Create TCP object
	struct tcp_pcb *tcpObj = tcp_new();
	
	// Allocate and assign data object for this connection
	tcp_conn_data_t *tcpConnData = (tcp_conn_data_t *)malloc(sizeof(tcp_conn_data_t));
	tcpConnData->tcpObj = tcpObj;
	tcpConnData->lastError = ERR_OK;
	mutex_init(&tcpConnData->receiveQueueMutex);
	tcpConnData->receiveQueueStart = 0;
	tcpConnData->receiveQueueEnd = 0;
	tcp_arg(tcpObj, (void *)tcpConnData);
	
	// Set error callback
	tcp_err(tcpObj, &handle_tcp_error);
	
	// Set sent callback
	tcp_sent(tcpObj, &handle_tcp_sent);
	
	// Set receive callback
	tcp_recv(tcpObj, &handle_tcp_receive);
	
	// Connect
	tcpConnData->connectState = ERR_INPROGRESS;
	/*err_t err =*/ tcp_connect(tcpObj, &targetAddr, targetPort, &handle_tcp_connected);
	//printf_locked("TCP connect request returned error code %d\n", err);
	
	// LWIP calls are done
	mutex_release(&lwipMutex);
	
	// Wait until connection succeeds
	while(tcpConnData->connectState == ERR_INPROGRESS)
		sys_yield();
	
	// Check error code
	if(tcpConnData->connectState != ERR_OK)
		return -1;
	
	// TODO support more than one simultaneous connection later
	tcpConnDataList = tcpConnData;
	return 0;
}

void itslwip_disconnect(tcp_handle_t tcpHandle)
{
	// Get TCP connection data container
	// TODO support more than one simultaneous connection later
	tcp_conn_data_t *tcpConnData = tcpConnDataList;
	
	// Ensure synchronized access to LWIP functions
	mutex_acquire(&lwipMutex);
	
	// Close connection
	/*err_t err =*/ tcp_close(tcpConnData->tcpObj);
	//printf_locked("TCP close request returned error code %d\n", err);
	
	// TODO use tcp_abort if it fails
	
	// LWIP calls are done
	mutex_release(&lwipMutex);
	
	// Free TCP connection data container
	free(tcpConnData);
	tcpConnDataList = 0;
}

void itslwip_send_string(tcp_handle_t tcpHandle, char *string, int stringLength)
{
	// Send string as raw data
	itslwip_send(tcpHandle, (uint8_t *)string, stringLength);
}

void itslwip_send(tcp_handle_t tcpHandle, uint8_t *data, int dataLength)
{
	// Get TCP connection data container
	// TODO support more than one simultaneous connection later
	tcp_conn_data_t *tcpConnData = tcpConnDataList;
	
	// Ensure synchronized access to LWIP functions
	mutex_acquire(&lwipMutex);
	
	// Pass send data in chunks to LWIP, as its length parameter allows only 2^16 bytes at once
	const uint16_t sendChunkSize = 32768;
	int i = 0;
	while(i < dataLength)
	{
		// Calculate size of next data chunk to send
		uint16_t nextChunkSize = ((dataLength - i) > sendChunkSize ? sendChunkSize : (dataLength - i));
		
		// Add chunk to send queue
		/*err_t err =*/ tcp_write(tcpConnData->tcpObj, &data[i], nextChunkSize, 0);
		//printf_locked("TCP write request returned error code %d\n", err);
		i += nextChunkSize;
	}
	
	// Send remaining parts that LWIP buffered for later transmission
	tcp_output(tcpConnData->tcpObj);
	
	// LWIP calls are done
	mutex_release(&lwipMutex);
}

void itslwip_receive_data(tcp_handle_t tcpHandle, uint8_t *dataBuffer, int dataLength)
{
	// Get TCP connection data container
	// TODO support more than one simultaneous connection later
	tcp_conn_data_t *tcpConnData = tcpConnDataList;
	
	// Poll receive queue until full data block has arrived
	int receivedDataLength = 0;
	int pendingDataLength = dataLength;
	while(receivedDataLength < dataLength)
	{
		// Entry available?
		if(tcpConnData->receiveQueueStart)
		{
			// Copy data from entry; if the entry is larger than the requested data amount, keep the remaining part
			mutex_acquire(&tcpConnData->receiveQueueMutex);
			receive_queue_entry_t *queueEntry = tcpConnData->receiveQueueStart;
			if(queueEntry->length > pendingDataLength)
			{
				// The entry is larger than needed, only copy the first part and move the rest to the beginning
				memcpy(dataBuffer, queueEntry->data, pendingDataLength);
				memmove(queueEntry->data, queueEntry->data + pendingDataLength, queueEntry->length - pendingDataLength);
				queueEntry->length -= pendingDataLength;
				receivedDataLength += pendingDataLength;
				dataBuffer += pendingDataLength;
				//printf_locked("Received %d bytes, %d bytes remaining\n", pendingDataLength, 0);
				pendingDataLength = 0;
			}
			else
			{
				// The entry is consumed entirely, so free it after copying its contents
				memcpy(dataBuffer, queueEntry->data, queueEntry->length);
				receivedDataLength += queueEntry->length;
				dataBuffer += queueEntry->length;
				pendingDataLength -= queueEntry->length;
				//printf_locked("Received %d bytes, %d bytes remaining\n", queueEntry->length, pendingDataLength);
				tcpConnData->receiveQueueStart = queueEntry->next;
				if(!tcpConnData->receiveQueueStart)
					tcpConnData->receiveQueueEnd = 0;
				free(queueEntry->data);
				free(queueEntry);
			}
			mutex_release(&tcpConnData->receiveQueueMutex);
		}
		else
			sys_yield();
	}
}

void itslwip_receive_line(tcp_handle_t tcpHandle, char *lineBuffer, int lineBufferLength)
{
	// Get TCP connection data container
	// TODO support more than one simultaneous connection later
	//tcp_conn_data_t *tcpConnData = tcpConnDataList;
	
	// Receive individual chars until new line is encountered or the buffer is filled up
	// TODO more efficient implementation
	char currentChar;
	int i = 0;
	while(i < lineBufferLength - 1)
	{
		// Receive next char
		itslwip_receive_data(tcpHandle, (uint8_t *)&currentChar, 1);
		if(currentChar == '\n')
			break;
		
		// Add char to the line buffer
		lineBuffer[i++] = currentChar;
	}
	lineBuffer[i] = '\0';
}