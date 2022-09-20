#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define NUM_OF_STEPS 200000000
#define INDEX_FOR_NUM_OF_THREADS 1
#define BUF_SIZE 1024
#define SUCCESS_CODE 0
#define MAX_INVALID_ARG 0
#define MESSAGE_FOR_INVALID_ARG "Please write number greater than 0\n"
#define NUM_OF_ARGS 2
#define MESSAGE_FOR_INVALID_ARGS_NUM "Please write one argument\n"

typedef struct partial_sum_args {
    int start_index;
    int end_index;
    double result;
} partial_sum_args;

void *calc_partial_sum(void *arg) {
    partial_sum_args *args = (partial_sum_args *)arg;
    double partial_sum = 0;
    for (int i = args->start_index; i < args->end_index; ++i) {
        partial_sum += 1.0 / (i * 4.0 + 1.0);
        partial_sum -= 1.0 / (i * 4.0 + 3.0);
    }
    args->result = partial_sum;
    return NULL;
}

int convert_index_to_number(int index) {
    return index + 1;
}

int join_threads_with_partial_sum(int num_of_threads, pthread_t *threads_id, partial_sum_args *threads_args, double *sum) {
    for (int thread_num = 0; thread_num < num_of_threads; ++thread_num) {
        int return_code = pthread_join(threads_id[thread_num], NULL);
        if (return_code != SUCCESS_CODE) {
    	    return return_code;
	}
        *sum += threads_args[thread_num].result;
    }
    return SUCCESS_CODE;
}

int create_threads_for_partial_sum(int num_of_threads, pthread_t *threads_id, partial_sum_args *threads_args) {
    int iteration_num_for_thread = NUM_OF_STEPS / num_of_threads;
    int additional_iterations = NUM_OF_STEPS % num_of_threads;
    for (int thread_num = 0; thread_num < num_of_threads; ++thread_num) {
        threads_args[thread_num].start_index = thread_num * iteration_num_for_thread; 
        threads_args[thread_num].end_index = threads_args[thread_num].start_index + iteration_num_for_thread;
	if (convert_index_to_number(thread_num) == num_of_threads) { 
             threads_args[thread_num].end_index += additional_iterations;
	}
        int return_code = pthread_create(&threads_id[thread_num], 
                                        NULL, &calc_partial_sum, (void *)&threads_args[thread_num]); 
	if (return_code != SUCCESS_CODE) {
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

int main(int argc, char **argv) {
    // read arg and check for validity
    if (argc != NUM_OF_ARGS) {
	printf(MESSAGE_FOR_INVALID_ARGS_NUM);
	exit(EXIT_SUCCESS);
    }
    int num_of_threads = atoi(argv[INDEX_FOR_NUM_OF_THREADS]);
    if (num_of_threads <= MAX_INVALID_ARG) {
	printf(MESSAGE_FOR_INVALID_ARG);
	exit(EXIT_SUCCESS);
    }

    // allocation of memory
    pthread_t *threads_id = (pthread_t *)malloc(num_of_threads * sizeof(pthread_t));
    partial_sum_args *threads_args = (partial_sum_args *)malloc(num_of_threads * sizeof(partial_sum_args));
    if (threads_id == NULL || threads_args == NULL) {
	perror("malloc error ");
	exit(EXIT_FAILURE);
    } 	

    // create threads and run functions
    int return_code = create_threads_for_partial_sum(num_of_threads, threads_id, threads_args); 
    if (return_code != SUCCESS_CODE) {
	print_error(return_code, "creating thread");
        exit(EXIT_FAILURE); 
    }

    double pi = 0;
   
    // join threads and sum partial sums
    return_code = join_threads_with_partial_sum(num_of_threads, threads_id, threads_args, &pi);
    if (return_code != SUCCESS_CODE) {
	print_error(return_code, "join thread");
        exit(EXIT_FAILURE); 
    }

    // free memory
    free(threads_id);
    free(threads_args);    

    pi = pi * 4.0;
    printf("pi done - %.15g \n", pi);    
    
    exit(EXIT_SUCCESS);
}
