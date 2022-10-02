#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#define BUF_SIZE 1024
#define SUCCESS_CODE 0
#define SLEEP_SUCCESS 0
#define TIME_TO_SLEEP 2
#define THREAD_MESSAGE "Thread working"

void *print_str_endlessly(void *arg) {
    while(true) {
	printf("%s\n", (char *)arg);
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
    pthread_t thread_id;
    return_code = pthread_create(&thread_id, NULL, print_str_endlessly, THREAD_MESSAGE);
    if (return_code != SUCCESS_CODE) {
        print_error(return_code, "creating thread");
        exit(EXIT_FAILURE);
    }
    unsigned int sleep_return_value = sleep(TIME_TO_SLEEP);
    if (sleep_return_value != SLEEP_SUCCESS) {
        perror("sleep error");
        exit(EXIT_FAILURE);
    }
    return_code = pthread_cancel(thread_id);
    if (return_code != SUCCESS_CODE) {
        print_error(return_code, "cancelling thread");
        exit(EXIT_FAILURE);
    }
    return_code = pthread_join(thread_id, NULL);
     if (return_code != SUCCESS_CODE) {
        print_error(return_code, "joining thread");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

