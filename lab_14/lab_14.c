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
#define DO_NOT_TO_EXECUTE_FUNCTION 0

struct print_args {
    char *message;
    int num_of_str;
    int index_of_semaphore_to_wait;
    pthread_t another_thread_id;
};

struct destroy_args {
    sem_t *semaphores;
    int num_of_semaphores;
    int destroy_res;
};

sem_t semaphores[NUM_OF_SEMAPHORES];

void destroy_semaphores(void *arg) {
    int return_code;
    struct destroy_args *args = (struct destroy_args *)arg;
    args->destroy_res = SUCCESS_CODE;
    for (int sem_num = 0; sem_num < args->num_of_semaphores; ++sem_num) {  
	int return_code = sem_destroy(&args->semaphores[sem_num]);
	if (return_code != SUCCESS_CODE && args->destroy_res == SUCCESS_CODE) {
	    args->destroy_res = return_code;
	}
    }
}

void *print_n_str(void *arg) {
    int return_code;
    struct print_args *args = (struct print_args *)arg;
    int str_number = 0;
    int index_of_semaphore_to_wait = args->index_of_semaphore_to_wait;
    int index_of_semaphore_to_post = (index_of_semaphore_to_wait + 1) % (NUM_OF_SEMAPHORES);
    while (str_number < args->num_of_str) {
        return_code = sem_wait(&semaphores[index_of_semaphore_to_wait]);
	if (return_code != SUCCESS_CODE) {
	    perror("waiting for a semaphore");
	    pthread_cancel(args->another_thread_id);
	    break;
	}
	printf("%s with %d number of string\n", args->message, str_number);
	return_code = sem_post(&semaphores[index_of_semaphore_to_post]);
	if (return_code != SUCCESS_CODE) {
            perror("posting semaphore");
            pthread_cancel(args->another_thread_id);
	    break;
        }
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
	    struct destroy_args args_for_destroy= {semaphores, sem_num};
	    destroy_semaphores((void *)&args_for_destroy);
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
    struct destroy_args args_for_destroy_all_sems = {semaphores, NUM_OF_SEMAPHORES};
    pthread_cleanup_push(destroy_semaphores, (void *)&args_for_destroy_all_sems);
    int return_code;
    pthread_t main_thread_id = pthread_self();
    unsigned int initial_value_of_semaphores[] = {INITIAL_VALUE_OF_FISRT_SEM, INITIAL_VALUE_OF_SECOND_SEM, 
							main_thread_id};
    return_code = init_semaphores(semaphores, NUM_OF_SEMAPHORES, initial_value_of_semaphores);
    if (return_code != SUCCESS_CODE) {
	perror("initializing semaphores");
	exit(EXIT_FAILURE);
    }
    pthread_t thread_id;
    struct print_args args_for_child = {CHILD_MESSAGE, NUM_OF_STR, SEMAPHORE_INDEX_FOR_CHILD};
    return_code = pthread_create(&thread_id, NULL, print_n_str, (void *)&args_for_child);
    if (return_code != SUCCESS_CODE) {
	destroy_semaphores((void *)&args_for_destroy_all_sems);
        print_error(return_code, "creating thread");
        exit(EXIT_FAILURE);
    }
    struct print_args args_for_parent = {PARENT_MESSAGE, NUM_OF_STR, SEMAPHORE_INDEX_FOR_PARENT, thread_id};
    print_n_str((void *)&args_for_parent);
    return_code = pthread_join(thread_id, NULL);
    if (return_code != SUCCESS_CODE) {
	destroy_semaphores((void *)&args_for_destroy_all_sems);
        print_error(return_code, "joining thread");
        exit(EXIT_FAILURE);
    }
    destroy_semaphores((void *)&args_for_destroy_all_sems);
    if (args_for_destroy_all_sems.destroy_res != SUCCESS_CODE) {
	perror("destroy semaphores");
	exit(EXIT_FAILURE);
    }
    pthread_cleanup_pop(DO_NOT_TO_EXECUTE_FUNCTION);
    exit(EXIT_SUCCESS);
}

