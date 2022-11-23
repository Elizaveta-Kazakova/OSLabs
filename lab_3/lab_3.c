#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 1024
#define SUCCESS_CODE 0
#define NUM_OF_THREADS 4
#define NUM_OF_STR 4

typedef struct print_message_args {
    char **message;
    int num_of_strings;
} print_message_args;

void *print_message(void *arg) {
    print_message_args *args = (print_message_args *)arg;
    for (int str_num = 0; str_num < args->num_of_strings; ++str_num) {
	printf("%s", args->message[str_num]);
    }
    return NULL;
}

int join_threads(pthread_t *thread_ids, int num_of_threads) {
    int return_code = SUCCESS_CODE;
    for (int thread_num = 0; thread_num < num_of_threads; ++thread_num) {
	int join_code =  pthread_join(thread_ids[thread_num], NULL);
        if (join_code != SUCCESS_CODE && return_code == SUCCESS_CODE) {
	    return_code = join_code;
        }
    }
    return return_code;
}

void print_error(int return_code, char *additional_message) {
    char buf[BUF_SIZE];
    strerror_r(return_code, buf, sizeof buf);
    fprintf(stderr, "%s: %s\n", additional_message, buf);
}

int main() {
    int return_code;
    pthread_t thread_ids[NUM_OF_THREADS];
    char *messages[NUM_OF_THREADS][NUM_OF_STR] = {
	{"Thread number 1, line 1\n", "Thread number 1, line 2\n", "Thread number 1, line 3\n", "Thread number 1, line 4\n"},
	{"Thread number 2, line 1\n", "Thread number 2, line 2\n", "Thread number 2, line 3\n", "Thread number 2, line 4\n"},
	{"Thread number 3, line 1\n", "Thread number 3, line 2\n", "Thread number 3, line 3\n", "Thread number 3, line 4\n"},
	{"Thread number 4, line 1\n", "Thread number 4, line 2\n", "Thread number 4, line 3\n", "Thread number 4, line 4\n"}
	};
    print_message_args message_args[NUM_OF_THREADS];
    for (int thread_num = 0; thread_num < NUM_OF_THREADS; ++thread_num) {
	message_args[thread_num].message = messages[thread_num];
	message_args[thread_num].num_of_strings = NUM_OF_STR;
	return_code = pthread_create(&thread_ids[thread_num], NULL, print_message, (void *)&message_args[thread_num]);
    	if (return_code != SUCCESS_CODE) {
            print_error(return_code, "creating thread");
	    join_threads(thread_ids, thread_num);
	    exit(EXIT_FAILURE);
	}
    }
    return_code = join_threads(thread_ids, NUM_OF_THREADS);
    if (return_code != SUCCESS_CODE) {
	print_error(return_code, "joining threads");
	exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}
