#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>

#define NUM_OF_STR 10
#define BUF_SIZE 1024
#define SUCCESS_CODE 0
#define ERROR_CODE 1
#define CHILD_MESSAGE "New thread"
#define PARENT_MESSAGE "Main thread"
#define NUM_OF_THREADS 2
#define NUM_OF_SEMAPHORES 2
#define SHARED_BETWEEN_THREADS_OF_A_PROCCESS 0
#define SEMAPHORE_INDEX_FOR_CHILD 0
#define SEMAPHORE_INDEX_FOR_PARENT 1
#define INITIAL_VALUE_OF_FISRT_SEM 0
#define INITIAL_VALUE_OF_SECOND_SEM 1


struct print_args {
    char *message;
    int num_of_str;
    int index_of_semaphore_to_wait;
};

sem_t semaphores[NUM_OF_SEMAPHORES];

int destroy_semaphores(sem_t *semaphores, int num_of_semaphores) {
    int destroy_res = SUCCESS_CODE;
    for (int sem_num = 0; sem_num < num_of_semaphores; ++sem_num) {  
	int return_code = sem_destroy(&semaphores[sem_num]);
	if (return_code != SUCCESS_CODE && destroy_res == SUCCESS_CODE) {
	    destroy_res = return_code;
	}
    }
    return destroy_res;
}

void *print_n_str(void *arg) {
    struct print_args *args = (struct print_args *)arg;
    int str_number = 0;
    int index_of_semaphore_to_wait = args->index_of_semaphore_to_wait;
    int index_of_semaphore_to_post = (index_of_semaphore_to_wait + 1) % (NUM_OF_SEMAPHORES);
    while (str_number < args->num_of_str) {
        sem_wait(&semaphores[index_of_semaphore_to_wait]);
	printf("%s with %d number of string\n", args->message, str_number);
	sem_post(&semaphores[index_of_semaphore_to_post]);
        ++str_number;
    }
    return NULL;
}

int init_semaphores(sem_t *semaphores, int num_of_semaphores, unsigned int *initial_value_of_semaphores) {
    int return_code;
     for (int sem_num = 0; sem_num < num_of_semaphores; ++sem_num) {  
	return_code = sem_init(&semaphores[sem_num], SHARED_BETWEEN_THREADS_OF_A_PROCCESS, 
							initial_value_of_semaphores[sem_num]);
	if (return_code != SUCCESS_CODE) {
	    destroy_semaphores(semaphores, sem_num);
	    return return_code;
	}
    }
    return SUCCESS_CODE;
}

void print_error(int return_code, char *additional_message) {
    char buf[BUF_SIZE];
    strerror_r(return_code, buf, sizeof buf);
    fprintf(stderr, "%s: %s\n", additional_message, buf);
}

int main() {
    int return_code;
    unsigned int initial_value_of_semaphores[] = {INITIAL_VALUE_OF_FISRT_SEM, INITIAL_VALUE_OF_SECOND_SEM};
    return_code = init_semaphores(semaphores, NUM_OF_SEMAPHORES, initial_value_of_semaphores);
    if (return_code != SUCCESS_CODE) {
	perror("initializing semaphores");
	exit(EXIT_FAILURE);
    }
    pthread_t thread_id;
    struct print_args args_for_child = {CHILD_MESSAGE, NUM_OF_STR, SEMAPHORE_INDEX_FOR_CHILD};
    return_code = pthread_create(&thread_id, NULL, print_n_str, (void *)&args_for_child);
    if (return_code != SUCCESS_CODE) {
	destroy_semaphores(semaphores, NUM_OF_SEMAPHORES);
        print_error(return_code, "creating thread");
        exit(EXIT_FAILURE);
    }
    struct print_args args_for_parent = {PARENT_MESSAGE, NUM_OF_STR, SEMAPHORE_INDEX_FOR_PARENT};
    print_n_str((void *)&args_for_parent);
    return_code = pthread_join(thread_id, NULL);
    if (return_code != SUCCESS_CODE) {
	destroy_semaphores(semaphores, NUM_OF_SEMAPHORES);
        print_error(return_code, "joining thread");
        exit(EXIT_FAILURE);
    }
    return_code = destroy_semaphores(semaphores, NUM_OF_SEMAPHORES);
    if (return_code != SUCCESS_CODE) {
	perror("destroy semaphores");
	exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

