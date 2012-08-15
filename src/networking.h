#ifndef NETWORKING_H
#define NETWORKING_H
#include "config.h"

// Network configuration struct
typedef struct statsite_networking statsite_networking;
typedef struct conn_info statsite_conn_info;

/**
 * Initializes the networking interfaces
 * @arg config Takes the statsite server configuration
 * @arg netconf Output. The configuration for the networking stack.
 */
int init_networking(statsite_config *config, statsite_networking **netconf_out);

/**
 * Entry point for threads to join the networking
 * stack. This method blocks indefinitely until the
 * network stack is shutdown.
 * @arg netconf The configuration for the networking stack.
 */
void start_networking_worker(statsite_networking *netconf);

/**
 * Shuts down all the connections
 * and listeners and prepares to exit.
 * @arg netconf The config for the networking stack.
 */
int shutdown_networking(statsite_networking *netconf);

/*
 * Connection related methods. These are exposed so
 * that the connection handlers can manipulate the buffers.
 */

/**
 * Closes the client connection.
 */
void close_client_connection(statsite_conn_info *conn);

/**
 * Sends a response to a client.
 * @arg conn The client connection
 * @arg response_buffers A list of response buffers to send
 * @arg buf_sizes A list of the buffer sizes
 * @arg num_bufs The number of response buffers
 * @return 0 on success.
 */
int send_client_response(statsite_conn_info *conn, char **response_buffers, int *buf_sizes, int num_bufs);

/**
 * This method is used to conveniently extract commands from the
 * command buffer. It scans up to a terminator, and then sets the
 * buf to the start of the buffer, and buf_len to the length
 * of the buffer. The output param should_free indicates that
 * the caller should free the buffer pointed to by buf when it is finished.
 * This method consumes the bytes from the underlying buffer, freeing
 * space for later reads.
 * @arg conn The client connection
 * @arg terminator The terminator charactor to look for. Included in buf.
 * @arg buf Output parameter, sets the start of the buffer.
 * @arg buf_len Output parameter, the length of the buffer.
 * @arg should_free Output parameter, should the buffer be freed by the caller.
 * @return 0 on success, -1 if the terminator is not found.
 */
int extract_to_terminator(statsite_conn_info *conn, char terminator, char **buf, int *buf_len, int *should_free);

/**
 * This method is used to query how much data is available
 * to be read from the command buffer.
 * @arg conn The client connection
 * @return The bytes available
 */
uint64_t available_bytes(statsite_conn_info *conn);

/**
 * This method is used to peek into the input buffer without
 * causing input to be consumed.
 * @arg conn The client connection
 * @arg bytes The number of bytes to peek
 * @arg buf The output buffer to write to
 * @return 0 on success, -1 if there is insufficient data.
 */
int peek_client_bytes(statsite_conn_info *conn, int bytes, char* buf);

#endif
