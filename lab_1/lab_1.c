#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_OF_STR 10
#define BUF_SIZE 1024
#define CHILD_MESSAGE "Child proccess"
#define PARENT_MESSAGE "Parent proccess"

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

int main(int argc, char *argv[]) {
    int err;
    pthread_t tid;
    struct print_args args_for_child;
    args_for_child.message = CHILD_MESSAGE;
    args_for_child.num_of_str = NUM_OF_STR;
    err = pthread_create(&tid, NULL, &print_n_str, (void *)&args_for_child);
    if (err) {
        char buf[BUF_SIZE];
	strerror_r(err, buf, sizeof buf);
	fprintf(stderr, "%s: creating thread : %s\n", argv[0], buf); 
        exit(1);
    }
    struct print_args args_for_parent;
    args_for_parent.message = PARENT_MESSAGE;
    args_for_parent.num_of_str = NUM_OF_STR;
    print_n_str((void *)&args_for_parent);
    pthread_exit(NULL);
}
