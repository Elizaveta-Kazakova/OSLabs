#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#define BUF_SIZE 1024
#define SUCCESS_CODE 0
#define TIME_TO_SLEEP 2
#define STR_FOR_TERMINATED_THREAD "Thread terminated!\n"
#define NOT_TO_EXECUTE_FUNCTION 0
#define INTERVAL_FOR_PRINTING 1
#define THREAD_MESSAGE "Thread working"

void print_str(void *arg) {
    printf("%s", (char *)arg);
}

void *print_str_endlessly(void *arg) {
    pthread_cleanup_push(print_str, STR_FOR_TERMINATED_THREAD);
    while(true) {
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	printf("%s\n", (char *)arg);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_testcancel();
    }
    pthread_cleanup_pop(NOT_TO_EXECUTE_FUNCTION);
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
    if (sleep_return_value != SUCCESS_CODE) {
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
