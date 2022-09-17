#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_OF_STR 10
#define BUF_SIZE 1024
#define SUCCESS_CODE 0
#define CHILD_MESSAGE "New thread"
#define PARENT_MESSAGE "Main thread"

struct print_args {
        char *message;
        int num_of_str;
};


void *print_n_str(void *arg) {
    struct print_args *args = (struct print_args *)arg;
    for (int str_number = 0; str_number < args->num_of_str; ++str_number) {
        printf("%s with %d number of string\n", args->message, str_number);
    }
    return NULL;
}


int main() {
    int return_code;
    pthread_t thread_id;
    struct print_args args_for_child;
    args_for_child.message = CHILD_MESSAGE;
    args_for_child.num_of_str = NUM_OF_STR;
    return_code = pthread_create(&thread_id, NULL, &print_n_str, (void *)&args_for_child);
    if (return_code != SUCCESS_CODE) {
        char buf[BUF_SIZE];
	strerror_r(return_code, buf, sizeof buf);
	fprintf(stderr, "creating thread %s\n", buf);
        exit(EXIT_FAILURE);
    }
    return_code = pthread_join(thread_id, NULL);
    if (return_code != SUCCESS_CODE) {
	char buf[BUF_SIZE];
	strerror_r(return_code, buf, sizeof buf);
	fprintf(stderr, "joining thread : %s\n", buf);
	exit(EXIT_FAILURE);
    }
    struct print_args args_for_parent;
    args_for_parent.message = PARENT_MESSAGE;
    args_for_parent.num_of_str = NUM_OF_STR;
    print_n_str((void *)&args_for_parent);
    exit(EXIT_SUCCESS);
}

