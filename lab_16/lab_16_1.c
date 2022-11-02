#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#define NUM_OF_STR 10
#define BUF_SIZE 1024
#define SUCCESS_CODE 0
#define ERROR_CODE 1
#define FIRST_PROC_MESSAGE "First process"
#define NUM_OF_THREADS 2
#define NUM_OF_SEMAPHORES 2
#define SEMAPHORE_INDEX_FOR_FIRST_PROCESS 1
#define SEMAPHORE_INDEX_FOR_SECOND_PROCESS 0
#define INITIAL_VALUE_OF_FISRT_SEM 0
#define INITIAL_VALUE_OF_SECOND_SEM 1
#define FIRST_SEMAPHORE_NAME "/first-semaphore"
#define SECOND_SEMAPHORE_NAME "/second-semaphore"
#define CHILD_PROCESS 0
#define WAIT_TIMEOUT_IN_SECONDS 128

struct print_args {
    char *message;
    int num_of_str;
    int index_of_semaphore_to_wait;
};

sem_t *semaphores[NUM_OF_SEMAPHORES];

int close_semaphores(sem_t **semaphores, int num_of_semaphores) {
    int close_res = SUCCESS_CODE;
    for (int sem_num = 0; sem_num < num_of_semaphores; ++sem_num) {
	int return_code = sem_close(semaphores[sem_num]);
	if (return_code != SUCCESS_CODE) {
	    close_res = return_code;
	}
    }
    return close_res;
}

int unlink_semaphores(char **semaphore_names, int num_of_semaphores) {
    int unlink_res = SUCCESS_CODE;
    for (int sem_num = 0; sem_num < num_of_semaphores; ++sem_num) {
        int return_code = sem_unlink(semaphore_names[sem_num]);
        if (return_code != SUCCESS_CODE) {
            unlink_res = return_code;
        }
    }
    return unlink_res;
}

int print_n_str(void *arg) {
    struct print_args *args = (struct print_args *)arg;
    int str_number = 0;
    int index_of_semaphore_to_wait = args->index_of_semaphore_to_wait;
    int index_of_semaphore_to_post = (index_of_semaphore_to_wait + 1) % (NUM_OF_SEMAPHORES);
    struct timespec time;
    int return_code = clock_gettime(CLOCK_REALTIME, &time);
    if (return_code != SUCCESS_CODE) {
	perror("clock_gettime");
	return return_code;
    }
    time.tv_sec += WAIT_TIMEOUT_IN_SECONDS;
    while (str_number < args->num_of_str) {
        return_code = sem_timedwait(semaphores[index_of_semaphore_to_wait], &time);
	if (return_code != SUCCESS_CODE) {
	    perror("sem_timedwait");
	    return return_code;
	}
	printf("%s with %d number of string\n", args->message, str_number);
	return_code = sem_post(semaphores[index_of_semaphore_to_post]);
	if (return_code != SUCCESS_CODE) {
	    perror("sem_post");
	    return return_code;
	}
        ++str_number;
    }
    return SUCCESS_CODE;
}

int destroy_semaphores(sem_t **semaphores, char **semaphore_names, int num_of_semaphores) {
    int return_code;
    int destroy_res = SUCCESS_CODE;

    return_code = close_semaphores(semaphores, NUM_OF_SEMAPHORES);
    if (return_code != SUCCESS_CODE) {
        perror("close_semaphores");
	destroy_res = return_code;
    }

    return_code = unlink_semaphores(semaphore_names, NUM_OF_SEMAPHORES);
    if (return_code != SUCCESS_CODE) {
        perror("unlink_semaphores");
	destroy_res = return_code;
    }

    return destroy_res;
}

int init_semaphores(sem_t **semaphores, int num_of_semaphores, unsigned int *initial_value_of_semaphores, 
				char **semaphore_names) {
    for (int sem_num = 0; sem_num < num_of_semaphores; ++sem_num) {  
	semaphores[sem_num] = sem_open(semaphore_names[sem_num], O_CREAT, S_IRWXO | S_IRWXG | S_IRWXU, 
							initial_value_of_semaphores[sem_num]);
	if (semaphores[sem_num] == SEM_FAILED) {
	    destroy_semaphores(semaphores, semaphore_names, sem_num);
	    return ERROR_CODE;
	}
    }
    return SUCCESS_CODE;
}



int main() {
    int return_code;
    unsigned int initial_value_of_semaphores[] = {INITIAL_VALUE_OF_FISRT_SEM,
							 INITIAL_VALUE_OF_SECOND_SEM};
    char *semaphore_names[] = {FIRST_SEMAPHORE_NAME, SECOND_SEMAPHORE_NAME};
    return_code = init_semaphores(semaphores, NUM_OF_SEMAPHORES, initial_value_of_semaphores, 
					semaphore_names);
    if (return_code != SUCCESS_CODE) {
	perror("open semaphores");
	exit(EXIT_FAILURE);
    }

    struct print_args args_for_first_process = {FIRST_PROC_MESSAGE, NUM_OF_STR,
							 SEMAPHORE_INDEX_FOR_FIRST_PROCESS};
    return_code = print_n_str((void *)&args_for_first_process);
    if (return_code != SUCCESS_CODE) {
	destroy_semaphores(semaphores, semaphore_names, NUM_OF_SEMAPHORES);
        exit(EXIT_FAILURE);
    }
    return_code = destroy_semaphores(semaphores, semaphore_names, NUM_OF_SEMAPHORES);
    if (return_code != SUCCESS_CODE) {
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}
