#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// Function to decompress files
void decompress_files(const char *filename) {
    int retval = fork();
    if (retval < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (retval == 0) {
        char *myargs[] = {"tar", "-xvf", (char *)filename, NULL};
        execvp("tar", myargs);
        perror("ERROR: Could not execute tar");
        exit(1);
    } else {
        int status;
        wait(&status);
        printf("Decompression of %s complete.\n", filename);
    }
}

int main() {
    decompress_files("testfiles.tar.gz");

    int pipes[4][2][2]; // [worker][input/output][read/write]

    // Create pipes for 4 workers
    for(int i = 0; i < 4; i++) {
        if (pipe(pipes[i][0]) == -1 || pipe(pipes[i][1]) == -1) {
            perror("pipe failed");
            exit(1);
        }
        printf("Pipes created for worker %d.\n", i + 1);
    }

    // Fork worker processes
    for(int i = 0; i < 4; i++) {
        int retval = fork();
        if (retval < 0) {
            perror("fork failed");
            exit(1);
        } else if (retval == 0) {
            printf("Worker %d (PID: %d) created.\n", i + 1, getpid());
            exit(0);
        } else {
            printf("Worker %d (PID: %d) forked by main process.\n", i + 1, retval);
        }
    }

    for (int i = 0; i < 4; i++) {
        int status;
        wait(&status);
        printf("Worker %d finished.\n", i + 1);
    }

    printf("All workers finished, exiting main process.\n");
    return 0;
}
