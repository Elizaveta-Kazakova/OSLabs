#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define NUM_OF_STEPS 200000000
#define INDEX_FOR_NUM_OF_THREADS 1
#define BUF_SIZE 1024
#define SUCCESS_CODE 0
#define ERROR_CODE 1
#define MIN_NUM_OF_THREADS 1
#define MESSAGE_FOR_INVALID_RANGE_OF_ARG "Please write number greater than 0 and smaller than 30001\n"
#define NUM_OF_ARGS 2
#define MESSAGE_FOR_INVALID_ARGS_NUM "Please write one argument\n"
#define MAX_NUM_OF_THREADS 30000
#define BASE 10
#define MESSAGE_FOR_INVALID_ARG "Please write an integer\n"
#define TERMINATING_SYMBOL_OF_STR '\0'
#define INDEX_OF_FIRST_THREAD 0

typedef struct partial_sum_args {
    int start_index;
    int num_of_iterations;
   double result;
} partial_sum_args;

void *calc_partial_sum(void *arg) {
    partial_sum_args *args = (partial_sum_args *)arg;
    double partial_sum = 0;
    for (int i = args->start_index; i < args->start_index + args->num_of_iterations; ++i) {
        partial_sum += 1.0 / (i * 4.0 + 1.0);
        partial_sum -= 1.0 / (i * 4.0 + 3.0);
    }
    args->result = partial_sum;
    pthread_exit(NULL);
}

void join_threads(pthread_t *threads_id, int start_index, int num_of_iterations) {
    for (int thread_num = start_index; thread_num < start_index +
							 num_of_iterations; ++thread_num) {
        pthread_join(threads_id[thread_num], NULL);
    }
}

int join_threads_with_partial_sum(int num_of_threads, pthread_t *threads_id,
						 partial_sum_args *threads_args, double *sum) {
    for (int thread_num = 0; thread_num < num_of_threads; ++thread_num) {
        int return_code = pthread_join(threads_id[thread_num], NULL);
	if (return_code != SUCCESS_CODE) {
	    join_threads(threads_id, thread_num + 1, num_of_threads);
    	    return return_code;
	}
        *sum += threads_args[thread_num].result;
    }
    return SUCCESS_CODE;
}

int create_threads_for_partial_sum(int num_of_threads, pthread_t *threads_id,
						 partial_sum_args *threads_args) {
    int iteration_num_for_thread = NUM_OF_STEPS / num_of_threads;
    int num_of_additional_iterations = NUM_OF_STEPS % num_of_threads;
    
    // give each thread its indexes for calculations
    for (int thread_num = 0; thread_num < num_of_threads; ++thread_num) {
        threads_args[thread_num].start_index = thread_num * iteration_num_for_thread; 
        threads_args[thread_num].num_of_iterations = iteration_num_for_thread;

	// distribute additional iterations between the first n threads, 
	// where n is the number of additional iterations
	if (thread_num < num_of_additional_iterations) { 
             ++threads_args[thread_num].num_of_iterations;
	}
        int return_code = pthread_create(&threads_id[thread_num], 
                               NULL, calc_partial_sum, (void *)&threads_args[thread_num]); 
	if (return_code != SUCCESS_CODE) {
	    join_threads(threads_id, INDEX_OF_FIRST_THREAD, thread_num);
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

void free_memory(pthread_t *threads_id, partial_sum_args *threads_args) {
    free(threads_id);
    free(threads_args);
    threads_id = NULL;
    threads_args = NULL;
}

int check_input(int num_of_args, char *arg) {
    if (num_of_args != NUM_OF_ARGS) {
        printf(MESSAGE_FOR_INVALID_ARGS_NUM);
	return ERROR_CODE;
    }
    char *endptr;
    long num_of_threads = strtol(arg, &endptr, BASE);
    // if there are invalid characters endptr will contain first of them
    if (*endptr != TERMINATING_SYMBOL_OF_STR) {
	printf(MESSAGE_FOR_INVALID_ARG);
	return ERROR_CODE;
    }

    if (num_of_threads < MIN_NUM_OF_THREADS || num_of_threads > MAX_NUM_OF_THREADS) {
        printf(MESSAGE_FOR_INVALID_RANGE_OF_ARG);
        return ERROR_CODE;
    }
    return SUCCESS_CODE;
}

int calculate_pi(int num_of_threads, double *pi) {
    // allocation of memory
    pthread_t *threads_id = (pthread_t *)malloc(num_of_threads * sizeof(pthread_t));
    partial_sum_args *threads_args = (partial_sum_args *)malloc(num_of_threads
                                                                 * sizeof(partial_sum_args));
    if (threads_id == NULL || threads_args == NULL) {
	free_memory(threads_id, threads_args);
        perror("malloc error ");
        return ERROR_CODE;
    }   

    // create threads and run functions
    int return_code = create_threads_for_partial_sum(num_of_threads, threads_id, threads_args); 
    if (return_code != SUCCESS_CODE) {
        print_error(return_code, "creating thread");
	free_memory(threads_id, threads_args);
        return ERROR_CODE; 
    }

    *pi = 0;
   
    // join threads and sum partial sums
    return_code = join_threads_with_partial_sum(num_of_threads, threads_id, threads_args, pi);
    if (return_code != SUCCESS_CODE) {
        print_error(return_code, "join thread");
	free_memory(threads_id, threads_args);
        return ERROR_CODE; 
    }

    free_memory(threads_id, threads_args);

    *pi = *pi * 4.0;
    return SUCCESS_CODE;
}

int main(int argc, char **argv) {
    // read arg and check for validity
    int return_code = check_input(argc, argv[INDEX_FOR_NUM_OF_THREADS]);
    if (return_code != SUCCESS_CODE) {
	exit(EXIT_SUCCESS);
    }
    int num_of_threads = (int)strtol(argv[INDEX_FOR_NUM_OF_THREADS], NULL, BASE);
    
    double pi = 0;
    return_code = calculate_pi(num_of_threads, &pi);
    if (return_code != SUCCESS_CODE) {
	exit(EXIT_FAILURE);
    }

    printf("pi done - %.15g \n", pi);    
    
    exit(EXIT_SUCCESS);
}

