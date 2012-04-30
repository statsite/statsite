#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <assert.h>
#include <regex.h>
#include "conn_handler.h"

/* Static regexes */
static regex_t VALID_METRIC;
static const char *VALID_METRIC_PATTERN= "^([a-zA-Z0-9_.-]+):(-?[0-9.]+)\\|([a-z]+)(\\|@([0-9.]+))?$";

/* Static method declarations */
static int buffer_after_terminator(char *buf, int buf_len, char terminator, char **after_term, int *after_len);

/**
 * Invoked to initialize the conn handler layer.
 */
void init_conn_handler() {
    // Compile our regexes
    int res;
    res = regcomp(&VALID_METRIC, VALID_METRIC_PATTERN, REG_EXTENDED);
    assert(res == 0);
}


/**
 * Invoked by the networking layer when there is new
 * data to be handled. The connection handler should
 * consume all the input possible, and generate responses
 * to all requests.
 * @arg handle The connection related information
 * @return 0 on success.
 */
int handle_client_connect(statsite_conn_handler *handle) {
    // Look for the next command line
    char *buf;
    int buf_len, should_free;
    int status;
    while (1) {
        status = extract_to_terminator(handle->conn, '\n', &buf, &buf_len, &should_free);
        if (status == -1) return 0; // Return if no command is available

        // Make sure to free the command buffer if we need to
        if (should_free) free(buf);
    }

    return 0;
}


/**
 * Scans the input buffer of a given length up to a terminator.
 * Then sets the start of the buffer after the terminator including
 * the length of the after buffer.
 * @arg buf The input buffer
 * @arg buf_len The length of the input buffer
 * @arg terminator The terminator to scan to. Replaced with the null terminator.
 * @arg after_term Output. Set to the byte after the terminator.
 * @arg after_len Output. Set to the length of the output buffer.
 * @return 0 if terminator found. -1 otherwise.
 */
static int buffer_after_terminator(char *buf, int buf_len, char terminator, char **after_term, int *after_len) {
    // Scan for a space
    char *term_addr = memchr(buf, terminator, buf_len);
    if (!term_addr) {
        *after_term = NULL;
        return -1;
    }

    // Convert the space to a null-seperator
    *term_addr = '\0';

    // Provide the arg buffer, and arg_len
    *after_term = term_addr+1;
    *after_len = buf_len - (term_addr - buf + 1);
    return 0;
}

