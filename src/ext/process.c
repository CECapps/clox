#include <stdlib.h>
#include <unistd.h>

#include "../memory.h"
#include "../vm.h"

#include "./ferrors.h"
#include "./userarray.h"


Value cc_function_process_open(int arg_count, Value* args) {
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

        struct flock dummy_lock = { .l_type = F_UNLCK };
        ObjFileHandle* stdin_h =  newFileHandle(fdopen(stdin_pipe[WRITING_END], "w"), dummy_lock);
        ObjFileHandle* stdout_h = newFileHandle(fdopen(stdout_pipe[READING_END], "r"), dummy_lock);
        ObjFileHandle* stderr_h = newFileHandle(fdopen(stderr_pipe[READING_END], "r"), dummy_lock);

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
        char* END = (char *)0;
        char* cmd[] = { "ls", "-alF", "/tmp", END };
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


void cc_register_ext_process() {
    defineNative("process_open", cc_function_process_open);
}
