#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#define NUM_OF_STR 10
#define BUF_SIZE 1024
#define SUCCESS_CODE 0
#define ERROR_CODE 1
#define CHILD_MESSAGE "New thread"
#define PARENT_MESSAGE "Main thread"
#define NUM_OF_THREADS 2
#define NUM_OF_MUTEXES 2
#define INDEX_FOR_MUTEX_FOR_READY 1
#define INDEX_OF_MUTEX_FOR_MAIN_THREAD 0
#define INDEX_OF_MUTEX_FOR_NEW_THREAD 0

struct print_args {
    char *message;
    int num_of_str;
    int initial_mutex_index_for_thread;
};

pthread_mutex_t mutexes[NUM_OF_MUTEXES];
int num_of_ready_threads = 0;

void destroy_mutexes(pthread_mutex_t *mutexes, int num_of_mutexes) {
    for (int mutex_num = 0; mutex_num < num_of_mutexes; ++mutex_num) {
	pthread_mutex_destroy(&mutexes[mutex_num]);
    }
}

void *print_n_str(void *arg) {
    struct print_args *args = (struct print_args *)arg;
    int str_number = 0;
    int index_of_using_mutex = args->initial_mutex_index_for_thread;
    while (str_number < args->num_of_str) {
	int return_code = pthread_mutex_lock(&mutexes[index_of_using_mutex]);
	if (return_code != SUCCESS_CODE) {
	    perror("lock mutex");
	    return NULL;
	}
        int next_index_of_mutex = (index_of_using_mutex + 1) % (NUM_OF_MUTEXES);
	return_code = pthread_mutex_lock(&mutexes[next_index_of_mutex]);
	if (return_code != SUCCESS_CODE) {
            perror("lock mutex");
            return NULL;
        }
	printf("%s with %d number of string\n", args->message, str_number);
        return_code = pthread_mutex_unlock(&mutexes[index_of_using_mutex]);
	if (return_code != SUCCESS_CODE) {
            perror("lock mutex");
            return NULL;
        }
	return_code = pthread_mutex_unlock(&mutexes[next_index_of_mutex]);
	if (return_code != SUCCESS_CODE) {
            perror("lock mutex");
            return NULL;
        }
        ++str_number;
    }
    return NULL;
}

int init_mutexes(pthread_mutex_t *mutexes, int num_of_mutexes) {
    pthread_mutexattr_t mutex_attr; 
    int return_code = pthread_mutexattr_init(&mutex_attr);
    if (return_code != SUCCESS_CODE) {
        return return_code;
    }
    return_code = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
    if (return_code != SUCCESS_CODE) {
	pthread_mutexattr_destroy(&mutex_attr);
        return return_code;
    }

    for (int mutex_num = 0; mutex_num < num_of_mutexes; ++mutex_num) {  
	return_code = pthread_mutex_init(&mutexes[mutex_num], &mutex_attr);
	if (return_code != SUCCESS_CODE) {
	    destroy_mutexes(mutexes, mutex_num);
	    pthread_mutexattr_destroy(&mutex_attr);
	    return return_code;
	}
    }

    pthread_mutexattr_destroy(&mutex_attr);
    return SUCCESS_CODE;
}

void print_error(int return_code, char *additional_message) {
    char buf[BUF_SIZE];
    strerror_r(return_code, buf, sizeof buf);
    fprintf(stderr, "%s: %s\n", additional_message, buf);
}

int main() {
    int return_code;
    return_code = init_mutexes(mutexes, NUM_OF_MUTEXES);
    if (return_code != SUCCESS_CODE) {
	print_error(return_code, "initializing mutexes");
	exit(EXIT_FAILURE);
    }
    pthread_t thread_id;
    struct print_args args_for_child;
    args_for_child.message = CHILD_MESSAGE;
    args_for_child.num_of_str = NUM_OF_STR;
    args_for_child.initial_mutex_index_for_thread = INDEX_OF_MUTEX_FOR_NEW_THREAD;
    return_code = pthread_create(&thread_id, NULL, print_n_str, (void *)&args_for_child);
    if (return_code != SUCCESS_CODE) {
        print_error(return_code, "creating thread");
        exit(EXIT_FAILURE);
    }
    struct print_args args_for_parent;
    args_for_parent.message = PARENT_MESSAGE;
    args_for_parent.num_of_str = NUM_OF_STR;
    args_for_parent.initial_mutex_index_for_thread  = INDEX_OF_MUTEX_FOR_MAIN_THREAD;
    print_n_str((void *)&args_for_parent);
    return_code = pthread_join(thread_id, NULL);
    if (return_code != SUCCESS_CODE) {
        print_error(return_code, "joining thread");
        exit(EXIT_FAILURE);
    }
    destroy_mutexes(mutexes, NUM_OF_MUTEXES);
    exit(EXIT_SUCCESS);
}

