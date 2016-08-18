#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "streaming.h"

// Struct to hold the callback info
struct callback_info {
    FILE *f;
    void *data;
    stream_callback cb;
};

/**
 * Local callback that invokes the user specified callback with the pip
 */
static int stream_cb(void *data, metric_type type, char *name, void *val) {
    struct callback_info *info = data;
    return info->cb(info->f, info->data, type, name, val);
}

/**
 * Streams the metrics stored in a metrics object to an external command
 * @arg m The metrics object to stream
 * @arg data An opaque handle passed to the callback
 * @arg cb The callback to invoke
 * @arg cmd The command to invoke, invoked with a shell.
 * @return 0 on success, or the value of stream callback.
 */
int stream_to_command(metrics *m, void *data, stream_callback cb, char *cmd) {
    // Create a pipe to the child
    int filedes[2] = {0, 0};
    int res = pipe(filedes);
    if (res < 0) return res;

    // Fork and exec
    int status = 0;
    pid_t pid = fork();
    if (pid < 0) return res;

    // Check if we are the child
    if (pid == 0) {
        // Set stdin to the pipe
        if (dup2(filedes[0], STDIN_FILENO)){
            perror("Failed to initialize stdin!");
            exit(250);
        }
        close(filedes[1]);

        // Try to run the command
        res = execl("/bin/sh", "sh", "-c", cmd, NULL);
        if (res != 0) perror("Failed to execute command!");

        // Always exit
        exit(255);
    } else {
        // Close the read end
        close(filedes[0]);
        waitpid(pid, &status, WNOHANG);
    }

    // Create a file wrapper
    FILE *f = fdopen(filedes[1], "w");

    // Wrap the relevant pointers
    struct callback_info info = {f, data, cb};

    // Start iterating
    metrics_iter(m, &info, stream_cb);

    // Close everything out
    fclose(f);
    close(filedes[1]);

    // Wait for termination
    do {
        if (waitpid(pid, &status, 0) < 0) break;
        usleep(100000);
    } while (!WIFEXITED(status));

    // Return the result of the process
    return WEXITSTATUS(status);
}

