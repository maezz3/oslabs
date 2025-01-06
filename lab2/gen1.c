#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>

// Структура для передачи данных в поток
typedef struct {
    int* array;
    int start;
    int end;
    int local_min;
    int local_max;
} ThreadData;

int global_min = 10000;
int global_max = -10000;
pthread_mutex_t mutex;


void* find_min_max(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    data->local_min = 10000;
    data->local_max = -10000;

    for (int i = data->start; i < data->end; i++) {
        if (data->array[i] < data->local_min) {
            data->local_min = data->array[i];
        }
        if (data->array[i] > data->local_max) {
            data->local_max = data->array[i];
        }
    }

    pthread_mutex_lock(&mutex);
    if (data->local_min < global_min) {
        global_min = data->local_min;
    }
    if (data->local_max > global_max) {
        global_max = data->local_max;
    }
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, char* argv[]) {
    pthread_mutex_init(&mutex, NULL);
    if (argc != 3) {
        const char msg[] = "Неправильный ввод аргументов\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
        return 1;
    }

    int array_size = atoi(argv[1]);
    int max_threads = atoi(argv[2]);

    if (array_size <= 0 || max_threads <= 0) {
        const char msg[] = "Размер массива и количество потоков должны быть положительными числами.\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
        return 1;
    }

    srand(time(NULL));

    int* array = malloc(array_size * sizeof(int));
    if (!array) {
        free(array);
        const char msg[] = "Ошибка выделения памяти\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
        return -1;
    }

    for (int i = 0; i < array_size; i++) {
        array[i] = (rand() % 19999) - 9999;
    }

    const char msg[] = "Массив: ";
	write(STDERR_FILENO, msg, sizeof(msg));
    for (int i = 0; i < array_size; i++) {
        char num_str[16];
        snprintf(num_str, sizeof(num_str), "%d ", array[i]);
        write(STDOUT_FILENO, num_str, strlen(num_str));
    }

    pthread_t* threads = malloc(max_threads * sizeof(pthread_t));
    ThreadData* thread_data = malloc(max_threads * sizeof(ThreadData));

    if (!threads || !thread_data) {
        free(array);
        free(threads);
        free(thread_data);
        const char msg[] = "Ошибка выделения памяти\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
        return -1;
    }

    int chunk_size = array_size / max_threads;
    int remainder = array_size % max_threads;

    for (int i = 0; i < max_threads; i++) {
        thread_data[i].array = array;
        thread_data[i].start = i * chunk_size;
        thread_data[i].end = (i + 1) * chunk_size;

        if (i == max_threads - 1) {
            thread_data[i].end += remainder; // Добавляем остаток к последнему потоку
        }
        printf("Создаётся поток %d: от %d до %d\n", i + 1, thread_data[i].start, thread_data[i].end);
        pthread_create(&threads[i], NULL, find_min_max, (void*)&thread_data[i]);
    }

    // Ждем завершения всех потоков
    for (int i = 0; i < max_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    free(array);
    free(threads);
    free(thread_data);

    char result_buffer[1024];
    snprintf(result_buffer, sizeof(result_buffer), "\nМинимальный элемент: %d\n", global_min);
    write(STDOUT_FILENO, result_buffer, strlen(result_buffer));

    snprintf(result_buffer, sizeof(result_buffer), "Максимальный элемент: %d\n", global_max);
    write(STDOUT_FILENO, result_buffer, strlen(result_buffer));

    pthread_mutex_destroy(&mutex);

    return 0;
}
