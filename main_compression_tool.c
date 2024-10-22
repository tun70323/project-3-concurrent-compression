#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>

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

// Function to distribute files to workers
void distribute_files(const char *directory, int pipes[4][2][2]) {
    DIR *dir = opendir(directory);
    if (dir == NULL) {
        perror("opendir failed");
        exit(1);
    }

    struct dirent *entry;
    int worker_id = 0;

    // Search directory items
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') // Skip files that start with .
            continue;
        
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", directory, entry->d_name);

        printf("Sending file %s to worker %d\n", filepath, worker_id + 1);
        dprintf(pipes[worker_id][0][1], "%s\n", filepath);

        worker_id = (worker_id + 1) % 4;
    }

    closedir(dir);
}

int main() {
    printf("Starting file decompression...\n");
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
            close(pipes[i][0][1]);
            close(pipes[i][1][0]);

            dup2(pipes[i][0][0], STDIN_FILENO);
            close(pipes[i][0][0]);

            dup2(pipes[i][1][1], STDOUT_FILENO);
            close(pipes[i][1][1]);

            printf("Worker %d (PID: %d) created.\n", i + 1, getpid());

            while (1) {
                char filepath[256];
                if (scanf("%s", filepath) !=EOF) {
                    printf("Received file path: %s\n", filepath);
                }
            }
            exit(0);
        } else {
            close(pipes[i][0][0]);
            close(pipes[i][1][1]);

            printf("Worker %d (PID: %d) forked by main process.\n", i + 1, retval);
        }
    }

    distribute_files("./decompressed_files", pipes);

    printf("All files sent, main process exiting.\n");
    return 0;
}
