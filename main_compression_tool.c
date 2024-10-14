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
        char *myargs[] = {"tar", "-xvf", filename, NULL};
        execvp("tar", myargs);
        perror("ERROR: Could not execute tar");
        exit(1);
    } else {
        int status;
        wait(&status);
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
    }

    // Fork worker processes
    for(int i = 0; i < 4; i++) {
        int retval = fork();
        if (retval < 0) {
            perror("fork failed");
            exit(1);
        } else if (retval == 0) {
            exit(0);
        }
    }

    return 0;
}
