#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 1024
#define SUCCESS_CODE 0
#define NUM_OF_THREADS 4

void *print_message(void *arg) {
    char **message = (char **)arg;
    for (int str_num = 0; message[str_num] != NULL; ++str_num) {
	printf("%s", message[str_num]);
    }
    return NULL;
}

void print_error(int return_code, char *additional_message) {
    char buf[BUF_SIZE];
    strerror_r(return_code, buf, sizeof buf);
    fprintf(stderr, "%s: %s\n", additional_message, buf);
}

int main() {
    int return_code;
    pthread_t threads_id[NUM_OF_THREADS];
    char *messages[][NUM_OF_THREADS] = {
	{"Thread number 1, line 1\n", "Thread number 1, line 2\n", "Thread number 1, line 3\n", NULL},
	{"Thread number 2, line 1\n", "Thread number 2, line 2\n", "Thread number 2, line 3\n", NULL},
	{"Thread number 3, line 1\n", "Thread number 3, line 2\n", "Thread number 3, line 3\n", NULL},
	{"Thread number 4, line 1\n", "Thread number 4, line 2\n", "Thread number 4, line 3\n", NULL}
	};
    for (int thread_num = 0; thread_num < NUM_OF_THREADS; ++thread_num) {
	return_code = pthread_create(&threads_id[thread_num], NULL, print_message, &messages[thread_num]);
    	if (return_code != SUCCESS_CODE) {
            print_error(return_code, "creating thread")
    }
    for (int thread_num = 0; thread_num < NUM_OF_THREADS; ++thread_num) {
	return_code =  pthread_join(threads_id[thread_num], NULL);
	if (return_code != SUCCESS_CODE) {
            print_error(return_code, "joining thread");
    }
    pthread_exit(NULL);
}
