#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <syslog.h>
#include <unistd.h>

#include "networking.h"
#include "conn_handler.h"

#define EV_STANDALONE 1
#define EV_API_STATIC 1
#define EV_COMPAT3 0
#define EV_MULTIPLICITY 0
#define EV_USE_MONOTONIC 1
#ifdef __linux__
#define EV_USE_CLOCK_SYSCALL 0
#define EV_USE_EPOLL 1
#endif
#ifdef __MACH__
#define EV_USE_KQUEUE 1
#endif
#include "ev.c"

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
 * Stores the thread specific user data.
 */
typedef struct {
    statsite_networking *netconf;
} worker_ev_userdata;

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
    ev_io client;
    circular_buffer input;
};
typedef struct conn_info conn_info;

/**
 * Defines a structure that is
 * used to store the state of the networking
 * stack.
 */
struct statsite_networking {
    statsite_config *config;
    ev_io tcp_client;
    ev_io udp_client;
    conn_info *stdin_client;
    ev_timer flush_timer;
};


// Static typedefs
static void handle_flush_event(ev_timer *watcher, int revents);
static void handle_new_client(ev_io *watcher, int ready_events);
static void handle_udp_message(ev_io *watch, int ready_events);
static void invoke_event_handler(ev_io *watch, int ready_events);

// Utility methods
static int set_client_sockopts(int client_fd);
static conn_info* get_conn();

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
    struct sockaddr_in addr;
    struct in_addr bind_addr;
    bzero(&addr, sizeof(addr));
    bzero(&bind_addr, sizeof(bind_addr));
    addr.sin_family = PF_INET;
    addr.sin_port = htons(netconf->config->tcp_port);

    int ret = inet_pton(AF_INET, netconf->config->bind_address, &bind_addr);
    if (ret != 1) {
        syslog(LOG_ERR, "Invalid IPv4 address '%s'!", netconf->config->bind_address);
        return 1;
    }
    addr.sin_addr = bind_addr;

    // Make the socket, bind and listen
    int tcp_listener_fd = socket(PF_INET, SOCK_STREAM, 0);
    int optval = 1;
    if (setsockopt(tcp_listener_fd, SOL_SOCKET,
                SO_REUSEADDR, &optval, sizeof(optval))) {
        syslog(LOG_ERR, "Failed to set SO_REUSEADDR! Err: %s", strerror(errno));
        close(tcp_listener_fd);
        return 1;
    }
    if (bind(tcp_listener_fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        syslog(LOG_ERR, "Failed to bind on TCP socket! Err: %s", strerror(errno));
        close(tcp_listener_fd);
        return 1;
    }
    if (listen(tcp_listener_fd, BACKLOG_SIZE) != 0) {
        syslog(LOG_ERR, "Failed to listen on TCP socket! Err: %s", strerror(errno));
        close(tcp_listener_fd);
        return 1;
    }

    syslog(LOG_INFO, "Listening on tcp '%s:%d'",
           netconf->config->bind_address, netconf->config->tcp_port);

    // Create the libev objects
    ev_io_init(&netconf->tcp_client, handle_new_client,
                tcp_listener_fd, EV_READ);
    ev_io_start(&netconf->tcp_client);
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
    struct sockaddr_in addr;
    struct in_addr bind_addr;
    bzero(&addr, sizeof(addr));
    bzero(&bind_addr, sizeof(bind_addr));
    addr.sin_family = PF_INET;
    addr.sin_port = htons(netconf->config->udp_port);

    int ret = inet_pton(AF_INET, netconf->config->bind_address, &bind_addr);
    if (ret != 1) {
        syslog(LOG_ERR, "Invalid IPv4 address '%s'!", netconf->config->bind_address);
        return 1;
    }
    addr.sin_addr = bind_addr;

    // Make the socket, bind and listen
    int udp_listener_fd = socket(PF_INET, SOCK_DGRAM, 0);
    int optval = 1;
    if (setsockopt(udp_listener_fd, SOL_SOCKET,
                SO_REUSEADDR, &optval, sizeof(optval))) {
        syslog(LOG_ERR, "Failed to set SO_REUSEADDR! Err: %s", strerror(errno));
        close(udp_listener_fd);
        return 1;
    }
    if (bind(udp_listener_fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        syslog(LOG_ERR, "Failed to bind on UDP socket! Err: %s", strerror(errno));
        close(udp_listener_fd);
        return 1;
    }

    // Put the socket in non-blocking mode
    int flags = fcntl(udp_listener_fd, F_GETFL, 0);
    fcntl(udp_listener_fd, F_SETFL, flags | O_NONBLOCK);

    // Allocate a connection object for the UDP socket,
    // ensure a min-buffer size of 64K
    conn_info *conn = get_conn();
    while (circbuf_avail_buf(&conn->input) < 65536) {
        circbuf_grow_buf(&conn->input);
    }
    netconf->udp_client.data = conn;

    syslog(LOG_INFO, "Listening on udp '%s:%d'.",
           netconf->config->bind_address, netconf->config->udp_port);

    // Create the libev objects
    ev_io_init(&netconf->udp_client, handle_udp_message,
                udp_listener_fd, EV_READ);
    ev_io_start(&netconf->udp_client);
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
    conn_info *conn = get_conn();
    netconf->stdin_client = conn;

    // Initialize the libev stuff
    ev_io_init(&conn->client, invoke_event_handler, STDIN_FILENO, EV_READ);
    ev_io_start(&conn->client);
    return 0;
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

    /**
     * Check if we can use kqueue instead of select.
     * By default, libev will not use kqueue since it only
     * works for sockets, which is all we need.
     */
    int ev_mode = EVFLAG_AUTO;
    if (ev_supported_backends () & ~ev_recommended_backends () & EVBACKEND_KQUEUE) {
        ev_mode = EVBACKEND_KQUEUE;
    }

    if (!ev_default_loop (ev_mode)) {
        syslog(LOG_CRIT, "Failed to initialize libev!");
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
        if (ev_is_active(&netconf->tcp_client)) {
            ev_io_stop(&netconf->tcp_client);
            close(netconf->tcp_client.fd);
        }
        free(netconf);
        return 1;
    }

    // Setup the timer
    ev_timer_init(&netconf->flush_timer, handle_flush_event, config->flush_interval, config->flush_interval);
    ev_timer_start(&netconf->flush_timer);

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
static void handle_flush_event(ev_timer *watcher, int revents) {
    // Inform the connection handler of the timeout
    flush_interval_trigger();
}


/**
 * Invoked when a TCP listening socket fd is ready
 * to accept a new client. Accepts the client, initializes
 * the connection buffers, and stars to listening for data
 */
static void handle_new_client(ev_io *watcher, int ready_events) {
    // Accept the client connection
    int listen_fd = watcher->fd;
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
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
    conn_info *conn = get_conn();

    // Initialize the libev stuff
    ev_io_init(&conn->client, invoke_event_handler, client_fd, EV_READ);
    ev_io_start(&conn->client);
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
    ssize_t read_bytes = readv(conn->client.fd, (struct iovec*)&vectors, num_vectors);

    // Make sure we actually read something
    if (read_bytes == 0) {
        syslog(LOG_DEBUG, "Closed client connection. [%d]\n", conn->client.fd);
        return 1;
    } else if (read_bytes == -1) {
        // Ignore the error, read again later
        if (errno == EAGAIN || errno == EINTR)
            return 0;

        syslog(LOG_ERR, "Failed to read() from connection [%d]! %s.",
                conn->client.fd, strerror(errno));
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
static void handle_udp_message(ev_io *watch, int ready_events) {
    while (1) {
        // Get the associated connection struct
        conn_info *conn = watch->data;

        // Clear the input buffer
        circbuf_clear(&conn->input);

        // Build the IO vectors to perform the read
        struct iovec vectors[2];
        int num_vectors;
        circbuf_setup_readv_iovec(&conn->input, (struct iovec*)&vectors, &num_vectors);

        /*
         * Issue the read, always use the first vector.
         * since we just cleared the buffer, and it should
         * be a contiguous buffer.
         */
        assert(num_vectors == 1);
        ssize_t read_bytes = recv(watch->fd, vectors[0].iov_base,
                                    vectors[0].iov_len, 0);

        // Make sure we actually read something
        if (read_bytes == 0) {
            syslog(LOG_DEBUG, "Got empty UDP packet. [%d]\n", watch->fd);
            return;

        } else if (read_bytes == -1) {
            if (errno != EAGAIN && errno != EINTR) {
                syslog(LOG_ERR, "Failed to recv() from connection [%d]! %s.",
                        watch->fd, strerror(errno));
            }
            return;
        }

        // Update the write cursor
        circbuf_advance_write(&conn->input, read_bytes);

        // UDP clients don't need to append newlines to the messages like
        // TCP clients do, but our parser requires them.  Append one if
        // it's not present.
        if (conn->input.buffer[conn->input.write_cursor - 1] != '\n')
            circbuf_write(&conn->input, "\n", 1);

        // Get the user data
        worker_ev_userdata *data = ev_userdata();

        // Invoke the connection handler
        statsite_conn_handler handle = {data->netconf->config, watch->data};
        handle_client_connect(&handle);
    }
}


/**
 * Reads the thread specific userdata to figure out what
 * we need to handle. Things that purely effect the network
 * stack should be handled here, but otherwise we should defer
 * to the connection handlers.
 */
static void invoke_event_handler(ev_io *watcher, int ready_events) {
    // Get the user data
    worker_ev_userdata *data = ev_userdata();

    // Read in the data, and close on issues
    conn_info *conn = watcher->data;
    if (read_client_data(conn)) {
        close_client_connection(conn);
        return;
    }

    // Invoke the connection handler, and close connection on error
    statsite_conn_handler handle = {data->netconf->config, watcher->data};
    if (handle_client_connect(&handle))
        close_client_connection(conn);
}


/**
 * Entry point for main thread to enter the networking
 * stack. This method blocks indefinitely until the
 * network stack is shutdown.
 * @arg netconf The configuration for the networking stack.
 * @arg should_run A reference to a variable that is set to 0 when
 * shutdown should be started
 */
void enter_networking_loop(statsite_networking *netconf, int *should_run) {
    // Allocate our user data
    worker_ev_userdata data;
    data.netconf = netconf;

    // Set the user data to be for this thread
    ev_set_userdata(&data);

    // Run forever until we are told to halt
    while (likely(*should_run)) {
        ev_run(EVRUN_ONCE);
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
    if (ev_is_active(&netconf->tcp_client)) {
        ev_io_stop(&netconf->tcp_client);
        close(netconf->tcp_client.fd);
    }
    if (ev_is_active(&netconf->udp_client)) {
        ev_io_stop(&netconf->udp_client);
        close(netconf->udp_client.fd);
    }
    if (netconf->stdin_client != NULL) {
        close_client_connection(netconf->stdin_client);
        netconf->stdin_client = NULL;
    }

    // Stop the other timers
    ev_timer_stop(&netconf->flush_timer);

    // TODO: Close all the client connections
    // ??? For now, we just leak the memory
    // since we are shutdown down anyways...

    // Free the event loop
    ev_loop_destroy(EV_DEFAULT);

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
 * scheduled. e.g. After ev_io_stop() has been called.
 * Leaves the connection in the conns list so that it
 * can be re-used.
 * @arg conn The connection to close
 */
void close_client_connection(conn_info *conn) {
    // Stop the libev clients
    ev_io_stop(&conn->client);

    // Clear everything out
    circbuf_free(&conn->input);

    // Close the fd
    syslog(LOG_DEBUG, "Closed connection. [%d]", conn->client.fd);
    close(conn->client.fd);
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
static conn_info* get_conn() {
    // Allocate space
    conn_info *conn = malloc(sizeof(conn_info));

    // Prepare the buffers
    circbuf_init(&conn->input);

    // Store a reference to the conn object
    conn->client.data = conn;
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
            memcpy(buf->buffer, in, (bytes - end_size));
            buf->write_cursor = (bytes - end_size);
        }
    }

    return 0;
}

