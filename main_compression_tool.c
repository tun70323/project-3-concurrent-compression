#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

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
    return 0;
}
