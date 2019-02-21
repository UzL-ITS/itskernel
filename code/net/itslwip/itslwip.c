/*
ITS kernel LWIP port main file.
*/

/* INCLUDES */

#include "itslwip.h"
#include <internal/syscall/syscalls.h>
#include <threading/lock.h>
#include <threading/thread.h>
#include "itsnetif.h"

#include "lwip/inet.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
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

// Contains state data of a connection.
typedef struct
{
	// Error code of the last callback.
	err_t lastError;
	
	// Determines whether this is a TCP or UDP connection.
	bool isUdp;
	
	// The LWIP TCP object of this connection.
	struct tcp_pcb *tcpObj;
	
	// The LWIP UDP object of this connection.
	struct udp_pcb *udpObj;
	
	// The state of the connect() operation (ERR_INPROGRESS until completed).
	err_t connectState;
	
	// Mutex for receive queue access.
	mutex_t receiveQueueMutex;
	
	// The first entry of the receive queue.
	receive_queue_entry_t *receiveQueueStart;
	
	// The last entry of the receive queue.
	receive_queue_entry_t *receiveQueueEnd;
	
	// The amount of bytes waiting for being sent.
	int sendQueueSize;
} conn_data_t;


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
static conn_data_t *connDataList;

// Mutex to ensure synchronized access to the LWIP functions by this thread and the caller thread.
static mutex_t lwipMutex;

// Buffer for receiving packets.
static uint8_t packetBuffer[ETHERNET_PACKET_SIZE];


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

// Polls for new packets and sends ticks to LWIP.
// This function needs to be protected with global mutex!
static void itslwip_poll()
{
	// Packet received?
	int packetLength = sys_receive_network_packet(packetBuffer);
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
}

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
	while(true)
	{
		mutex_acquire(&lwipMutex);
		itslwip_poll();
		mutex_release(&lwipMutex);
	}
}

static void handle_tcp_error(void *arg, err_t err)
{
	// Get connection data container
	//conn_data_t *connData = (conn_data_t *)arg;
	
	// Set error code
	printf_locked("LWIP connection error: %d\n", err);
	//connData->lastError = err;      <- causes page fault if connection was terminated already
}

static err_t handle_tcp_connected(void *arg, struct tcp_pcb *tcpObj, err_t err)
{
	// Get connection data container
	conn_data_t *connData = (conn_data_t *)arg;
	
	// Set error code
	//printf_locked("TCP connected with error code %d\n", err);
	connData->lastError = err;
	connData->connectState = err;
	
	return ERR_OK;
}

static err_t handle_tcp_sent(void *arg, struct tcp_pcb *tcpObj, u16_t len)
{
	// Get connection data container
	conn_data_t *connData = (conn_data_t *)arg;
	
	// Debug output
	//printf_locked("TCP successfully sent %d bytes\n", len);
	
	// Update queue size
	connData->sendQueueSize -= len;
	
	return ERR_OK;
}

static err_t handle_tcp_receive(void *arg, struct tcp_pcb *tcpObj, struct pbuf *p, err_t err)
{
	// Get connection data container
	conn_data_t *connData = (conn_data_t *)arg;
	
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
	mutex_acquire(&connData->receiveQueueMutex);
	if(!connData->receiveQueueStart)
		connData->receiveQueueStart = queueEntry;
	if(connData->receiveQueueEnd)
		connData->receiveQueueEnd->next = queueEntry;
	connData->receiveQueueEnd = queueEntry;
	queueEntry->next = 0;
	mutex_release(&connData->receiveQueueMutex);
	
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

static void handle_udp_receive(void *arg, struct udp_pcb *udpObj, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	// Get connection data container
	conn_data_t *connData = (conn_data_t *)arg;
	
	// Calculate length of received data
	int length = 0;
	for(struct pbuf *q = p; q != 0; q = q->next)
		if(q->payload)
			length += q->len;
	if(!length)
	{
		//printf_locked("UDP receive function call without data\n");
		return;
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
	mutex_acquire(&connData->receiveQueueMutex);
	if(!connData->receiveQueueStart)
		connData->receiveQueueStart = queueEntry;
	if(connData->receiveQueueEnd)
		connData->receiveQueueEnd->next = queueEntry;
	connData->receiveQueueEnd = queueEntry;
	queueEntry->next = 0;
	mutex_release(&connData->receiveQueueMutex);
	
	printf_locked("UDP successfully received %d bytes\n", length);
	for(int i = 0; i < length; ++i)
	{
		if(i % 16 == 0)
			printf_locked("\n");
		printf_locked("%02x ", data[i]);
	}
	printf_locked("\n");
	
	// Done
	pbuf_free(p);
}

conn_handle_t itslwip_connect(const char *targetAddress, int targetPort, bool useUdp)
{
	// Convert IP address
	ip4_addr_t targetAddr;
	inet_aton(targetAddress, &targetAddr);
	
	// Allocate and assign data object for this connection
	conn_data_t *connData = (conn_data_t *)malloc(sizeof(conn_data_t));
	connData->lastError = ERR_OK;
	mutex_init(&connData->receiveQueueMutex);
	connData->receiveQueueStart = 0;
	connData->receiveQueueEnd = 0;
	connData->sendQueueSize = 0;
	connData->isUdp = useUdp;
	
	// Ensure synchronized access to LWIP functions
	mutex_acquire(&lwipMutex);
	
	// Initialize LWIP object
	if(!useUdp)
	{
		// Create TCP object
		struct tcp_pcb *tcpObj = tcp_new();
		connData->tcpObj = tcpObj;
		tcp_arg(tcpObj, (void *)connData);
		
		// Set error callback
		tcp_err(tcpObj, &handle_tcp_error);
		
		// Set sent callback
		tcp_sent(tcpObj, &handle_tcp_sent);
		
		// Set receive callback
		tcp_recv(tcpObj, &handle_tcp_receive);
		
		// Connect
		connData->connectState = ERR_INPROGRESS;
		/*err_t err =*/ tcp_connect(tcpObj, &targetAddr, targetPort, &handle_tcp_connected);
		//printf_locked("TCP connect request returned error code %d\n", err);
	}
	else
	{
		// Create UDP object
		struct udp_pcb *udpObj = udp_new();
		udpObj->ttl = UDP_TTL;
		connData->udpObj = udpObj;
		
		// Set receive callback
		udp_recv(udpObj, &handle_udp_receive, (void *)connData);
		
		// Bind to local port
		/*err_t err =*/ udp_bind(udpObj, &address, targetPort);
		//printf_locked("UDP bind request returned error code %d\n", err);
		
		// Connect
		connData->connectState = udp_connect(udpObj, &targetAddr, targetPort);
		//printf_locked("UDP connect request returned error code %d\n", connData->connectState);
	}
	
	// LWIP calls are done
	mutex_release(&lwipMutex);
	
	// Wait until connection succeeds
	while(connData->connectState == ERR_INPROGRESS)
		sys_yield();
	
	// Check error code
	if(connData->connectState != ERR_OK)
		return -1;
	
	// TODO support more than one simultaneous connection later
	connDataList = connData;
	return 0;
}

void itslwip_disconnect(conn_handle_t connHandle)
{
	// Get connection data container
	// TODO support more than one simultaneous connection later
	conn_data_t *connData = connDataList;
	
	// Ensure synchronized access to LWIP functions
	mutex_acquire(&lwipMutex);
	
	// Close connection
	if(!connData->isUdp)
	{
		/*err_t err =*/ tcp_close(connData->tcpObj);
		//printf_locked("TCP close request returned error code %d\n", err);
		
		// TODO use tcp_abort if it fails
	}
	else
		udp_remove(connData->udpObj);
	
	// LWIP calls are done
	mutex_release(&lwipMutex);
	
	// Free connection data container
	free(connData);
	connDataList = 0;
}

void itslwip_send_string(conn_handle_t connHandle, char *string, int stringLength)
{
	// Send string as raw data
	itslwip_send(connHandle, (uint8_t *)string, stringLength);
}

void itslwip_send(conn_handle_t connHandle, uint8_t *data, int dataLength)
{
	// Get connection data container
	// TODO support more than one simultaneous connection later
	conn_data_t *connData = connDataList;
	
	// Ensure synchronized access to LWIP functions
	mutex_acquire(&lwipMutex);
	
	// Send data
	if(!connData->isUdp)
	{
		// Pass send data in chunks to LWIP
		uint16_t sendChunkSize = tcp_sndbuf(connData->tcpObj);
		int i = 0;
		while(i < dataLength)
		{
			// Calculate size of next data chunk to send
			uint16_t nextChunkSize = ((dataLength - i) > sendChunkSize ? sendChunkSize : (dataLength - i));
			
			// Add chunk to send queue
			while(tcp_write(connData->tcpObj, &data[i], nextChunkSize, 0) != ERR_OK)
			{
				// Give TCP time to send data and receive ACK messages
				itslwip_poll();
			}
			i += nextChunkSize;
			connData->sendQueueSize += nextChunkSize;
		}
		
		// Send remaining parts that LWIP buffered for later transmission
		tcp_output(connData->tcpObj);
	}
	else
	{
		// Allocate memory for send buffer
		uint16_t sendChunkSize = 1024;
		struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, sendChunkSize, PBUF_RAM);
		if(!p)
		{
			printf_locked("Could not allocate pbuf for sending\n");
			return;
		}
		
		// Send data piece by piece
		int i = 0;
		while(i < dataLength)
		{
			// Calculate size of next data chunk to send
			uint16_t nextChunkSize = ((dataLength - i) > sendChunkSize ? sendChunkSize : (dataLength - i));
			
			// Send chunk
			memcpy(p->payload, &data[i], nextChunkSize);
			p->len = nextChunkSize;
			p->tot_len = nextChunkSize;
			printf_locked("Sending %d bytes...\n", p->len);
			err_t err = udp_send(connData->udpObj, p);
			if(err != ERR_OK)
			{
				printf_locked("UDP send failed: %d\n", err);
				return;
			}
			i += nextChunkSize;
		}
		
		// Free send buffer
		pbuf_free(p);
	}
	
	// LWIP calls are done
	mutex_release(&lwipMutex);
	
	// Wait until all data was sent
	while(connData->sendQueueSize > 0)
		sys_yield();
}

void itslwip_receive_data(conn_handle_t connHandle, uint8_t *dataBuffer, int dataLength)
{
	// Get connection data container
	// TODO support more than one simultaneous connection later
	conn_data_t *connData = connDataList;
	
	// Poll receive queue until full data block has arrived
	int receivedDataLength = 0;
	int pendingDataLength = dataLength;
	while(receivedDataLength < dataLength)
	{
		// Entry available?
		if(connData->receiveQueueStart)
		{
			// Copy data from entry; if the entry is larger than the requested data amount, keep the remaining part
			mutex_acquire(&connData->receiveQueueMutex);
			receive_queue_entry_t *queueEntry = connData->receiveQueueStart;
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
				connData->receiveQueueStart = queueEntry->next;
				if(!connData->receiveQueueStart)
					connData->receiveQueueEnd = 0;
				free(queueEntry->data);
				free(queueEntry);
			}
			mutex_release(&connData->receiveQueueMutex);
		}
		else
			sys_yield();
	}
}

void itslwip_receive_line(conn_handle_t connHandle, char *lineBuffer, int lineBufferLength)
{
	// Get connection data container
	// TODO support more than one simultaneous connection later
	//conn_data_t *connData = connDataList;
	
	// Receive individual chars until new line is encountered or the buffer is filled up
	// TODO more efficient implementation
	char currentChar;
	int i = 0;
	while(i < lineBufferLength - 1)
	{
		// Receive next char
		itslwip_receive_data(connHandle, (uint8_t *)&currentChar, 1);
		if(currentChar == '\n')
			break;
		
		// Add char to the line buffer
		lineBuffer[i++] = currentChar;
	}
	lineBuffer[i] = '\0';
}