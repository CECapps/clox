#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>


#include "../memory.h"
#include "../vm.h"

#include "./ferrors.h"
#include "./userarray.h"


Value cc_function_process_open(int arg_count, Value* args) {
    if (arg_count < 1 || arg_count > 2) { return FERROR_VAL(FE_ARG_COUNT_1_2); }
    if (!IS_STRING(args[0])) { return FERROR_VAL(FE_ARG_1_STRING); }
    if (arg_count == 2 && !IS_USERARRAY(args[1])) { return FERROR_VAL(FE_ARG_2_ARRAY); }

    ObjString* command = AS_STRING(args[0]);
    ObjUserArray* command_args;
    int cmd_size = 1;
    if (arg_count == 2) {
        command_args = AS_USERARRAY(args[1]);
        cmd_size += command_args->inner.count;
    }
    char* cmd[cmd_size + 1];
    cmd[0] = command->chars;
    if (arg_count == 2) {
        for (int i = 0; i < command_args->inner.count; i++) {
            if (!IS_STRING(command_args->inner.values[i])) {
                continue;
            }
            cmd[1 + i] = AS_CSTRING(command_args->inner.values[i]);
        }
    }
    cmd[cmd_size] = NULL;

    #define READING_END 0
    #define WRITING_END 1

    // We need to keep track of not just failure or success, but also the error
    // as it happens.  A failure when creating the stderr pipe needs to close
    // down the previous pipes, then report the original pipe error to the
    // calling code, without the possibility of reporting a bogus errno from,
    // say, a botched close().
    errno = 0;
    int stdin_pipe[2];
    int stdin_ok = pipe(stdin_pipe);
    int stdin_errno = errno;

    errno = 0;
    int stdout_pipe[2];
    int stdout_ok = pipe(stdout_pipe);
    int stdout_errno = errno;

    errno = 0;
    int stderr_pipe[2];
    int stderr_ok = pipe(stderr_pipe);
    int stderr_errno = errno;

    if(stdin_ok != 0 || stdout_ok != 0 || stderr_ok != 0) {
        if(stdin_ok == 0)  { close(stdin_pipe[READING_END]);  close(stdin_pipe[WRITING_END]);  }
        if(stdout_ok == 0) { close(stdout_pipe[READING_END]); close(stdout_pipe[WRITING_END]); }
        if(stderr_ok == 0) { close(stderr_pipe[READING_END]); close(stderr_pipe[WRITING_END]); }
        if(stdin_errno)  { return FERROR_ERRNO_VAL(FE_PROCESS_PIPE_CREATE_FAILED, stdin_errno);  }
        if(stdout_errno) { return FERROR_ERRNO_VAL(FE_PROCESS_PIPE_CREATE_FAILED, stdout_errno); }
        if(stderr_errno) { return FERROR_ERRNO_VAL(FE_PROCESS_PIPE_CREATE_FAILED, stderr_errno); }
    }

    int pid = fork();
    if(pid > 0) {
        // We're in the parent post-fork.  Close our connections to the wrong
        // end of each pipe.
        close(stdin_pipe[READING_END]);
        close(stdout_pipe[WRITING_END]);
        close(stderr_pipe[WRITING_END]);

        ObjFileHandle* stdin_h =  newFileHandle(fdopen(stdin_pipe[WRITING_END], "w"));
        stdin_h->is_open = true;
        stdin_h->is_writer = true;

        ObjFileHandle* stdout_h = newFileHandle(fdopen(stdout_pipe[READING_END], "r"));
        stdout_h->is_open = true;
        stdout_h->is_reader = true;

        ObjFileHandle* stderr_h = newFileHandle(fdopen(stderr_pipe[READING_END], "r"));
        stderr_h->is_open = true;
        stderr_h->is_reader = true;

        ObjUserArray* ua = newUserArray();
        ua_grow(ua, 4);
        ua->inner.values[0] = NUMBER_VAL(pid);
        ua->inner.values[1] = OBJ_VAL(stdin_h);
        ua->inner.values[2] = OBJ_VAL(stdout_h);
        ua->inner.values[3] = OBJ_VAL(stderr_h);
        ua->inner.count = 4;
        return OBJ_VAL(ua);
    } else if(pid == 0) {
        // We're in the child post-fork.  Close our connections to the wrong end
        // of each pipe, then reconnect the correct end where it belongs.
        // dup2() will automatically close the target original file descriptor.
        // These calls can fail, but I'm not yet sure how to realistically deal
        // with sending the error somewhere that matters.  Like, what happens if
        // the call to replace stderr fails?  Where do I report that?
        close(stdin_pipe[WRITING_END]);
        dup2(stdin_pipe[READING_END], 0);

        close(stdout_pipe[READING_END]);
        dup2(stdout_pipe[WRITING_END], 1);

        close(stderr_pipe[READING_END]);
        dup2(stderr_pipe[WRITING_END], 2);

        // Execute!
        execvp(cmd[0], cmd);
        // We should not get control back, but if we do for some reason,
        exit(EXIT_FAILURE);
    } else {
        return FERROR_AUTOERRNO_VAL(FE_PROCESS_FORK_FAILED);
    }

    return FERROR_AUTOERRNO_VAL(FE_PROCESS_CREATE_FAILED);
    #undef READING_END
    #undef WRITING_END
}


Value cc_function_process_close(int arg_count, Value* args) {
    if(arg_count != 1) { return FERROR_VAL(FE_ARG_COUNT_1); }
    if(!IS_NUMBER(args[0])) { return FERROR_VAL(FE_ARG_1_NUMBER); }

    int pid = AS_NUMBER(args[0]);
    int status = 0;
    int res = waitpid(pid, &status, WNOHANG);

    if(res == 0) {
        // No status information was reported.  The process is probably still
        // running.  Let's make it not run any more, waiting until it stops.
        kill(pid, SIGTERM);
        status = 0;
        res = waitpid(pid, &status, 0);
    }

    if(res != pid) {
        // Nope, it's still not dead.  It should be dead.
        return FERROR_AUTOERRNO_VAL(FE_PROCESS_CLOSE_FAILED);
    }

    ObjUserArray* ua = newUserArray();
    ua_grow(ua, 2);
    if(WIFEXITED(status)) {
        // The process exited normally.  Return the status value.
        ua->inner.values[0] = BOOL_VAL(true);
        ua->inner.values[1] = NUMBER_VAL(WEXITSTATUS(status));
    } else {
        // We're expressly not passing WUNTRACED or WCONTINUED, telling waitpid()
        // that we do not care about stopped or continued processes.  In this
        // case, the spec says that either WIFEXITED() or WIFSIGNALED() will
        // be true.  Given that WIFEXITED() is false, the child process must
        // have died a death by signal.  Report the killing signal.
        ua->inner.values[0] = BOOL_VAL(false);
        ua->inner.values[1] = NUMBER_VAL(WTERMSIG(status));
    }
    ua->inner.count = 2;
    return OBJ_VAL(ua);
}


void cc_register_ext_process() {
    defineNative("process_open",  cc_function_process_open);
    defineNative("process_close", cc_function_process_close);
}
