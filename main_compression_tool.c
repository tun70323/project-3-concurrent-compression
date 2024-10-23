#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <time.h>

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

void log_event(const char *filename, pid_t worker_pid, const char *event) {
    FILE *log_file = fopen("compression_log.txt", "a");
    if (log_file == NULL) {
        perror("fopen failed");
        exit(1);
    }

    time_t now = time(NULL);
    char *time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0';

    fprintf(log_file, "[%s] Worker PID %d: %s - %s\n", time_str, worker_pid, event, filename);
    fclose(log_file);
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

        pid_t worker_pid = worker_id + 1;
        log_event(filepath, worker_pid, "Sent to worker");

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
            close(pipes[i][1][1]);

            dup2(pipes[i][0][0], STDIN_FILENO);
            close(pipes[i][0][0]);

            while (1) {
                char filepath[256];
                if (scanf("%s", filepath) == EOF) {
                    break;
                }
                
                if (strcmp(filepath, "EXIT") == 0) {
                    printf("Worker %d: Received EXIT signal. Shutting down.\n", getpid());
                    exit(0);
                }

                printf("Worker %d: Processing %s\n", getpid(), filepath);

                int retval = fork();
                if (retval < 0) {
                    perror("fork failed");
                    exit(1);
                } else if (retval == 0) {
                    char *args[] = {"gzip", filepath, NULL};
                    execvp("gzip", args);

                    perror("execvp failed");
                    exit(1);
                } else {
                    int status;
                    wait(&status);

                    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                        printf("Worker %d: Successfully compressed %s\n", getpid(), filepath);
                        log_event(filepath, getpid(), "Compression completed");
                    } else {
                        fprintf(stderr, "Worker %d: Failed to compress %s - %s\n", getpid(), filepath, strerror(errno));
                    }
                }
            }
        } else {
            close(pipes[i][0][0]);
            close(pipes[i][1][1]);
            close(pipes[i][1][1]);
        }
    }

    distribute_files("./decompressed_files", pipes);

    // Send termination signals
    for (int i = 0; i < 4; i++) {
        dprintf(pipes[i][0][1], "EXIT\n");
        close(pipes[i][0][1]); 
    }

    printf("All files sent, main process exiting.\n");
    return 0;
}
