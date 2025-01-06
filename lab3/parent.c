#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <string.h>

#define SHM_NAME "/shared_memory"
#define SEM_NAME_PARENT "/sem_parent"
#define SEM_NAME_CHILD "/sem_child"
#define SHM_SIZE 1024

int main() {
    int shm_fd;
    void *shm_ptr;
    sem_t *sem_parent, *sem_child;
    pid_t pid;

    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        write(STDERR_FILENO, "smem error\n", 11);
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        write(STDERR_FILENO, "ftrun error\n", 12);
        exit(EXIT_FAILURE);
    }

    shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        write(STDERR_FILENO, "mmap error\n", 11);
        exit(EXIT_FAILURE);
    }

    sem_parent = sem_open(SEM_NAME_PARENT, O_CREAT, 0666, 0);
    sem_child = sem_open(SEM_NAME_CHILD, O_CREAT, 0666, 0);
    if (sem_parent == SEM_FAILED || sem_child == SEM_FAILED) {
        write(STDERR_FILENO, "sem error\n", 10);
        exit(EXIT_FAILURE);
    }

    char filename[256];
    write(STDOUT_FILENO, "Введите имя файла: ", 35);
    ssize_t nbytes = read(STDIN_FILENO, filename, sizeof(filename) - 1);
    if (nbytes <= 0) {
        write(STDERR_FILENO, "read error\n", 11);
        exit(EXIT_FAILURE);
    }
    filename[nbytes - 1] = '\0';

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        write(STDERR_FILENO, "open error\n", 11);
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        write(STDERR_FILENO, "fork error\n", 11);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        close(fd);

        execl("./child", "child", filename, (char *)NULL);
        write(STDERR_FILENO, "execl error\n", 12);
        exit(EXIT_FAILURE);
    } else {
        ssize_t nbytes;

        while ((nbytes = read(fd, shm_ptr, SHM_SIZE - 1)) > 0) {
            ((char *)shm_ptr)[nbytes] = '\0';

            sem_post(sem_parent);
          
            sem_wait(sem_child);

            if (waitpid(pid, NULL, WNOHANG) > 0) {
                break;
            }
        }
        
        strcpy((char *)shm_ptr, "");
        sem_post(sem_parent);

        wait(NULL);

        shm_unlink(SHM_NAME);
        sem_close(sem_parent);
        sem_close(sem_child);
        sem_unlink(SEM_NAME_PARENT);
        sem_unlink(SEM_NAME_CHILD);
        close(fd);
    }

    return 0;
}
