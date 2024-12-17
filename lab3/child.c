#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <string.h>

#define SHM_NAME "/shared_memory"
#define SEM_NAME_PARENT "/sem_parent"
#define SEM_NAME_CHILD "/sem_child"
#define SHM_SIZE 4096

int is_prime(int n) {
    if (n <= 1) return 0;
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    int shm_fd;
    void *shm_ptr;
    sem_t *sem_parent, *sem_child;

    shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        write(STDERR_FILENO, "smem error\n", 11);
        exit(EXIT_FAILURE);
    }
  
    shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        write(STDERR_FILENO, "mmap error\n", 11);
        exit(EXIT_FAILURE);
    }

    sem_parent = sem_open(SEM_NAME_PARENT, 0);
    sem_child = sem_open(SEM_NAME_CHILD, 0);
    if (sem_parent == SEM_FAILED || sem_child == SEM_FAILED) {
        write(STDERR_FILENO, "sem error\n", 10);
        exit(EXIT_FAILURE);
    }

    int number;

    while (1) {
        sem_wait(sem_parent);

        if (strlen((char *)shm_ptr) == 0) { 
            sem_post(sem_child);
            break;
        }

        char *line = strtok(shm_ptr, "\n");
        while (line != NULL) {
            number = atoi(line);
            if (number < 0 || is_prime(number)) {
                sem_post(sem_child);
                sem_close(sem_parent);
                sem_close(sem_child);
                munmap(shm_ptr, SHM_SIZE);
                exit(EXIT_SUCCESS);
            } else {
                char num_str[12];
                snprintf(num_str, sizeof(num_str), "%d\n", number);
                write(STDOUT_FILENO, num_str, strlen(num_str));
            }
            line = strtok(NULL, "\n");
        }

        sem_post(sem_child);
    }
    
    sem_close(sem_parent);
    sem_close(sem_child);
    munmap(shm_ptr, SHM_SIZE);
    return 0;
}
