#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <talloc.h>

#ifdef HANDLEBARSC_HAVE_EXEC_HELPERS
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#endif

#ifdef HANDLEBARS_HAVE_JSON
#include <json.h>
#endif

#include "handlebars.h"
#include "handlebars_closure.h"
#include "handlebars_compiler.h"
#include "handlebars_helpers.h"
#include "handlebars_json.h"
#include "handlebars_value_private.h"

#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"

#include "handlebarsc_helpers.h"

#ifdef HANDLEBARSC_HAVE_EXEC_HELPERS
extern char ** environ;
#endif

enum handlebarsc_helper_local {
    handlebarsc_helper_local_name = 0,
    handlebarsc_helper_local_command,
    handlebarsc_helper_local_mode,
    handlebarsc_helper_local_timeout,
    handlebarsc_helper_local_output_limit,
    handlebarsc_helper_local_count
};

enum handlebarsc_helper_run_error {
    handlebarsc_helper_run_success = 0,
    handlebarsc_helper_run_unsupported,
    handlebarsc_helper_run_system,
    handlebarsc_helper_run_spawn,
    handlebarsc_helper_run_timeout,
    handlebarsc_helper_run_output_limit
};

struct handlebarsc_helper_run_result {
    char * output;
    size_t output_length;
    int child_status;
    int error_number;
    enum handlebarsc_helper_run_error error;
};

#ifdef HANDLEBARS_HAVE_JSON
struct handlebarsc_tracked_json {
    struct json_object * object;
    bool owned;
    struct handlebarsc_tracked_json * next;
};
#endif

struct handlebarsc_helper_dispatch_state {
    struct handlebars_vm * vm;
    struct handlebars_value * rv;
    struct handlebars_value result;
    bool result_initialized;
    void * owner;
    struct handlebarsc_helper_run_result run;
#ifdef HANDLEBARS_HAVE_JSON
    struct handlebarsc_tracked_json * json;
    const void * active[HANDLEBARS_VALUE_MAX_DEPTH];
    size_t active_count;
#endif
};

static void handlebarsc_helper_dispatch_cleanup(
    struct handlebarsc_helper_dispatch_state * state
)
{
#ifdef HANDLEBARS_HAVE_JSON
    struct handlebarsc_tracked_json * json = state->json;

    while( json != NULL ) {
        if( json->owned && json->object != NULL ) {
            json_object_put(json->object);
        }
        json = json->next;
    }
#endif
    if( state->result_initialized ) {
        handlebars_value_dtor(&state->result);
        state->result_initialized = false;
    }
    free(state->run.output);
    state->run.output = NULL;
    if( state->owner != NULL ) {
        handlebars_talloc_free(state->owner);
        state->owner = NULL;
    }
}

static HBS_ATTR_NORETURN void handlebarsc_helper_rethrow(
    struct handlebars_context * context,
    jmp_buf * target,
    enum handlebars_error_type caught
)
{
    struct handlebars_value_iterator * iterator = context->e->iterator_cleanup;

    while( iterator != NULL ) {
        struct handlebars_value_iterator * next = iterator->unwind_next;

        if( iterator->unwind_target == target ) {
            handlebars_value_iterator_close(iterator);
        }
        iterator = next;
    }
    longjmp(*target, caught);
}

#ifdef HANDLEBARSC_HAVE_EXEC_HELPERS
static bool handlebarsc_helper_set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    return flags >= 0 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

static bool handlebarsc_helper_set_close_on_exec(int fd)
{
    int flags = fcntl(fd, F_GETFD, 0);

    return flags >= 0 && fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == 0;
}

static bool handlebarsc_helper_pipe(int descriptors[2])
{
    int pipe_descriptors[2] = { -1, -1 };
    int saved_errno;

    if( pipe(pipe_descriptors) != 0 ) {
        return false;
    }
    for( size_t i = 0; i < 2; i++ ) {
        if( pipe_descriptors[i] <= STDERR_FILENO ) {
            int descriptor = fcntl(
                pipe_descriptors[i],
                F_DUPFD,
                STDERR_FILENO + 1
            );

            if( descriptor < 0 ) {
                goto fail;
            }
            close(pipe_descriptors[i]);
            pipe_descriptors[i] = descriptor;
        }
        if( !handlebarsc_helper_set_close_on_exec(pipe_descriptors[i]) ) {
            goto fail;
        }
    }
    descriptors[0] = pipe_descriptors[0];
    descriptors[1] = pipe_descriptors[1];
    return true;

fail:
    saved_errno = errno;
    if( pipe_descriptors[0] >= 0 ) {
        close(pipe_descriptors[0]);
    }
    if( pipe_descriptors[1] >= 0 ) {
        close(pipe_descriptors[1]);
    }
    errno = saved_errno;
    return false;
}

static void handlebarsc_helper_close(int * fd)
{
    if( *fd >= 0 ) {
        close(*fd);
        *fd = -1;
    }
}

static void handlebarsc_helper_terminate(pid_t pid)
{
    int status;

    if( pid <= 0 ) {
        return;
    }
    (void) kill(-pid, SIGKILL);
    (void) kill(pid, SIGKILL);
    while( waitpid(pid, &status, 0) < 0 && errno == EINTR ) {
        continue;
    }
}

static bool handlebarsc_helper_would_block(int error_number)
{
    return error_number == EAGAIN
#if EWOULDBLOCK != EAGAIN
        || error_number == EWOULDBLOCK
#endif
        ;
}

static bool handlebarsc_helper_try_reap(
    pid_t pid,
    bool * child_reaped,
    int * child_status,
    struct handlebarsc_helper_run_result * result
)
{
    pid_t waited;

    if( *child_reaped ) {
        return true;
    }
    do {
        waited = waitpid(pid, child_status, WNOHANG);
    } while( waited < 0 && errno == EINTR );
    if( waited == pid ) {
        *child_reaped = true;
        return true;
    }
    if( waited < 0 ) {
        result->error = handlebarsc_helper_run_system;
        result->error_number = errno;
        return false;
    }
    return true;
}

static bool handlebarsc_helper_pipe_has_hangup(
    int fd,
    bool * hung_up,
    struct handlebarsc_helper_run_result * result
)
{
    struct pollfd descriptor = {
        .fd = fd,
        .events = POLLIN | POLLHUP,
        .revents = 0
    };
    int call_error;

    do {
        call_error = poll(&descriptor, 1, 0);
    } while( call_error < 0 && errno == EINTR );
    if( call_error < 0 ) {
        result->error = handlebarsc_helper_run_system;
        result->error_number = errno;
        return false;
    }
    *hung_up = call_error > 0 && (descriptor.revents & POLLHUP) != 0;
    return true;
}

static unsigned long long handlebarsc_helper_elapsed_ms(
    const struct timespec * start,
    const struct timespec * now
)
{
    time_t seconds = now->tv_sec - start->tv_sec;
    long nanoseconds = now->tv_nsec - start->tv_nsec;

    if( nanoseconds < 0 ) {
        seconds--;
        nanoseconds += 1000000000L;
    }
    if( seconds < 0 ) {
        return 0;
    }
    if( (unsigned long long) seconds > ULLONG_MAX / 1000ULL ) {
        return ULLONG_MAX;
    }
    return (unsigned long long) seconds * 1000ULL
        + (unsigned long long) nanoseconds / 1000000ULL;
}

static bool handlebarsc_helper_deadline(
    const struct timespec * start,
    unsigned long timeout_ms,
    int * poll_timeout,
    struct handlebarsc_helper_run_result * result
)
{
    struct timespec now;
    unsigned long long elapsed;
    unsigned long long remaining;

    if( timeout_ms == 0 ) {
        if( poll_timeout != NULL ) {
            *poll_timeout = -1;
        }
        return true;
    }
    if( clock_gettime(CLOCK_MONOTONIC, &now) != 0 ) {
        result->error = handlebarsc_helper_run_system;
        result->error_number = errno;
        return false;
    }
    elapsed = handlebarsc_helper_elapsed_ms(start, &now);
    if( elapsed >= (unsigned long long) timeout_ms ) {
        result->error = handlebarsc_helper_run_timeout;
        return false;
    }
    if( poll_timeout != NULL ) {
        remaining = (unsigned long long) timeout_ms - elapsed;
        *poll_timeout = remaining > (unsigned long long) INT_MAX
            ? INT_MAX
            : (int) remaining;
    }
    return true;
}

static bool handlebarsc_helper_output_append(
    struct handlebarsc_helper_run_result * result,
    const char * input,
    size_t length,
    size_t output_limit,
    size_t * capacity
)
{
    size_t required;
    size_t new_capacity;
    char * output;

    if( output_limit != 0
            && (result->output_length > output_limit
                || length > output_limit - result->output_length) ) {
        result->error = handlebarsc_helper_run_output_limit;
        return false;
    }
    if( length > SIZE_MAX - result->output_length - 1 ) {
        result->error = handlebarsc_helper_run_system;
        result->error_number = EOVERFLOW;
        return false;
    }
    required = result->output_length + length + 1;
    if( required > *capacity ) {
        new_capacity = *capacity ? *capacity : 4096;
        while( new_capacity < required ) {
            if( new_capacity > SIZE_MAX / 2 ) {
                new_capacity = required;
                break;
            }
            new_capacity *= 2;
        }
        output = realloc(result->output, new_capacity);
        if( output == NULL ) {
            result->error = handlebarsc_helper_run_system;
            result->error_number = ENOMEM;
            return false;
        }
        result->output = output;
        *capacity = new_capacity;
    }
    if( length != 0 ) {
        memcpy(result->output + result->output_length, input, length);
        result->output_length += length;
    }
    result->output[result->output_length] = '\0';
    return true;
}

static bool handlebarsc_helper_run(
    const char * command,
    char * const argv[],
    const char * input,
    size_t input_length,
    unsigned long timeout_ms,
    size_t output_limit,
    struct handlebarsc_helper_run_result * result
)
{
    posix_spawn_file_actions_t actions;
    posix_spawnattr_t attributes;
    struct sigaction default_child = {0};
    struct sigaction ignore_pipe = {0};
    struct sigaction previous_child = {0};
    struct sigaction previous_pipe = {0};
    struct timespec start;
    pid_t pid = -1;
    int input_pipe[2] = { -1, -1 };
    int output_pipe[2] = { -1, -1 };
    size_t input_offset = 0;
    size_t output_capacity = 0;
    bool actions_open = false;
    bool attributes_open = false;
    bool child_reaped = false;
    bool output_eof = false;
    bool child_signal_changed = false;
    bool pipe_signal_changed = false;
    int child_status = 0;
    int call_error;
    short spawn_flags = POSIX_SPAWN_SETPGROUP;

#define HANDLEBARSC_SPAWN_ACTION(call) \
    do { \
        call_error = (call); \
        if( call_error != 0 ) { \
            result->error = handlebarsc_helper_run_system; \
            result->error_number = call_error; \
            goto fail; \
        } \
    } while(0)

    memset(result, 0, sizeof(*result));
    result->error = handlebarsc_helper_run_system;

    if( !handlebarsc_helper_pipe(output_pipe) ) {
        result->error_number = errno;
        goto fail;
    }
    if( input != NULL && !handlebarsc_helper_pipe(input_pipe) ) {
        result->error_number = errno;
        goto fail;
    }
    call_error = posix_spawn_file_actions_init(&actions);
    if( call_error != 0 ) {
        result->error_number = call_error;
        goto fail;
    }
    actions_open = true;
    call_error = posix_spawnattr_init(&attributes);
    if( call_error != 0 ) {
        result->error_number = call_error;
        goto fail;
    }
    attributes_open = true;
    HANDLEBARSC_SPAWN_ACTION(posix_spawnattr_setflags(&attributes, spawn_flags));
    HANDLEBARSC_SPAWN_ACTION(posix_spawnattr_setpgroup(&attributes, 0));

    if( sigaction(SIGCHLD, NULL, &previous_child) != 0 ) {
        result->error_number = errno;
        goto fail;
    }
    if( previous_child.sa_handler == SIG_IGN
#ifdef SA_NOCLDWAIT
            || (previous_child.sa_flags & SA_NOCLDWAIT) != 0
#endif
    ) {
        default_child.sa_handler = SIG_DFL;
        sigemptyset(&default_child.sa_mask);
        if( sigaction(SIGCHLD, &default_child, NULL) != 0 ) {
            result->error_number = errno;
            goto fail;
        }
        child_signal_changed = true;
    }

    if( input != NULL ) {
        HANDLEBARSC_SPAWN_ACTION(posix_spawn_file_actions_adddup2(
            &actions,
            input_pipe[0],
            STDIN_FILENO
        ));
        HANDLEBARSC_SPAWN_ACTION(posix_spawn_file_actions_addclose(&actions, input_pipe[0]));
        HANDLEBARSC_SPAWN_ACTION(posix_spawn_file_actions_addclose(&actions, input_pipe[1]));
    } else {
        HANDLEBARSC_SPAWN_ACTION(posix_spawn_file_actions_addopen(
            &actions,
            STDIN_FILENO,
            "/dev/null",
            O_RDONLY,
            0
        ));
    }
    HANDLEBARSC_SPAWN_ACTION(posix_spawn_file_actions_adddup2(
        &actions,
        output_pipe[1],
        STDOUT_FILENO
    ));
    HANDLEBARSC_SPAWN_ACTION(posix_spawn_file_actions_addclose(&actions, output_pipe[0]));
    HANDLEBARSC_SPAWN_ACTION(posix_spawn_file_actions_addclose(&actions, output_pipe[1]));

    if( strchr(command, '/') != NULL ) {
        call_error = posix_spawn(&pid, command, &actions, &attributes, argv, environ);
    } else {
        call_error = posix_spawnp(&pid, command, &actions, &attributes, argv, environ);
    }
    if( call_error != 0 ) {
        result->error = handlebarsc_helper_run_spawn;
        result->error_number = call_error;
        pid = -1;
        goto fail;
    }
    posix_spawn_file_actions_destroy(&actions);
    actions_open = false;
    posix_spawnattr_destroy(&attributes);
    attributes_open = false;

    handlebarsc_helper_close(&output_pipe[1]);
    handlebarsc_helper_close(&input_pipe[0]);
    if( !handlebarsc_helper_set_nonblocking(output_pipe[0])
            || (input_pipe[1] >= 0
                && !handlebarsc_helper_set_nonblocking(input_pipe[1])) ) {
        result->error_number = errno;
        goto fail;
    }
    if( clock_gettime(CLOCK_MONOTONIC, &start) != 0 ) {
        result->error_number = errno;
        goto fail;
    }

    if( input_pipe[1] >= 0 ) {
        ignore_pipe.sa_handler = SIG_IGN;
        sigemptyset(&ignore_pipe.sa_mask);
        if( sigaction(SIGPIPE, &ignore_pipe, &previous_pipe) != 0 ) {
            result->error_number = errno;
            goto fail;
        }
        pipe_signal_changed = true;
    }

    if( !handlebarsc_helper_output_append(result, NULL, 0, output_limit, &output_capacity) ) {
        goto fail;
    }

    while( !child_reaped || !output_eof ) {
        char buffer[8192];
        struct pollfd pollfds[2];
        nfds_t pollfd_count = 0;
        int poll_timeout = -1;

        while( !output_eof ) {
            ssize_t amount = read(output_pipe[0], buffer, sizeof(buffer));

            if( amount > 0 ) {
                if( !handlebarsc_helper_output_append(
                        result,
                        buffer,
                        (size_t) amount,
                        output_limit,
                        &output_capacity
                ) ) {
                    goto fail;
                }
                if( !handlebarsc_helper_try_reap(
                        pid,
                        &child_reaped,
                        &child_status,
                        result
                ) ) {
                    goto fail;
                }
                if( child_reaped ) {
                    bool output_hung_up;

                    if( !handlebarsc_helper_pipe_has_hangup(
                            output_pipe[0],
                            &output_hung_up,
                            result
                    ) ) {
                        goto fail;
                    }
                    if( output_hung_up ) {
                        continue;
                    }
                }
                if( !handlebarsc_helper_deadline(
                        &start,
                        timeout_ms,
                        NULL,
                        result
                ) ) {
                    goto fail;
                }
                continue;
            }
            if( amount == 0 ) {
                output_eof = true;
                handlebarsc_helper_close(&output_pipe[0]);
                break;
            }
            if( errno == EINTR ) {
                continue;
            }
            if( !handlebarsc_helper_would_block(errno) ) {
                result->error = handlebarsc_helper_run_system;
                result->error_number = errno;
                goto fail;
            }
            break;
        }

        while( input_pipe[1] >= 0 && input_offset < input_length ) {
            ssize_t amount = write(
                input_pipe[1],
                input + input_offset,
                input_length - input_offset
            );

            if( amount > 0 ) {
                input_offset += (size_t) amount;
                if( !handlebarsc_helper_try_reap(
                        pid,
                        &child_reaped,
                        &child_status,
                        result
                ) ) {
                    goto fail;
                }
                if( child_reaped ) {
                    handlebarsc_helper_close(&input_pipe[1]);
                    break;
                }
                if( !handlebarsc_helper_deadline(
                        &start,
                        timeout_ms,
                        NULL,
                        result
                ) ) {
                    goto fail;
                }
                continue;
            }
            if( amount < 0 && errno == EINTR ) {
                continue;
            }
            if( amount < 0 && handlebarsc_helper_would_block(errno) ) {
                break;
            }
            if( amount < 0 && errno == EPIPE ) {
                handlebarsc_helper_close(&input_pipe[1]);
                break;
            }
            result->error = handlebarsc_helper_run_system;
            result->error_number = amount < 0 ? errno : EIO;
            goto fail;
        }
        if( input_pipe[1] >= 0 && input_offset == input_length ) {
            handlebarsc_helper_close(&input_pipe[1]);
        }

        if( !handlebarsc_helper_try_reap(
                pid,
                &child_reaped,
                &child_status,
                result
        ) ) {
            goto fail;
        }
        if( child_reaped && output_eof ) {
            break;
        }

        if( !handlebarsc_helper_deadline(
                &start,
                timeout_ms,
                &poll_timeout,
                result
        ) ) {
            goto fail;
        }

        if( output_pipe[0] >= 0 ) {
            pollfds[pollfd_count].fd = output_pipe[0];
            pollfds[pollfd_count].events = POLLIN | POLLHUP;
            pollfds[pollfd_count].revents = 0;
            pollfd_count++;
        }
        if( input_pipe[1] >= 0 ) {
            pollfds[pollfd_count].fd = input_pipe[1];
            pollfds[pollfd_count].events = POLLOUT | POLLHUP;
            pollfds[pollfd_count].revents = 0;
            pollfd_count++;
        }
        if( pollfd_count == 0 && (poll_timeout < 0 || poll_timeout > 20) ) {
            poll_timeout = 20;
        }
        do {
            call_error = poll(pollfds, pollfd_count, poll_timeout);
        } while( call_error < 0 && errno == EINTR );
        if( call_error < 0 ) {
            result->error = handlebarsc_helper_run_system;
            result->error_number = errno;
            goto fail;
        }
    }

    if( pipe_signal_changed ) {
        (void) sigaction(SIGPIPE, &previous_pipe, NULL);
    }
    if( child_signal_changed ) {
        (void) sigaction(SIGCHLD, &previous_child, NULL);
    }
    (void) kill(-pid, SIGKILL);
    result->child_status = child_status;
    result->error = handlebarsc_helper_run_success;
    return true;

fail:
    if( pipe_signal_changed ) {
        (void) sigaction(SIGPIPE, &previous_pipe, NULL);
    }
    if( actions_open ) {
        posix_spawn_file_actions_destroy(&actions);
    }
    if( attributes_open ) {
        posix_spawnattr_destroy(&attributes);
    }
    handlebarsc_helper_close(&input_pipe[0]);
    handlebarsc_helper_close(&input_pipe[1]);
    handlebarsc_helper_close(&output_pipe[0]);
    handlebarsc_helper_close(&output_pipe[1]);
    if( pid > 0 && !child_reaped ) {
        handlebarsc_helper_terminate(pid);
    } else if( pid > 0 ) {
        (void) kill(-pid, SIGKILL);
    }
    if( child_signal_changed ) {
        (void) sigaction(SIGCHLD, &previous_child, NULL);
    }
    return false;

#undef HANDLEBARSC_SPAWN_ACTION
}
#else
static bool handlebarsc_helper_run(
    const char * command,
    char * const argv[],
    const char * input,
    size_t input_length,
    unsigned long timeout_ms,
    size_t output_limit,
    struct handlebarsc_helper_run_result * result
)
{
    (void) command;
    (void) argv;
    (void) input;
    (void) input_length;
    (void) timeout_ms;
    (void) output_limit;
    memset(result, 0, sizeof(*result));
    result->error = handlebarsc_helper_run_unsupported;
    return false;
}
#endif

#ifdef HANDLEBARS_HAVE_JSON
static int handlebarsc_helper_json_hex_value(unsigned char character)
{
    if( character >= '0' && character <= '9' ) {
        return character - '0';
    }
    if( character >= 'a' && character <= 'f' ) {
        return character - 'a' + 10;
    }
    if( character >= 'A' && character <= 'F' ) {
        return character - 'A' + 10;
    }
    return -1;
}

static bool handlebarsc_helper_json_code_unit(
    const unsigned char * input,
    size_t length,
    size_t offset,
    unsigned int * code_unit
)
{
    unsigned int value = 0;

    if( offset > length || length - offset < 4 ) {
        return false;
    }
    for( size_t i = 0; i < 4; i++ ) {
        int digit = handlebarsc_helper_json_hex_value(input[offset + i]);

        if( digit < 0 ) {
            return false;
        }
        value = value * 16U + (unsigned int) digit;
    }
    *code_unit = value;
    return true;
}

static bool handlebarsc_helper_json_valid_utf8(
    const unsigned char * input,
    size_t length
)
{
    size_t i = 0;

    while( i < length ) {
        unsigned char first = input[i++];

        if( first <= 0x7f ) {
            continue;
        }
        if( first >= 0xc2 && first <= 0xdf ) {
            if( i >= length || input[i] < 0x80 || input[i] > 0xbf ) {
                return false;
            }
            i++;
            continue;
        }
        if( first >= 0xe0 && first <= 0xef ) {
            unsigned char second;

            if( length - i < 2 ) {
                return false;
            }
            second = input[i];
            if( (first == 0xe0 && (second < 0xa0 || second > 0xbf))
                    || (first == 0xed && (second < 0x80 || second > 0x9f))
                    || (first != 0xe0 && first != 0xed
                        && (second < 0x80 || second > 0xbf))
                    || input[i + 1] < 0x80
                    || input[i + 1] > 0xbf ) {
                return false;
            }
            i += 2;
            continue;
        }
        if( first >= 0xf0 && first <= 0xf4 ) {
            unsigned char second;

            if( length - i < 3 ) {
                return false;
            }
            second = input[i];
            if( (first == 0xf0 && (second < 0x90 || second > 0xbf))
                    || (first == 0xf4 && (second < 0x80 || second > 0x8f))
                    || (first != 0xf0 && first != 0xf4
                        && (second < 0x80 || second > 0xbf))
                    || input[i + 1] < 0x80
                    || input[i + 1] > 0xbf
                    || input[i + 2] < 0x80
                    || input[i + 2] > 0xbf ) {
                return false;
            }
            i += 3;
            continue;
        }
        return false;
    }
    return true;
}

static bool handlebarsc_helper_json_valid_surrogates(
    const unsigned char * input,
    size_t length
)
{
    bool in_string = false;

    for( size_t i = 0; i < length; i++ ) {
        unsigned int code_unit;

        if( !in_string ) {
            if( input[i] == '"' ) {
                in_string = true;
            }
            continue;
        }
        if( input[i] == '"' ) {
            in_string = false;
            continue;
        }
        if( input[i] != '\\' ) {
            continue;
        }
        if( ++i >= length ) {
            return false;
        }
        if( input[i] != 'u' ) {
            continue;
        }
        if( !handlebarsc_helper_json_code_unit(
                input,
                length,
                i + 1,
                &code_unit
        ) ) {
            return false;
        }
        i += 4;
        if( code_unit >= 0xdc00 && code_unit <= 0xdfff ) {
            return false;
        }
        if( code_unit >= 0xd800 && code_unit <= 0xdbff ) {
            unsigned int low_surrogate;

            if( i > length || length - i < 7
                    || input[i + 1] != '\\'
                    || input[i + 2] != 'u'
                    || !handlebarsc_helper_json_code_unit(
                        input,
                        length,
                        i + 3,
                        &low_surrogate
                    )
                    || low_surrogate < 0xdc00
                    || low_surrogate > 0xdfff ) {
                return false;
            }
            i += 6;
        }
    }
    return true;
}

static bool handlebarsc_helper_json_match_literal(
    const unsigned char * input,
    size_t length,
    size_t offset,
    const char * literal,
    size_t literal_length
)
{
    return offset <= length
        && literal_length <= length - offset
        && memcmp(input + offset, literal, literal_length) == 0;
}

static bool handlebarsc_helper_json_is_whitespace(unsigned char character)
{
    return character == ' '
        || character == '\t'
        || character == '\r'
        || character == '\n';
}

static bool handlebarsc_helper_json_valid_literals(
    const unsigned char * input,
    size_t length
)
{
    bool in_string = false;

    for( size_t i = 0; i < length; i++ ) {
        if( in_string ) {
            if( input[i] == '\\' ) {
                if( ++i >= length ) {
                    return false;
                }
            } else if( input[i] == '"' ) {
                in_string = false;
            }
            continue;
        }
        if( input[i] == '"' ) {
            in_string = true;
            continue;
        }
        if( input[i] <= 0x1f
                && !handlebarsc_helper_json_is_whitespace(input[i]) ) {
            return false;
        }
        if( input[i] >= 0x80 ) {
            return false;
        }
        if( input[i] == '/' || input[i] == '\'' ) {
            return false;
        }
        if( input[i] == 't'
                && handlebarsc_helper_json_match_literal(
                    input,
                    length,
                    i,
                    "true",
                    4
                ) ) {
            i += 3;
            continue;
        }
        if( input[i] == 'f'
                && handlebarsc_helper_json_match_literal(
                    input,
                    length,
                    i,
                    "false",
                    5
                ) ) {
            i += 4;
            continue;
        }
        if( input[i] == 'n'
                && handlebarsc_helper_json_match_literal(
                    input,
                    length,
                    i,
                    "null",
                    4
                ) ) {
            i += 3;
            continue;
        }
        if( (input[i] >= 'A' && input[i] <= 'Z')
                || (input[i] >= 'a' && input[i] <= 'z') ) {
            if( input[i] == 'e' || input[i] == 'E' ) {
                continue;
            }
            return false;
        }
    }
    return true;
}

static bool handlebarsc_helper_json_valid_unicode(
    const char * input,
    size_t length
)
{
    const unsigned char * bytes = (const unsigned char *) input;

    return handlebarsc_helper_json_valid_utf8(bytes, length)
        && handlebarsc_helper_json_valid_surrogates(bytes, length);
}

static struct json_object * handlebarsc_helper_json_parse_response(
    struct handlebarsc_helper_dispatch_state * state,
    const char * name,
    const char * input,
    size_t length,
    int flags
)
{
    struct json_tokener * tokener = json_tokener_new();
    struct json_object * response;
    size_t parse_end;

    if( tokener == NULL ) {
        handlebars_throw(
            HBSCTX(state->vm),
            HANDLEBARS_NOMEM,
            "Failed to initialize external helper JSON parser"
        );
    }
    json_tokener_set_flags(tokener, flags);
    response = json_tokener_parse_ex(tokener, input, (int) length + 1);
    parse_end = (size_t) tokener->char_offset;
    while( parse_end < length
            && handlebarsc_helper_json_is_whitespace(
                (unsigned char) input[parse_end]
            ) ) {
        parse_end++;
    }
    if( json_tokener_get_error(tokener) != json_tokener_success
            || response == NULL
            || parse_end != length ) {
        enum json_tokener_error parse_error = json_tokener_get_error(tokener);
        enum handlebars_error_type error_type = HANDLEBARS_ERROR;

#if defined(JSON_C_VERSION_NUM) && JSON_C_VERSION_NUM >= 0x001100
        if( parse_error == json_tokener_error_memory ) {
            error_type = HANDLEBARS_NOMEM;
        }
#endif
        if( response != NULL ) {
            json_object_put(response);
        }
        json_tokener_free(tokener);
        handlebars_throw(
            HBSCTX(state->vm),
            error_type,
            "External helper %s returned an invalid JSON response: %s",
            name,
            json_tokener_error_desc(parse_error)
        );
    }
    json_tokener_free(tokener);
    return response;
}
#endif

static HBS_ATTR_NORETURN void handlebarsc_helper_throw_run_error(
    struct handlebarsc_helper_dispatch_state * state,
    const char * name
)
{
    switch( state->run.error ) {
        case handlebarsc_helper_run_unsupported:
            handlebars_throw(
                HBSCTX(state->vm),
                HANDLEBARS_ERROR,
                "Executable-backed helpers are not supported on this platform"
            );
            break;
        case handlebarsc_helper_run_spawn:
            handlebars_throw(
                HBSCTX(state->vm),
                HANDELBARS_EXTERNAL_ERROR,
                "Failed to execute external helper %s: %s",
                name,
                strerror(state->run.error_number)
            );
            break;
        case handlebarsc_helper_run_timeout:
            handlebars_throw(
                HBSCTX(state->vm),
                HANDELBARS_EXTERNAL_ERROR,
                "External helper %s timed out",
                name
            );
            break;
        case handlebarsc_helper_run_output_limit:
            handlebars_throw(
                HBSCTX(state->vm),
                HANDELBARS_EXTERNAL_ERROR,
                "External helper %s exceeded its output limit",
                name
            );
            break;
        case handlebarsc_helper_run_system:
            handlebars_throw(
                HBSCTX(state->vm),
                state->run.error_number == ENOMEM ? HANDLEBARS_NOMEM : HANDELBARS_EXTERNAL_ERROR,
                "External helper %s failed: %s",
                name,
                strerror(state->run.error_number)
            );
            break;
        case handlebarsc_helper_run_success:
        default:
            handlebars_throw(
                HBSCTX(state->vm),
                HANDELBARS_EXTERNAL_ERROR,
                "External helper %s failed",
                name
            );
            break;
    }
}

static void handlebarsc_helper_check_status(
    struct handlebarsc_helper_dispatch_state * state,
    const char * name
)
{
#ifdef HANDLEBARSC_HAVE_EXEC_HELPERS
    if( !WIFEXITED(state->run.child_status) ) {
        if( WIFSIGNALED(state->run.child_status) ) {
            handlebars_throw(
                HBSCTX(state->vm),
                HANDELBARS_EXTERNAL_ERROR,
                "External helper %s terminated by signal %d",
                name,
                WTERMSIG(state->run.child_status)
            );
        }
        handlebars_throw(
            HBSCTX(state->vm),
            HANDELBARS_EXTERNAL_ERROR,
            "External helper %s terminated abnormally",
            name
        );
    }
    if( WEXITSTATUS(state->run.child_status) != 0 ) {
        handlebars_throw(
            HBSCTX(state->vm),
            HANDELBARS_EXTERNAL_ERROR,
            "External helper %s exited with status %d",
            name,
            WEXITSTATUS(state->run.child_status)
        );
    }
#else
    (void) state;
    (void) name;
#endif
}

static bool handlebarsc_helper_plain_type_supported(enum handlebars_value_type type)
{
    switch( type ) {
        case HANDLEBARS_VALUE_TYPE_NULL:
        case HANDLEBARS_VALUE_TYPE_TRUE:
        case HANDLEBARS_VALUE_TYPE_FALSE:
        case HANDLEBARS_VALUE_TYPE_INTEGER:
        case HANDLEBARS_VALUE_TYPE_FLOAT:
        case HANDLEBARS_VALUE_TYPE_STRING:
            return true;
        case HANDLEBARS_VALUE_TYPE_ARRAY:
        case HANDLEBARS_VALUE_TYPE_MAP:
        case HANDLEBARS_VALUE_TYPE_USER:
        case HANDLEBARS_VALUE_TYPE_PTR:
        case HANDLEBARS_VALUE_TYPE_HELPER:
        case HANDLEBARS_VALUE_TYPE_CLOSURE:
        default:
            return false;
    }
}

static void handlebarsc_helper_dispatch_plain(
    struct handlebarsc_helper_dispatch_state * state,
    const char * name,
    const char * command,
    int argc,
    struct handlebars_value * argv,
    unsigned long timeout_ms,
    size_t output_limit
)
{
    char ** child_argv;

    state->owner = handlebars_talloc_zero_size(HBSCTX(state->vm), 1);
    HANDLEBARS_MEMCHECK(state->owner, HBSCTX(state->vm));
    child_argv = talloc_zero_array(state->owner, char *, (size_t) argc + 2);
    HANDLEBARS_MEMCHECK(child_argv, HBSCTX(state->vm));
    child_argv[0] = (char *) command;
    for( int i = 0; i < argc; i++ ) {
        struct handlebars_string * string;
        enum handlebars_value_type type = handlebars_value_get_type(&argv[i]);

        if( !handlebarsc_helper_plain_type_supported(type) ) {
            handlebars_throw(
                HBSCTX(state->vm),
                HANDLEBARS_ERROR,
                "External helper %s received unsupported argument type %s",
                name,
                handlebars_value_type_readable(type)
            );
        }
        string = handlebars_value_to_string(&argv[i], HBSCTX(state->vm));
        if( strlen(hbs_str_val(string)) != hbs_str_len(string) ) {
            handlebars_string_delref(string);
            handlebars_throw(
                HBSCTX(state->vm),
                HANDLEBARS_ERROR,
                "External helper %s arguments cannot contain NUL bytes",
                name
            );
        }
        child_argv[i + 1] = handlebars_talloc_strndup(
            state->owner,
            hbs_str_val(string),
            hbs_str_len(string)
        );
        handlebars_string_delref(string);
        HANDLEBARS_MEMCHECK(child_argv[i + 1], HBSCTX(state->vm));
    }

    if( !handlebarsc_helper_run(
            command,
            child_argv,
            NULL,
            0,
            timeout_ms,
            output_limit,
            &state->run
    ) ) {
        handlebarsc_helper_throw_run_error(state, name);
    }
    handlebarsc_helper_check_status(state, name);
    handlebars_value_str(
        &state->result,
        handlebars_string_ctor(
            HBSCTX(state->vm),
            state->run.output,
            state->run.output_length
        )
    );
}

#ifdef HANDLEBARS_HAVE_JSON
static struct handlebarsc_tracked_json * handlebarsc_helper_json_track(
    struct handlebarsc_helper_dispatch_state * state,
    struct json_object * object
)
{
    struct handlebarsc_tracked_json * tracked = handlebars_talloc_zero(
        state->owner,
        struct handlebarsc_tracked_json
    );

    if( tracked == NULL ) {
        if( object != NULL ) {
            json_object_put(object);
        }
        handlebars_throw(
            HBSCTX(state->vm),
            HANDLEBARS_NOMEM,
            "Failed to track external helper JSON"
        );
    }
    tracked->object = object;
    tracked->owned = true;
    tracked->next = state->json;
    state->json = tracked;
    return tracked;
}

static struct json_object * handlebarsc_helper_json_new(
    struct handlebarsc_helper_dispatch_state * state,
    struct json_object * object,
    struct handlebarsc_tracked_json ** tracked
)
{
    if( object == NULL ) {
        handlebars_throw(
            HBSCTX(state->vm),
            HANDLEBARS_NOMEM,
            "Failed to allocate external helper JSON"
        );
    }
    *tracked = handlebarsc_helper_json_track(state, object);
    return object;
}

static void handlebarsc_helper_json_attach_array(
    struct handlebarsc_helper_dispatch_state * state,
    struct json_object * parent,
    struct json_object * child,
    struct handlebarsc_tracked_json * tracked
)
{
    if( json_object_array_add(parent, child) != 0 ) {
        handlebars_throw(
            HBSCTX(state->vm),
            HANDLEBARS_NOMEM,
            "Failed to append external helper JSON"
        );
    }
    if( tracked != NULL ) {
        tracked->owned = false;
    }
}

static void handlebarsc_helper_json_attach_object(
    struct handlebarsc_helper_dispatch_state * state,
    struct json_object * parent,
    const char * key,
    struct json_object * child,
    struct handlebarsc_tracked_json * tracked
)
{
    if( json_object_object_add(parent, key, child) != 0 ) {
        handlebars_throw(
            HBSCTX(state->vm),
            HANDLEBARS_NOMEM,
            "Failed to append external helper JSON"
        );
    }
    if( tracked != NULL ) {
        tracked->owned = false;
    }
}

static const void * handlebarsc_helper_json_identity(
    struct handlebars_value * value
)
{
    switch( handlebars_value_get_real_type(value) ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            return value->v.stack;
        case HANDLEBARS_VALUE_TYPE_MAP:
            return value->v.map;
        case HANDLEBARS_VALUE_TYPE_USER:
            return value->v.user;
        default:
            return NULL;
    }
}

static void handlebarsc_helper_json_enter(
    struct handlebarsc_helper_dispatch_state * state,
    struct handlebars_value * value
)
{
    const void * identity = handlebarsc_helper_json_identity(value);

    if( state->active_count >= HANDLEBARS_VALUE_MAX_DEPTH ) {
        handlebars_throw(
            HBSCTX(state->vm),
            HANDLEBARS_ERROR,
            "External helper JSON exceeds the maximum depth of %d",
            HANDLEBARS_VALUE_MAX_DEPTH
        );
    }
    for( size_t i = 0; i < state->active_count; i++ ) {
        if( state->active[i] == identity ) {
            handlebars_throw(
                HBSCTX(state->vm),
                HANDLEBARS_ERROR,
                "External helper JSON cannot contain cycles"
            );
        }
    }
    state->active[state->active_count++] = identity;
}

static struct json_object * handlebarsc_helper_json_from_value(
    struct handlebarsc_helper_dispatch_state * state,
    struct handlebars_value * value,
    struct handlebarsc_tracked_json ** tracked
)
{
    enum handlebars_value_type type = handlebars_value_get_type(value);
    struct json_object * object;

    *tracked = NULL;
    switch( type ) {
        case HANDLEBARS_VALUE_TYPE_NULL:
            return NULL;
        case HANDLEBARS_VALUE_TYPE_TRUE:
        case HANDLEBARS_VALUE_TYPE_FALSE:
            return handlebarsc_helper_json_new(
                state,
                json_object_new_boolean(type == HANDLEBARS_VALUE_TYPE_TRUE),
                tracked
            );
        case HANDLEBARS_VALUE_TYPE_INTEGER:
            return handlebarsc_helper_json_new(
                state,
                json_object_new_int64((int64_t) handlebars_value_get_intval(value)),
                tracked
            );
        case HANDLEBARS_VALUE_TYPE_FLOAT:
            if( !isfinite(handlebars_value_get_floatval(value)) ) {
                handlebars_throw(
                    HBSCTX(state->vm),
                    HANDLEBARS_ERROR,
                    "External helper JSON cannot represent a non-finite number"
                );
            }
            return handlebarsc_helper_json_new(
                state,
                json_object_new_double(handlebars_value_get_floatval(value)),
                tracked
            );
        case HANDLEBARS_VALUE_TYPE_STRING: {
            const char * string = handlebars_value_get_strval(value);
            size_t length = handlebars_value_get_strlen(value);

            if( length > INT_MAX ) {
                handlebars_throw(
                    HBSCTX(state->vm),
                    HANDLEBARS_ERROR,
                    "External helper JSON string is too large"
                );
            }
            if( !handlebarsc_helper_json_valid_utf8(
                    (const unsigned char *) string,
                    length
            ) ) {
                handlebars_throw(
                    HBSCTX(state->vm),
                    HANDLEBARS_ERROR,
                    "External helper JSON string contains malformed UTF-8"
                );
            }
            return handlebarsc_helper_json_new(
                state,
                json_object_new_string_len(string, (int) length),
                tracked
            );
        }
        case HANDLEBARS_VALUE_TYPE_ARRAY: {
            HANDLEBARS_VALUE_ITERATOR_DECL(iterator);

            object = handlebarsc_helper_json_new(
                state,
                json_object_new_array(),
                tracked
            );
            handlebarsc_helper_json_enter(state, value);
            if( HANDLEBARS_VALUE_ITERATOR_INIT(iterator, value) ) {
                do {
                    struct handlebarsc_tracked_json * child_tracked;
                    struct json_object * child = handlebarsc_helper_json_from_value(
                        state,
                        iterator->cur,
                        &child_tracked
                    );
                    handlebarsc_helper_json_attach_array(
                        state,
                        object,
                        child,
                        child_tracked
                    );
                } while( handlebars_value_iterator_next(iterator) );
            }
            handlebars_value_iterator_close(iterator);
            state->active_count--;
            return object;
        }
        case HANDLEBARS_VALUE_TYPE_MAP: {
            HANDLEBARS_VALUE_ITERATOR_DECL(iterator);

            object = handlebarsc_helper_json_new(
                state,
                json_object_new_object(),
                tracked
            );
            handlebarsc_helper_json_enter(state, value);
            if( HANDLEBARS_VALUE_ITERATOR_INIT(iterator, value) ) {
                do {
                    struct handlebarsc_tracked_json * child_tracked;
                    struct json_object * child;
                    const char * key = hbs_str_val(iterator->key);
                    size_t key_length = hbs_str_len(iterator->key);

                    if( strlen(key) != key_length ) {
                        handlebars_throw(
                            HBSCTX(state->vm),
                            HANDLEBARS_ERROR,
                            "External helper JSON object keys cannot contain NUL bytes"
                        );
                    }
                    if( !handlebarsc_helper_json_valid_utf8(
                            (const unsigned char *) key,
                            key_length
                    ) ) {
                        handlebars_throw(
                            HBSCTX(state->vm),
                            HANDLEBARS_ERROR,
                            "External helper JSON object key contains malformed UTF-8"
                        );
                    }
                    child = handlebarsc_helper_json_from_value(
                        state,
                        iterator->cur,
                        &child_tracked
                    );
                    handlebarsc_helper_json_attach_object(
                        state,
                        object,
                        key,
                        child,
                        child_tracked
                    );
                } while( handlebars_value_iterator_next(iterator) );
            }
            handlebars_value_iterator_close(iterator);
            state->active_count--;
            return object;
        }
        case HANDLEBARS_VALUE_TYPE_USER:
        case HANDLEBARS_VALUE_TYPE_PTR:
        case HANDLEBARS_VALUE_TYPE_HELPER:
        case HANDLEBARS_VALUE_TYPE_CLOSURE:
        default:
            handlebars_throw(
                HBSCTX(state->vm),
                HANDLEBARS_ERROR,
                "External helper JSON received unsupported value type %s",
                handlebars_value_type_readable(type)
            );
            return NULL;
    }
}

static void handlebarsc_helper_json_add_value(
    struct handlebarsc_helper_dispatch_state * state,
    struct json_object * parent,
    const char * key,
    struct handlebars_value * value
)
{
    struct handlebarsc_tracked_json * tracked;
    struct json_object * object = value == NULL
        ? NULL
        : handlebarsc_helper_json_from_value(state, value, &tracked);

    if( value == NULL ) {
        tracked = NULL;
    }
    handlebarsc_helper_json_attach_object(state, parent, key, object, tracked);
}

static void handlebarsc_helper_dispatch_json(
    struct handlebarsc_helper_dispatch_state * state,
    const char * name,
    const char * command,
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    unsigned long timeout_ms,
    size_t output_limit
)
{
    struct handlebarsc_tracked_json * request_tracked;
    struct handlebarsc_tracked_json * field_tracked;
    struct json_object * request;
    struct json_object * arguments;
    struct json_object * response;
    struct json_object * protocol;
    struct json_object * ok;
    struct json_object * value;
    struct json_object * safe = NULL;
    struct json_object * validation;
    const char * request_string;
    char * request_input;
    char * strict_input;
    size_t request_length;
    char * child_argv[2] = { (char *) command, NULL };

    if( !handlebarsc_helper_json_valid_utf8(
            (const unsigned char *) name,
            strlen(name)
    ) ) {
        handlebars_throw(
            HBSCTX(state->vm),
            HANDLEBARS_ERROR,
            "External helper JSON name contains malformed UTF-8"
        );
    }
    state->owner = handlebars_talloc_zero_size(HBSCTX(state->vm), 1);
    HANDLEBARS_MEMCHECK(state->owner, HBSCTX(state->vm));
    request = handlebarsc_helper_json_new(
        state,
        json_object_new_object(),
        &request_tracked
    );
    protocol = handlebarsc_helper_json_new(
        state,
        json_object_new_int(1),
        &field_tracked
    );
    handlebarsc_helper_json_attach_object(
        state,
        request,
        "protocol",
        protocol,
        field_tracked
    );
    protocol = handlebarsc_helper_json_new(
        state,
        json_object_new_string_len(name, (int) strlen(name)),
        &field_tracked
    );
    handlebarsc_helper_json_attach_object(
        state,
        request,
        "helper",
        protocol,
        field_tracked
    );
    arguments = handlebarsc_helper_json_new(
        state,
        json_object_new_array(),
        &field_tracked
    );
    handlebarsc_helper_json_attach_object(
        state,
        request,
        "args",
        arguments,
        field_tracked
    );
    for( int i = 0; i < argc; i++ ) {
        struct handlebarsc_tracked_json * argument_tracked;
        struct json_object * argument = handlebarsc_helper_json_from_value(
            state,
            &argv[i],
            &argument_tracked
        );
        handlebarsc_helper_json_attach_array(
            state,
            arguments,
            argument,
            argument_tracked
        );
    }
    handlebarsc_helper_json_add_value(state, request, "hash", options->hash);
    handlebarsc_helper_json_add_value(state, request, "scope", options->scope);
    handlebarsc_helper_json_add_value(state, request, "data", options->data);

    request_string = json_object_to_json_string_ext(request, JSON_C_TO_STRING_PLAIN);
    if( request_string == NULL ) {
        handlebars_throw(
            HBSCTX(state->vm),
            HANDLEBARS_NOMEM,
            "Failed to serialize external helper JSON request"
        );
    }
    request_length = strlen(request_string);
    if( request_length == SIZE_MAX ) {
        handlebars_throw(
            HBSCTX(state->vm),
            HANDLEBARS_ERROR,
            "External helper JSON request is too large"
        );
    }
    request_input = handlebars_talloc_array(state->owner, char, request_length + 2);
    HANDLEBARS_MEMCHECK(request_input, HBSCTX(state->vm));
    memcpy(request_input, request_string, request_length);
    request_input[request_length++] = '\n';
    request_input[request_length] = '\0';

    if( !handlebarsc_helper_run(
            command,
            child_argv,
            request_input,
            request_length,
            timeout_ms,
            output_limit,
            &state->run
    ) ) {
        handlebarsc_helper_throw_run_error(state, name);
    }
    handlebarsc_helper_check_status(state, name);

    if( state->run.output_length > INT_MAX - 1 ) {
        handlebars_throw(
            HBSCTX(state->vm),
            HANDLEBARS_ERROR,
            "External helper %s returned an invalid JSON response: response is too large",
            name
        );
    }
    if( !handlebarsc_helper_json_valid_unicode(
            state->run.output,
            state->run.output_length
    ) ) {
        handlebars_throw(
            HBSCTX(state->vm),
            HANDLEBARS_ERROR,
            "External helper %s returned an invalid JSON response: malformed Unicode",
            name
        );
    }
    if( !handlebarsc_helper_json_valid_literals(
            (const unsigned char *) state->run.output,
            state->run.output_length
    ) ) {
        handlebars_throw(
            HBSCTX(state->vm),
            HANDLEBARS_ERROR,
            "External helper %s returned an invalid JSON response: non-standard literal",
            name
        );
    }
    strict_input = handlebars_talloc_array(
        state->owner,
        char,
        state->run.output_length + 1
    );
    HANDLEBARS_MEMCHECK(strict_input, HBSCTX(state->vm));
    for( size_t i = 0; i < state->run.output_length; i++ ) {
        unsigned char byte = (unsigned char) state->run.output[i];

        strict_input[i] = byte >= 0x80 ? 'x' : (char) byte;
    }
    strict_input[state->run.output_length] = '\0';
    validation = handlebarsc_helper_json_parse_response(
        state,
        name,
        strict_input,
        state->run.output_length,
        JSON_TOKENER_STRICT
    );
    json_object_put(validation);
    response = handlebarsc_helper_json_parse_response(
        state,
        name,
        state->run.output,
        state->run.output_length,
        0
    );
    (void) handlebarsc_helper_json_track(state, response);

    if( json_object_get_type(response) != json_type_object
            || !json_object_object_get_ex(response, "protocol", &protocol)
            || json_object_get_type(protocol) != json_type_int
            || json_object_get_int64(protocol) != 1
            || !json_object_object_get_ex(response, "ok", &ok)
            || json_object_get_type(ok) != json_type_boolean ) {
        handlebars_throw(
            HBSCTX(state->vm),
            HANDLEBARS_ERROR,
            "External helper %s returned an invalid JSON response envelope",
            name
        );
    }
    if( !json_object_get_boolean(ok) ) {
        struct json_object * error;

        if( !json_object_object_get_ex(response, "error", &error)
                || json_object_get_type(error) != json_type_string ) {
            handlebars_throw(
                HBSCTX(state->vm),
                HANDLEBARS_ERROR,
                "External helper %s returned an invalid JSON error response",
                name
            );
        }
        handlebars_throw(
            HBSCTX(state->vm),
            HANDELBARS_EXTERNAL_ERROR,
            "External helper %s failed: %.*s",
            name,
            json_object_get_string_len(error),
            json_object_get_string(error)
        );
    }
    if( !json_object_object_get_ex(response, "value", &value) ) {
        handlebars_throw(
            HBSCTX(state->vm),
            HANDLEBARS_ERROR,
            "External helper %s returned a JSON success response without value",
            name
        );
    }
    if( json_object_object_get_ex(response, "safe", &safe)
            && json_object_get_type(safe) != json_type_boolean ) {
        handlebars_throw(
            HBSCTX(state->vm),
            HANDLEBARS_ERROR,
            "External helper %s returned a non-boolean safe field",
            name
        );
    }
    if( value == NULL ) {
        handlebars_value_null(&state->result);
    } else {
        handlebars_value_init_json_object(HBSCTX(state->vm), &state->result, value);
    }
    if( safe != NULL && json_object_get_boolean(safe) ) {
        if( handlebars_value_get_type(&state->result) != HANDLEBARS_VALUE_TYPE_STRING ) {
            handlebars_throw(
                HBSCTX(state->vm),
                HANDLEBARS_ERROR,
                "External helper %s marked a non-string value as safe",
                name
            );
        }
        handlebars_value_set_flag(&state->result, HANDLEBARS_VALUE_FLAG_SAFE_STRING);
    }
    (void) request_tracked;
}
#endif

static void handlebarsc_helper_dispatch_worker(
    struct handlebarsc_helper_dispatch_state * state,
    int localc,
    struct handlebars_value * localv,
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options
)
{
    const char * name;
    const char * command;
    long timeout;
    long output_limit;
    long mode;

    if( localc != handlebarsc_helper_local_count ) {
        handlebars_throw(
            HBSCTX(state->vm),
            HANDLEBARS_ERROR,
            "Invalid external helper closure"
        );
    }
    name = handlebars_value_get_strval(&localv[handlebarsc_helper_local_name]);
    command = handlebars_value_get_strval(&localv[handlebarsc_helper_local_command]);
    mode = handlebars_value_get_intval(&localv[handlebarsc_helper_local_mode]);
    timeout = handlebars_value_get_intval(&localv[handlebarsc_helper_local_timeout]);
    output_limit = handlebars_value_get_intval(
        &localv[handlebarsc_helper_local_output_limit]
    );

    if( options->program >= 0 || options->inverse >= 0 ) {
        handlebars_throw(
            HBSCTX(state->vm),
            HANDLEBARS_ERROR,
            "External helper %s does not support block invocation",
            name
        );
    }
    if( mode == handlebarsc_helper_mode_exec ) {
        if( handlebars_value_count(options->hash) != 0 ) {
            handlebars_throw(
                HBSCTX(state->vm),
                HANDLEBARS_ERROR,
                "External helper %s does not support hash arguments",
                name
            );
        }
        handlebarsc_helper_dispatch_plain(
            state,
            name,
            command,
            argc,
            argv,
            (unsigned long) timeout,
            (size_t) output_limit
        );
    }
#ifdef HANDLEBARS_HAVE_JSON
    else if( mode == handlebarsc_helper_mode_json ) {
        handlebarsc_helper_dispatch_json(
            state,
            name,
            command,
            argc,
            argv,
            options,
            (unsigned long) timeout,
            (size_t) output_limit
        );
    }
#endif
    else {
        handlebars_throw(
            HBSCTX(state->vm),
            HANDLEBARS_ERROR,
            "Unsupported external helper mode"
        );
    }
    handlebars_value_value(state->rv, &state->result);
}

HBS_ATTR_NOINLINE
static void handlebarsc_helper_dispatch_guarded(
    struct handlebarsc_helper_dispatch_state * state,
    int localc,
    struct handlebars_value * localv,
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options
)
{
    struct handlebars_context * context = HBSCTX(state->vm);
    struct handlebars_error * error = context->e;
    jmp_buf * volatile previous = error->jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    jmp_buf boundary;

    if( handlebars_setjmp_ex(context, &boundary) ) {
        caught = error->num;
    } else {
        handlebarsc_helper_dispatch_worker(
            state,
            localc,
            localv,
            argc,
            argv,
            options
        );
    }
    error->jmp = previous;
    handlebarsc_helper_dispatch_cleanup(state);
    if( caught != HANDLEBARS_SUCCESS ) {
        if( previous != NULL ) {
            handlebarsc_helper_rethrow(context, previous, caught);
        }
        abort();
    }
}

static struct handlebars_value * handlebarsc_helper_dispatch(
    int localc,
    struct handlebars_value * localv,
    HANDLEBARS_HELPER_ARGS
)
{
    struct handlebarsc_helper_dispatch_state state = {0};

    state.vm = vm;
    state.rv = rv;
    handlebars_value_init(&state.result);
    state.result_initialized = true;
    handlebarsc_helper_dispatch_guarded(
        &state,
        localc,
        localv,
        argc,
        argv,
        options
    );
    return rv;
}

void handlebarsc_helper_registry_init(
    struct handlebarsc_helper_registry * registry,
    void * ctx
)
{
    memset(registry, 0, sizeof(*registry));
    registry->ctx = ctx;
    registry->timeout_ms = 5000;
    registry->output_limit = 1024 * 1024;
}

bool handlebarsc_helper_registry_add(
    struct handlebarsc_helper_registry * registry,
    enum handlebarsc_helper_mode mode,
    const char * specification,
    const char ** error
)
{
    const char * separator = strchr(specification, '=');
    struct handlebarsc_helper_registration * registration;
    char * name;
    char * command;

    registry->options_used = true;
    if( separator == NULL || separator == specification || separator[1] == '\0' ) {
        *error = "External helper must use NAME=COMMAND";
        return false;
    }
    if( registry->count == registry->capacity ) {
        size_t capacity = registry->capacity ? registry->capacity * 2 : 4;
        struct handlebarsc_helper_registration * registrations;

        if( capacity < registry->capacity ) {
            *error = "Too many external helper registrations";
            return false;
        }
        registrations = talloc_realloc(
            registry->ctx,
            registry->registrations,
            struct handlebarsc_helper_registration,
            capacity
        );
        if( registrations == NULL ) {
            *error = "Failed to allocate external helper registration";
            return false;
        }
        registry->registrations = registrations;
        registry->capacity = capacity;
    }
    name = talloc_strndup(
        registry->registrations,
        specification,
        (size_t) (separator - specification)
    );
    command = talloc_strdup(registry->registrations, separator + 1);
    if( name == NULL || command == NULL ) {
        talloc_free(name);
        talloc_free(command);
        *error = "Failed to allocate external helper registration";
        return false;
    }
    registration = &registry->registrations[registry->count++];
    registration->name = name;
    registration->command = command;
    registration->mode = mode;
    return true;
}

bool handlebarsc_helper_registry_validate(
    struct handlebarsc_helper_registry * registry,
    bool supported_mode,
    const char ** error
)
{
    if( !registry->options_used ) {
        return true;
    }
    if( !supported_mode ) {
        *error = "External helper options are only valid in compile, module, and execute modes";
        return false;
    }
#ifndef HANDLEBARSC_HAVE_EXEC_HELPERS
    *error = "Executable-backed helpers are not supported on this platform";
    return false;
#endif
#ifndef HANDLEBARS_HAVE_JSON
    for( size_t i = 0; i < registry->count; i++ ) {
        if( registry->registrations[i].mode == handlebarsc_helper_mode_json ) {
            *error = "JSON-backed helpers require JSON support";
            return false;
        }
    }
#endif
    for( size_t i = 0; i < registry->count; i++ ) {
        const char * name = registry->registrations[i].name;
        size_t length = strlen(name);

        if( length > INT_MAX ) {
            *error = "External helper name is too long";
            return false;
        }
        if( registry->allow_override ) {
            continue;
        }
        if( handlebars_builtins_find(name, (unsigned int) length) != NULL ) {
            *error = "External helper registration conflicts with a built-in helper";
            return false;
        }
        for( size_t j = 0; j < i; j++ ) {
            if( strcmp(name, registry->registrations[j].name) == 0 ) {
                *error = "External helper name was registered more than once";
                return false;
            }
        }
    }
    return true;
}

void handlebarsc_helper_registry_apply_compiler(
    struct handlebarsc_helper_registry * registry,
    struct handlebars_context * context,
    struct handlebars_compiler * compiler
)
{
    const char ** builtins;
    const char ** names;
    size_t builtin_count = 0;

    if( registry->count == 0 ) {
        return;
    }
    builtins = handlebars_builtins_names();
    while( builtins[builtin_count] != NULL ) {
        builtin_count++;
    }
    names = talloc_zero_array(
        context,
        const char *,
        builtin_count + registry->count + 1
    );
    HANDLEBARS_MEMCHECK(names, context);
    for( size_t i = 0; i < builtin_count; i++ ) {
        names[i] = builtins[i];
    }
    for( size_t i = 0; i < registry->count; i++ ) {
        names[builtin_count + i] = registry->registrations[i].name;
    }
    handlebars_compiler_set_known_helpers(compiler, names);
}

void handlebarsc_helper_registry_apply_vm(
    struct handlebarsc_helper_registry * registry,
    struct handlebars_vm * vm
)
{
    HANDLEBARS_VALUE_DECL(helpers);
    struct handlebars_map * map;

    if( registry->count == 0 ) {
        return;
    }
    map = handlebars_map_ctor(HBSCTX(vm), registry->count);
    for( size_t i = 0; i < registry->count; i++ ) {
        HANDLEBARS_VALUE_ARRAY_DECL(locals, handlebarsc_helper_local_count);
        HANDLEBARS_VALUE_DECL(callable);
        struct handlebars_closure * closure;
        struct handlebarsc_helper_registration * registration = &registry->registrations[i];

        handlebars_value_str(
            &locals[handlebarsc_helper_local_name],
            handlebars_string_ctor(
                HBSCTX(vm),
                registration->name,
                strlen(registration->name)
            )
        );
        handlebars_value_str(
            &locals[handlebarsc_helper_local_command],
            handlebars_string_ctor(
                HBSCTX(vm),
                registration->command,
                strlen(registration->command)
            )
        );
        handlebars_value_integer(
            &locals[handlebarsc_helper_local_mode],
            registration->mode
        );
        handlebars_value_integer(
            &locals[handlebarsc_helper_local_timeout],
            (long) registry->timeout_ms
        );
        handlebars_value_integer(
            &locals[handlebarsc_helper_local_output_limit],
            (long) registry->output_limit
        );
        closure = handlebars_closure_ctor(
            vm,
            handlebarsc_helper_dispatch,
            handlebarsc_helper_local_count,
            locals
        );
        handlebars_value_closure(callable, closure);
        map = handlebars_map_str_update(
            map,
            registration->name,
            strlen(registration->name),
            callable
        );
        HANDLEBARS_VALUE_UNDECL(callable);
        HANDLEBARS_VALUE_ARRAY_UNDECL(locals, handlebarsc_helper_local_count);
    }
    handlebars_value_map(helpers, map);
    handlebars_vm_set_helpers(vm, helpers);
    HANDLEBARS_VALUE_UNDECL(helpers);
}
