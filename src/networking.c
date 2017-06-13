#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/uio.h>
#include <syslog.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "networking.h"
#include "conn_handler.h"

// Length of string to represent maximum port of 65535
#define MAX_PORT_LEN 6

#include "ae.h"

/**
 * Default listen backlog size for
 * our TCP listener.
 */
#define BACKLOG_SIZE 64

/**
 * How big should the default connection
 * buffer size be. One page seems reasonable
 * since most requests will not be this large
 */
#define INIT_CONN_BUF_SIZE 32768

/**
 * This is the scale factor we use when
 * we are growing our connection buffers.
 * We want this to be aggressive enough to reduce
 * the number of resizes, but to also avoid wasted
 * space. With this, we will go from:
 * 32K -> 64K -> 128K
 */
#define CONN_BUF_MULTIPLIER 2

// Macro to provide branch meta-data
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

/**
 * Represents a simple circular buffer
 */
typedef struct {
    int write_cursor;
    int read_cursor;
    uint32_t buf_size;
    char *buffer;
} circular_buffer;

/**
 * Stores the connection specific data.
 * We initialize one of these per connection
 */
struct conn_info {
    int client_fd;
    circular_buffer input;
    statsite_networking *nc;
};
typedef struct conn_info conn_info;

/**
 * Defines a structure that is
 * used to store the state of the networking
 * stack.
 */
struct statsite_networking {
    statsite_config *config;
    aeEventLoop *loop;
    int tcp_listener_fd;
    long long flush_timer;
    conn_info *stdin_client;
    conn_info *udp_client;
};


// Static typedefs
static int handle_flush_event(aeEventLoop *loop, long long id, void *edata);
static void handle_new_client(aeEventLoop *loop, int fd, void *edata, int mask);
static void handle_udp_message(aeEventLoop *loop, int fd, void *edata, int mask);
static void invoke_event_handler(aeEventLoop *loop, int fd, void *edata, int mask);

// Utility methods
static int set_client_sockopts(int client_fd);
static conn_info* get_conn(statsite_networking *nc, int fd);

// Circular buffer method
static void circbuf_init(circular_buffer *buf);
static void circbuf_clear(circular_buffer *buf);
static void circbuf_free(circular_buffer *buf);
static uint64_t circbuf_avail_buf(circular_buffer *buf);
static uint64_t circbuf_used_buf(circular_buffer *buf);
static void circbuf_grow_buf(circular_buffer *buf);
static void circbuf_setup_readv_iovec(circular_buffer *buf, struct iovec *vectors, int *num_vectors);
static void circbuf_advance_write(circular_buffer *buf, uint64_t bytes);
static void circbuf_advance_read(circular_buffer *buf, uint64_t bytes);
static int circbuf_write(circular_buffer *buf, char *in, uint64_t bytes);

/**
 * Initializes the TCP listener
 * @arg netconf The network configuration
 * @return 0 on success.
 */
static int setup_tcp_listener(statsite_networking *netconf) {
    if (netconf->config->tcp_port == 0) {
        syslog(LOG_INFO, "TCP port is disabled");
        return 0;
    }
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s;
    int tcp_listener_fd;
    int optval = 1;
    char tcp_port[MAX_PORT_LEN];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    snprintf(tcp_port, MAX_PORT_LEN, "%d", netconf->config->tcp_port);

    s = getaddrinfo(netconf->config->bind_address, tcp_port, &hints, &result);
    if (s != 0) {
        syslog(LOG_ERR, "getaddrinfo: %s\n", gai_strerror(s));
        return 1;
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        tcp_listener_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (tcp_listener_fd == -1)
            continue;
        if (setsockopt(tcp_listener_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) {
            syslog(LOG_ERR, "Failed to set SO_REUSEADDR! Err: %s", strerror(errno));
            close(tcp_listener_fd);
            continue;
        }
        if (bind(tcp_listener_fd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;
        syslog(LOG_ERR, "Failed to bind on TCP socket! Err: %s", strerror(errno));
        close(tcp_listener_fd);
    }
    if (rp == NULL) {               /* No address succeeded */
        syslog(LOG_ERR, "Failed to bind on any TCP socket!\n");
        return 1;
    }
    freeaddrinfo(result);
    if (listen(tcp_listener_fd, BACKLOG_SIZE) != 0) {
        syslog(LOG_ERR, "Failed to listen on TCP socket! Err: %s", strerror(errno));
        close(tcp_listener_fd);
        return 1;
    }

    syslog(LOG_INFO, "Listening on tcp '%s:%d'",
           netconf->config->bind_address, netconf->config->tcp_port);

    // Create the tcp event handler
    aeCreateFileEvent(netconf->loop, tcp_listener_fd, AE_READABLE, handle_new_client, netconf);
    netconf->tcp_listener_fd = tcp_listener_fd;
    return 0;
}

/**
 * Initializes the UDP Listener.
 * @arg netconf The network configuration
 * @return 0 on success.
 */
static int setup_udp_listener(statsite_networking *netconf) {
    if (netconf->config->udp_port == 0) {
        syslog(LOG_INFO, "UDP port is disabled");
        return 0;
    }
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s;
    int udp_listener_fd;
    int optval = 1;
    char udp_port[MAX_PORT_LEN];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    snprintf(udp_port, MAX_PORT_LEN, "%d", netconf->config->udp_port);

    s = getaddrinfo(netconf->config->bind_address, udp_port, &hints, &result);
    if (s != 0) {
        syslog(LOG_ERR, "getaddrinfo: %s\n", gai_strerror(s));
        return 1;
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        udp_listener_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (udp_listener_fd == -1)
            continue;
        if (setsockopt(udp_listener_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) {
            syslog(LOG_ERR, "Failed to set SO_REUSEADDR! Err: %s", strerror(errno));
            close(udp_listener_fd);
            continue;
        }
        if (bind(udp_listener_fd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;
        syslog(LOG_ERR, "Failed to bind on UDP socket! Err: %s", strerror(errno));
        close(udp_listener_fd);
    }
    if (rp == NULL) {               /* No address succeeded */
        syslog(LOG_ERR, "Failed to bind on any UDP socket!\n");
        return 1;
    }
    freeaddrinfo(result);
    // Put the socket in non-blocking mode
    int flags = fcntl(udp_listener_fd, F_GETFL, 0);
    fcntl(udp_listener_fd, F_SETFL, flags | O_NONBLOCK);

    // Set the RCVBUF socket buffer
    if (netconf->config->udp_rcvbuf) {
        optval = netconf->config->udp_rcvbuf;
        if (setsockopt(udp_listener_fd, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval))) {
            syslog(LOG_ERR, "Failed to set SO_RCVBUF! Err: %s\n", strerror(errno));
        }
    }

    // Allocate a connection object for the UDP socket,
    // ensure a min-buffer size of 64K
    conn_info *conn = get_conn(netconf, udp_listener_fd);
    while (circbuf_avail_buf(&conn->input) < 65536) {
        circbuf_grow_buf(&conn->input);
    }
    netconf->udp_client = conn;

    syslog(LOG_INFO, "Listening on udp '%s:%d'.",
           netconf->config->bind_address, netconf->config->udp_port);

    // Create the udp event handler
    aeCreateFileEvent(netconf->loop, udp_listener_fd, AE_READABLE, handle_udp_message, netconf);
    return 0;
}

/**
 * Initializes the stdin listener.
 * @arg netconf The network configuration
 * @return 0 on success.
 */
static int setup_stdin_listener(statsite_networking *netconf) {
    if (!netconf->config->parse_stdin) {
        syslog(LOG_INFO, "stdin is disabled");
        return 0;
    }

    // Log we are listening
    syslog(LOG_INFO, "Listening on stdin.");

    // Create an associated conn object
    conn_info *conn = get_conn(netconf, STDIN_FILENO);
    netconf->stdin_client = conn;

    // Initialize the stdin event
    aeCreateFileEvent(netconf->loop, STDIN_FILENO, AE_READABLE, invoke_event_handler, conn);
    return 0;
}

/**
 * Adjust flush interval to align with clock
 * @arg flush_interval The flush interval from configuration
 */
static long long align_timer(int flush_interval) {
  long long next_flush_ms = flush_interval * 1000;
  struct timeval now;
  gettimeofday(&now, NULL);
  next_flush_ms -= (now.tv_sec % flush_interval) * 1000;
  next_flush_ms -= (now.tv_usec / 1000);
  if (next_flush_ms <= 0) next_flush_ms = flush_interval * 1000;
  return next_flush_ms;
}

/**
 * Initializes the networking interfaces
 * @arg config Takes the bloom server configuration
 * @arg mgr The filter manager to pass up to the connection handlers
 * @arg netconf Output. The configuration for the networking stack.
 */
int init_networking(statsite_config *config, statsite_networking **netconf_out) {
    // Initialize the netconf structure
    statsite_networking *netconf = calloc(1, sizeof(struct statsite_networking));
    netconf->config = config;

    struct rlimit limit;
    int maxclients = (getrlimit(RLIMIT_NOFILE,&limit) == -1) ? 1024 : limit.rlim_cur;
    netconf->loop = aeCreateEventLoop(maxclients);

    if (!netconf->loop) {
        syslog(LOG_CRIT, "Failed to initialize event loop!");
        free(netconf);
        return 1;
    }

    // Setup the stdin listener
    int res = setup_stdin_listener(netconf);
    if (res != 0) {
        free(netconf);
        return 1;
    }

    // Setup the TCP listener
    res = setup_tcp_listener(netconf);
    if (res != 0) {
        free(netconf);
        return 1;
    }

    // Setup the UDP listener
    res = setup_udp_listener(netconf);
    if (res != 0) {
        aeDeleteFileEvent(netconf->loop, netconf->tcp_listener_fd, AE_READABLE);
        close(netconf->tcp_listener_fd);
        free(netconf);
        return 1;
    }

    // Setup the timer
    long long first_flush_ms = config->flush_interval * 1000;
    if (config->aligned_flush) {
      first_flush_ms = align_timer(config->flush_interval);
    }
    netconf->flush_timer = aeCreateTimeEvent(netconf->loop, first_flush_ms, handle_flush_event, netconf, NULL);

    // Prepare the conn handlers
    init_conn_handler(config);

    // Success!
    *netconf_out = netconf;
    return 0;
}


/**
 * Invoked when our flush timer is reached.
 * We need to instruct the connection handler about this.
 */
static int handle_flush_event(aeEventLoop *loop, long long id, void *edata) {
    statsite_networking *netconf = (statsite_networking *) edata;
    long long next_flush_ms = netconf->config->flush_interval * 1000;
    // Inform the connection handler of the timeout
    flush_interval_trigger();
    if (netconf->config->aligned_flush) {
      next_flush_ms = align_timer(netconf->config->flush_interval);
    }
    return next_flush_ms;
}


/**
 * Invoked when a TCP listening socket fd is ready
 * to accept a new client. Accepts the client, initializes
 * the connection buffers, and stars to listening for data
 */
static void handle_new_client(aeEventLoop *loop, int fd, void *edata, int mask) {
		statsite_networking *netconf = (statsite_networking *) edata;

    // Accept the client connection
    int listen_fd = fd;
    struct sockaddr_in client_addr;
    unsigned int client_addr_len = sizeof(client_addr);
    int client_fd = accept(listen_fd,
                        (struct sockaddr*)&client_addr,
                        &client_addr_len);

    // Check for an error
    if (client_fd == -1) {
        syslog(LOG_ERR, "Failed to accept() connection! %s.", strerror(errno));
        return;
    }

    // Setup the socket
    if (set_client_sockopts(client_fd)) {
        return;
    }

    // Debug info
    syslog(LOG_DEBUG, "Accepted client connection: %s %d [%d]",
            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), client_fd);

    // Get the associated conn object
    conn_info *conn = get_conn(netconf, client_fd);

    // Initialize the libev stuff
    aeCreateFileEvent(netconf->loop, client_fd, AE_READABLE, invoke_event_handler, conn);
}


/**
 * Invoked when a client connection has data ready to be read.
 * We need to take care to add the data to our buffers, and then
 * invoke the connection handlers who have the business logic
 * of what to do.
 */
static int read_client_data(conn_info *conn) {
    /**
     * Figure out how much space we have to write.
     * If we have < 50% free, we resize the buffer using
     * a multiplier.
     */
    int avail_buf = circbuf_avail_buf(&conn->input);
    if (avail_buf < conn->input.buf_size / 2) {
        circbuf_grow_buf(&conn->input);
    }

    // Build the IO vectors to perform the read
    struct iovec vectors[2];
    int num_vectors;
    circbuf_setup_readv_iovec(&conn->input, (struct iovec*)&vectors, &num_vectors);

    // Issue the read
    ssize_t read_bytes = readv(conn->client_fd, (struct iovec*)&vectors, num_vectors);

    // Make sure we actually read something
    if (read_bytes == 0) {
        syslog(LOG_DEBUG, "Closed client connection. [%d]\n", conn->client_fd);
        return 1;
    } else if (read_bytes == -1) {
        // Ignore the error, read again later
        if (errno == EAGAIN || errno == EINTR)
            return 0;

        syslog(LOG_ERR, "Failed to read() from connection [%d]! %s.",
                conn->client_fd, strerror(errno));
        return 1;
    }

    // Update the write cursor
    circbuf_advance_write(&conn->input, read_bytes);
    return 0;
}


/**
 * Invoked when a UDP connection has a message ready to be read.
 * We need to take care to add the data to our buffers, and then
 * invoke the connection handlers who have the business logic
 * of what to do.
 */
static void handle_udp_message(aeEventLoop *loop, int fd, void *edata, int mask) {
    struct iovec vectors[2];
    int num_vectors;
    ssize_t read_bytes;
    statsite_networking *netconf = (statsite_networking *) edata;

    // Get the associated connection struct
    conn_info *conn = netconf->udp_client;

    // Clear the input buffer
    circbuf_clear(&conn->input);

    // Loop until our buffer cannot hold another UDP packet (using a 1500
    // byte MTU) or we cannot read another packet off the wire.  We do not
    // read continuously off of the UDP socket to preserve timer execution.
    while (circbuf_avail_buf(&conn->input) > 1500) {
        // Build the IO vectors to perform the read
        circbuf_setup_readv_iovec(&conn->input, (struct iovec*)&vectors, &num_vectors);

        /*
         * Issue the read, always use the first vector.
         * since we just cleared the buffer, and it should
         * be a contiguous buffer.
         */
        assert(num_vectors == 1);
        read_bytes = recv(fd, vectors[0].iov_base, vectors[0].iov_len, 0);

        // Make sure we actually read something
        if (read_bytes == 0) {
            syslog(LOG_DEBUG, "Got empty UDP packet. [%d]\n", fd);
            continue;

        } else if (read_bytes == -1) {
            if (errno != EAGAIN && errno != EINTR) {
                syslog(LOG_ERR, "Failed to recv() from connection [%d]! %s.",
                        fd, strerror(errno));
            }
            break;
        }

        // Update the write cursor
        circbuf_advance_write(&conn->input, read_bytes);

        // UDP clients don't need to append newlines to the messages like
        // TCP clients do, but our parser requires them.  Append one if
        // it's not present.
        if (conn->input.buffer[conn->input.write_cursor - 1] != '\n')
            circbuf_write(&conn->input, "\n", 1);
    }

    // Invoke the connection handler
    statsite_conn_handler handle = {netconf->config, netconf->udp_client};
    handle_client_connect(&handle);
}


/**
 * Reads the thread specific userdata to figure out what
 * we need to handle. Things that purely effect the network
 * stack should be handled here, but otherwise we should defer
 * to the connection handlers.
 */
static void invoke_event_handler(aeEventLoop *loop, int fd, void *edata, int mask) {
    conn_info *conn = (conn_info *) edata;
    statsite_networking *netconf = conn->nc;

    // Read in the data, and close on issues
    if (read_client_data(conn)) {
        if (fd != STDIN_FILENO)
            close_client_connection(conn);
        return;
    }

    // Invoke the connection handler, and close connection on error
    statsite_conn_handler handle = {netconf->config, conn};
    if (handle_client_connect(&handle) && fd != STDIN_FILENO)
        close_client_connection(conn);
}


/**
 * Entry point for main thread to enter the networking
 * stack. This method blocks indefinitely until the
 * network stack is shutdown.
 * @arg netconf The configuration for the networking stack.
 * @arg signum A reference to a variable that is set when
 * a signal is caught and shutdown should be started
 */
void enter_networking_loop(statsite_networking *netconf, volatile int *signum) {
    // Run forever until we are told to halt
    while (likely(*signum == 0)) {
        aeProcessEvents(netconf->loop, AE_ALL_EVENTS);
    }
    return;
}

/**
 * Shuts down all the connections
 * and listeners and prepares to exit.
 * @arg netconf The config for the networking stack.
 */
int shutdown_networking(statsite_networking *netconf) {
    // Stop listening for new connections
    aeDeleteFileEvent(netconf->loop, netconf->tcp_listener_fd, AE_READABLE);
    close(netconf->tcp_listener_fd);

    if (netconf->udp_client != NULL) {
        close_client_connection(netconf->udp_client);
        netconf->udp_client = NULL;
    }

    if (netconf->stdin_client != NULL) {
        close_client_connection(netconf->stdin_client);
        netconf->stdin_client = NULL;
    }

    // Stop the other timers
    aeDeleteTimeEvent(netconf->loop, netconf->flush_timer);

    // TODO: Close all the client connections
    // ??? For now, we just leak the memory
    // since we are shutdown down anyways...

    // Free the event loop
    aeDeleteEventLoop(netconf->loop);

    // Free the netconf
    free(netconf);
    return 0;
}

/*
 * These are externally visible methods for
 * interacting with the connection buffers.
 */

/**
 * Called to close and cleanup a client connection.
 * Must be called when the connection is not already
 * scheduled. e.g. After event has been canceled.
 * Leaves the connection in the conns list so that it
 * can be re-used.
 * @arg conn The connection to close
 */
void close_client_connection(conn_info *conn) {
    // Stop the events
    aeDeleteFileEvent(conn->nc->loop, conn->client_fd, AE_READABLE);

    // Clear everything out
    circbuf_free(&conn->input);

    // Close the fd
    syslog(LOG_DEBUG, "Closed connection. [%d]", conn->client_fd);
    close(conn->client_fd);
    free(conn);
}


/**
 * This method is used to conveniently extract commands from the
 * command buffer. It scans up to a terminator, and then sets the
 * buf to the start of the buffer, and buf_len to the length
 * of the buffer. The output param should_free indicates that
 * the caller should free the buffer pointed to by buf when it is finished.
 * This method consumes the bytes from the underlying buffer, freeing
 * space for later reads.
 * @arg conn The client connection
 * @arg terminator The terminator charactor to look for. Replaced by null terminator.
 * @arg buf Output parameter, sets the start of the buffer.
 * @arg buf_len Output parameter, the length of the buffer.
 * @arg should_free Output parameter, should the buffer be freed by the caller.
 * @return 0 on success, -1 if the terminator is not found.
 */
int extract_to_terminator(statsite_conn_info *conn, char terminator, char **buf, int *buf_len, int *should_free) {
    // First we need to find the terminator...
    char *term_addr = NULL;
    if (unlikely(conn->input.write_cursor < conn->input.read_cursor)) {
        /*
         * We need to scan from the read cursor to the end of
         * the buffer, and then from the start of the buffer to
         * the write cursor.
        */
        term_addr = memchr(conn->input.buffer+conn->input.read_cursor,
                           terminator,
                           conn->input.buf_size - conn->input.read_cursor);

        // If we've found the terminator, we can just move up
        // the read cursor
        if (term_addr) {
            *buf = conn->input.buffer + conn->input.read_cursor;
            *buf_len = term_addr - *buf + 1;    // Difference between the terminator and location
            *term_addr = '\0';              // Add a null terminator
            *should_free = 0;               // No need to free, in the buffer

            // Push the read cursor forward
            conn->input.read_cursor = (term_addr - conn->input.buffer + 1) % conn->input.buf_size;
            return 0;
        }

        // Wrap around
        term_addr = memchr(conn->input.buffer,
                           terminator,
                           conn->input.write_cursor);

        // If we've found the terminator, we need to allocate
        // a contiguous buffer large enough to store everything
        // and provide a linear buffer
        if (term_addr) {
            int start_size = term_addr - conn->input.buffer + 1;
            int end_size = conn->input.buf_size - conn->input.read_cursor;
            *buf_len = start_size + end_size;
            *buf = malloc(*buf_len);

            // Copy from the read cursor to the end
            memcpy(*buf, conn->input.buffer+conn->input.read_cursor, end_size);

            // Copy from the start to the terminator
            *term_addr = '\0';              // Add a null terminator
            memcpy(*buf+end_size, conn->input.buffer, start_size);

            *should_free = 1;               // Must free, not in the buffer
            conn->input.read_cursor = start_size; // Push the read cursor forward
        }

    } else {
        /*
         * We need to scan from the read cursor to write buffer.
         */
        term_addr = memchr(conn->input.buffer+conn->input.read_cursor,
                           terminator,
                           conn->input.write_cursor - conn->input.read_cursor);

        // If we've found the terminator, we can just move up
        // the read cursor
        if (term_addr) {
            *buf = conn->input.buffer + conn->input.read_cursor;
            *buf_len = term_addr - *buf + 1; // Difference between the terminator and location
            *term_addr = '\0';               // Add a null terminator
            *should_free = 0;                // No need to free, in the buffer
            conn->input.read_cursor = term_addr - conn->input.buffer + 1; // Push the read cursor forward
        }
    }

    // Minor optimization, if our read-cursor has caught up
    // with the write cursor, reset them to the beginning
    // to avoid wrapping in the future
    if (conn->input.read_cursor == conn->input.write_cursor) {
        conn->input.read_cursor = 0;
        conn->input.write_cursor = 0;
    }

    // Return success if we have a term address
    return ((term_addr) ? 0 : -1);
}


/**
 * This method is used to query how much data is available
 * to be read from the command buffer.
 * @arg conn The client connection
 * @return The bytes available
 */
uint64_t available_bytes(statsite_conn_info *conn) {
    // Query the circular buffer
    return circbuf_used_buf(&conn->input);
}

/**
 * Lets the caller look at the next byte
 * @arg conn The client connectoin
 * @arg byte The output byte
 * @return 0 on success, -1 if there is no data.
 */
int peek_client_byte(statsite_conn_info *conn, unsigned char* byte) {
    if (unlikely(!circbuf_used_buf(&conn->input))) return -1;
    *byte = *(unsigned char*)(conn->input.buffer+conn->input.read_cursor);
    return 0;
}

/**
 * This method is used to peek into the input buffer without
 * causing input to be consumed. It attempts to use the data
 * in-place, similar to read_client_bytes.
 * @arg conn The client connection
 * @arg bytes The number of bytes to peek
 * @arg buf Output parameter, sets the start of the buffer.
 * @arg should_free Output parameter, should the buffer be freed by the caller.
 * @return 0 on success, -1 if there is insufficient data.
 */
int peek_client_bytes(statsite_conn_info *conn, int bytes, char** buf, int* should_free) {
    if (unlikely(bytes > circbuf_used_buf(&conn->input))) return -1;

    // Handle the wrap around case
    if (unlikely(conn->input.write_cursor < conn->input.read_cursor)) {
        // Check if we can use a contiguous chunk
        int end_size = conn->input.buf_size - conn->input.read_cursor;
        if (end_size >= bytes) {
            *buf = conn->input.buffer + conn->input.read_cursor;
            *should_free = 0;

        // Otherwise, allocate a dynamic slab, and copy
        } else {
            *buf = malloc(bytes);
            memcpy(*buf, conn->input.buffer + conn->input.read_cursor, end_size);
            memcpy(*buf + end_size, conn->input.buffer, bytes - end_size);
            *should_free = 1;
        }

    // Handle the contiguous case
    } else {
        *buf = conn->input.buffer + conn->input.read_cursor;
        *should_free = 0;
    }

    return 0;
}

/**
 * This method is used to seek the input buffer without
 * consuming input. It can be used in conjunction with
 * peek_client_bytes to conditionally seek.
 * @arg conn The client connection
 * @arg bytes The number of bytes to seek
 * @return 0 on success, -1 if there is insufficient data.
 */
int seek_client_bytes(statsite_conn_info *conn, int bytes) {
    if (unlikely(bytes > circbuf_used_buf(&conn->input))) return -1;
    circbuf_advance_read(&conn->input, bytes);
    return 0;
}


/**
 * This method is used to read and consume the input buffer
 * @arg conn The client connection
 * @arg bytes The number of bytes to read
 * @arg buf Output parameter, sets the start of the buffer.
 * @arg should_free Output parameter, should the buffer be freed by the caller.
 * @return 0 on success, -1 if there is insufficient data.
 */
int read_client_bytes(statsite_conn_info *conn, int bytes, char** buf, int* should_free) {
    if (unlikely(bytes > circbuf_used_buf(&conn->input))) return -1;

    // Handle the wrap around case
    if (unlikely(conn->input.write_cursor < conn->input.read_cursor)) {
        // Check if we can use a contiguous chunk
        int end_size = conn->input.buf_size - conn->input.read_cursor;
        if (end_size >= bytes) {
            *buf = conn->input.buffer + conn->input.read_cursor;
            *should_free = 0;

        // Otherwise, allocate a dynamic slab, and copy
        } else {
            *buf = malloc(bytes);
            memcpy(*buf, conn->input.buffer + conn->input.read_cursor, end_size);
            memcpy(*buf + end_size, conn->input.buffer, bytes - end_size);
            *should_free = 1;
        }

    // Handle the contiguous case
    } else {
        *buf = conn->input.buffer + conn->input.read_cursor;
        *should_free = 0;
    }

    // Advance the read cursor
    circbuf_advance_read(&conn->input, bytes);
    return 0;
}


/**
 * Sets the client socket options.
 * @return 0 on success, 1 on error.
 */
static int set_client_sockopts(int client_fd) {
    // Setup the socket to be non-blocking
    int sock_flags = fcntl(client_fd, F_GETFL, 0);
    if (sock_flags < 0) {
        syslog(LOG_ERR, "Failed to get socket flags on connection! %s.", strerror(errno));
        close(client_fd);
        return 1;
    }
    if (fcntl(client_fd, F_SETFL, sock_flags | O_NONBLOCK)) {
        syslog(LOG_ERR, "Failed to set O_NONBLOCK on connection! %s.", strerror(errno));
        close(client_fd);
        return 1;
    }

    /**
     * Set TCP_NODELAY. This will allow us to send small response packets more
     * quickly, since our responses are rarely large enough to consume a packet.
     */
    int flag = 1;
    if (setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int))) {
        syslog(LOG_WARNING, "Failed to set TCP_NODELAY on connection! %s.", strerror(errno));
    }

    // Set keep alive
    if(setsockopt(client_fd, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(int))) {
        syslog(LOG_WARNING, "Failed to set SO_KEEPALIVE on connection! %s.", strerror(errno));
    }

    return 0;
}


/**
 * Returns the conn_info* object associated with the FD
 * or allocates a new one as necessary.
 */
static conn_info* get_conn(statsite_networking *nc, int fd) {
    // Allocate space
    conn_info *conn = malloc(sizeof(conn_info));

    // Prepare the buffers
    circbuf_init(&conn->input);

    // Store fd and a reference back to netconf
    conn->nc = nc;
    conn->client_fd = fd;

    return conn;
}

/*
 * Methods for manipulating our circular buffers
 */

// Conditionally allocates if there is no buffer
static void circbuf_init(circular_buffer *buf) {
    buf->read_cursor = 0;
    buf->write_cursor = 0;
    buf->buf_size = INIT_CONN_BUF_SIZE * sizeof(char);
    buf->buffer = malloc(buf->buf_size);
}

// Clears the circular buffer, reseting it.
static void circbuf_clear(circular_buffer *buf) {
    buf->read_cursor = 0;
    buf->write_cursor = 0;
}

// Frees a buffer
static void circbuf_free(circular_buffer *buf) {
    if (buf->buffer) free(buf->buffer);
    buf->buffer = NULL;
}

// Calculates the available buffer size
static uint64_t circbuf_avail_buf(circular_buffer *buf) {
    uint64_t avail_buf;
    if (buf->write_cursor < buf->read_cursor) {
        avail_buf = buf->read_cursor - buf->write_cursor - 1;
    } else {
        avail_buf = buf->buf_size - buf->write_cursor + buf->read_cursor - 1;
    }
    return avail_buf;
}

// Calculates the used buffer size
static uint64_t circbuf_used_buf(circular_buffer *buf) {
    uint64_t used_buf;
    if (buf->write_cursor < buf->read_cursor) {
        used_buf = buf->buf_size - buf->read_cursor + buf->write_cursor;
    } else {
        used_buf = buf->write_cursor - buf->read_cursor;
    }
    return used_buf;
}

// Grows the circular buffer to make room for more data
static void circbuf_grow_buf(circular_buffer *buf) {
    int new_size = buf->buf_size * CONN_BUF_MULTIPLIER * sizeof(char);
    char *new_buf = malloc(new_size);
    int bytes_written = 0;

    // Check if the write has wrapped around
    if (buf->write_cursor < buf->read_cursor) {
        // Copy from the read cursor to the end of the buffer
        bytes_written = buf->buf_size - buf->read_cursor;
        memcpy(new_buf,
               buf->buffer+buf->read_cursor,
               bytes_written);

        // Copy from the start to the write cursor
        memcpy(new_buf+bytes_written,
               buf->buffer,
               buf->write_cursor);
        bytes_written += buf->write_cursor;

    // We haven't wrapped yet...
    } else {
        // Copy from the read cursor up to the write cursor
        bytes_written = buf->write_cursor - buf->read_cursor;
        memcpy(new_buf,
               buf->buffer + buf->read_cursor,
               bytes_written);
    }

    // Update the buffer locations and everything
    free(buf->buffer);
    buf->buffer = new_buf;
    buf->buf_size = new_size;
    buf->read_cursor = 0;
    buf->write_cursor = bytes_written;
}


// Initializes a pair of iovectors to be used for readv
static void circbuf_setup_readv_iovec(circular_buffer *buf, struct iovec *vectors, int *num_vectors) {
    // Check if we've wrapped around
    *num_vectors = 1;
    if (buf->write_cursor < buf->read_cursor) {
        vectors[0].iov_base = buf->buffer + buf->write_cursor;
        vectors[0].iov_len = buf->read_cursor - buf->write_cursor - 1;
    } else {
        vectors[0].iov_base = buf->buffer + buf->write_cursor;
        vectors[0].iov_len = buf->buf_size - buf->write_cursor - 1;
        if (buf->read_cursor > 0)  {
            vectors[0].iov_len += 1;
            vectors[1].iov_base = buf->buffer;
            vectors[1].iov_len = buf->read_cursor - 1;
            *num_vectors = 2;
        }
    }
}

// Advances the cursors
static void circbuf_advance_write(circular_buffer *buf, uint64_t bytes) {
    buf->write_cursor = (buf->write_cursor + bytes) % buf->buf_size;
}

static void circbuf_advance_read(circular_buffer *buf, uint64_t bytes) {
    buf->read_cursor = (buf->read_cursor + bytes) % buf->buf_size;

    // Optimization, reset the cursors if they catchup with each other
    if (buf->read_cursor == buf->write_cursor) {
        buf->read_cursor = 0;
        buf->write_cursor = 0;
    }
}

/**
 * Writes the data from a given input buffer
 * into the circular buffer.
 * @return 0 on success.
 */
static int circbuf_write(circular_buffer *buf, char *in, uint64_t bytes) {
    // Check for available space
    uint64_t avail = circbuf_avail_buf(buf);
    while (avail < bytes) {
        circbuf_grow_buf(buf);
        avail = circbuf_avail_buf(buf);
    }

    if (buf->write_cursor < buf->read_cursor) {
        memcpy(buf->buffer+buf->write_cursor, in, bytes);
        buf->write_cursor += bytes;

    } else {
        uint64_t end_size = buf->buf_size - buf->write_cursor;
        if (end_size >= bytes) {
            memcpy(buf->buffer+buf->write_cursor, in, bytes);
            buf->write_cursor += bytes;

        } else {
            // Copy the first end_size bytes
            memcpy(buf->buffer+buf->write_cursor, in, end_size);

            // Copy the remaining data
            memcpy(buf->buffer, in+end_size, (bytes - end_size));
            buf->write_cursor = (bytes - end_size);
        }
    }

    return 0;
}

