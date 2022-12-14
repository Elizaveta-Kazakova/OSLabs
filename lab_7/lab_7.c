#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <libgen.h>
#include <stddef.h>

#define NUM_OF_ARGS 3
#define ERROR_CODE 1
#define SUCCESS_CODE 0
#define INDEX_FOR_PATHNAME_OF_THE_SOURCE_TREE 0
#define NULL_TERMINATOR_SIZE 1
#define END_OF_LINE_SIZE 1
#define PATH_DELIMITER "/"
#define PATH_DELIMITER_SIZE 1
#define MODE 0777
#define LINK_TO_THIS_DIR "."
#define DIR_LINK_ABOVE ".."
#define EQUAL 0
#define BUF_SIZE 1024
#define INDEX_FOR_SRC_PATH 1
#define INDEX_FOR_DEST_PATH 2
#define THREADS_BUF_SIZE 256
#define REMAINDER_FOR_NEW_ALLOCATION 1
#define MESSAGE_FOR_INVALID_ARGS_NUM "Please write two arguments: src and dest path\n"
#define NUM_OF_SECONDS_TO_WAIT_FILE 1

typedef struct Paths {
    char *src;
    char *dest;
} Paths;

int create_paths(Paths **paths, char *src, char *dest);
int add_name_in_path(char *old_path, char *new_name_in_path, char **result);
void free_paths(Paths *paths);
int free_dir_resources(Paths *paths, DIR *dir_stream, struct dirent *dir_info);
int create_new_paths_from_old(Paths *old_path, char *new_name_in_path, Paths **new_path);
void *copy_regular_file(void *args);
void *copy_directory(void *args);
int create_thread_by_file_stat(struct stat file_stat, Paths *next_path);
void print_error(int return_code, char *additional_message);
int cp_r(char **argv);

int create_paths(Paths **paths, char *src, char *dest) {
    size_t len_source_path = strlen(src) + NULL_TERMINATOR_SIZE;
    size_t len_destination_path = strlen(dest) + NULL_TERMINATOR_SIZE;
    *paths = malloc(sizeof(Paths));
    if (paths == NULL) {
        return ERROR_CODE;
    }

    (*paths)->src = malloc(len_source_path * sizeof(char));
    if ((*paths)->src == NULL) {
	free(*paths);
	return ERROR_CODE;
    }
    (*paths)->dest = malloc(len_destination_path * sizeof(char));
    if ((*paths)->dest == NULL) {
	free(*paths);
	free((*paths)->src);
        return ERROR_CODE;
    }
    strncpy((*paths)->src, src, len_source_path);
    strncpy((*paths)->dest, dest, len_destination_path);
    return SUCCESS_CODE;
}

int add_name_in_path(char *old_path, char *new_name_in_path, char **result) {
    size_t len_new_path = strlen(old_path) + strlen(new_name_in_path) 
        + NULL_TERMINATOR_SIZE + PATH_DELIMITER_SIZE;
    *result = malloc(len_new_path * sizeof(char));
    if (*result == NULL) {
	return ERROR_CODE;
    }
    strcpy(*result, old_path);
    strcat(*result, PATH_DELIMITER);
    strcat(*result, new_name_in_path);
    return SUCCESS_CODE;
}

void free_paths(Paths *paths) {
    free(paths->src);
    free(paths->dest);
    paths->src = NULL;
    paths->dest = NULL;
    free(paths);
    paths = NULL;
}

int free_dir_resources(Paths *paths, DIR *dir_stream, struct dirent *dir_info) {
    free_paths(paths);
    free(dir_info);
    int return_code = closedir(dir_stream);
    if (return_code != SUCCESS_CODE) {
	perror("closedir");
	return return_code;
    }

    return SUCCESS_CODE;
}

int free_file_resourcces(Paths *paths, int src_fd, int dest_fd) {
    int close_src_code = close(src_fd); 
    int close_dest_code = close(dest_fd);
    free_paths(paths);
    if (close_src_code != SUCCESS_CODE || close_dest_code != SUCCESS_CODE) {
	perror("close file");
	return ERROR_CODE;
    }
    return SUCCESS_CODE;
}

int create_new_paths_from_old(Paths *old_path, char *new_name_in_path, Paths **new_path) {
    char *new_src_path;
    int return_code = add_name_in_path(old_path->src, new_name_in_path, &new_src_path);
    if (return_code != SUCCESS_CODE) {
	perror("add_name_in_path");
        return return_code;
    }

    char *new_dest_path;
    return_code = add_name_in_path(old_path->dest, new_name_in_path, &new_dest_path);
    if (return_code != SUCCESS_CODE) {
	perror("add_name_in_path");
	free(new_src_path);
        return return_code;
    }

    (*new_path) = malloc(sizeof(Paths));
    if (*new_path == NULL) {
	perror("malloc");
	free(new_src_path);
	free(new_dest_path);
        return ERROR_CODE;
    }

    (*new_path)->src = new_src_path;
    (*new_path)->dest = new_dest_path;
    return SUCCESS_CODE;
}

int wait_to_open_file(char *filename, int flags, mode_t mode, int *fd) {
    errno = SUCCESS_CODE;
    int file_desc = open(filename, flags, mode);

    while (file_desc == ERROR_CODE && errno == EMFILE) {
	sleep(NUM_OF_SECONDS_TO_WAIT_FILE);
        file_desc = open(filename, flags, mode);
    }
    if (file_desc == ERROR_CODE) {
	return ERROR_CODE;
    }

    *fd = file_desc;
    return SUCCESS_CODE;
}

int copy_bytes_from_fd(Paths *paths, int src_fd, int dest_fd) {
    char buf[BUFSIZ];

    ssize_t bytes_read = read(src_fd, buf, BUFSIZ);
    while (bytes_read > 0) {
        ssize_t bytes_write = write(dest_fd, buf, bytes_read);
        if (bytes_write == ERROR_CODE) {
            perror("write");
            return ERROR_CODE;
        }
        bytes_read = read(src_fd, buf, BUFSIZ);
    }
    if (bytes_read == ERROR_CODE) {
        perror("read");
        return ERROR_CODE;
    }
    return SUCCESS_CODE;
}

void *copy_regular_file(void *args) {
    Paths *paths = (Paths *)args;

    int src_fd;
    int return_code = wait_to_open_file(paths->src, O_RDONLY, MODE, &src_fd);
    if (return_code != SUCCESS_CODE) {
	perror("open src");
	free_paths(paths);
	return NULL;
    }

    struct stat file_stat;
    return_code = stat(paths->src, &file_stat);
    if (return_code != SUCCESS_CODE) {
        perror("stat");
        free_paths(paths);
	close(src_fd);
        return NULL;
    }
    int dest_fd;
    return_code = wait_to_open_file(paths->dest, O_WRONLY | O_CREAT | O_EXCL, file_stat.st_mode, &dest_fd);
    if (return_code == ERROR_CODE) {
	perror("open dest");
        free_paths(paths);
	close(src_fd);
        return NULL;
    }

    copy_bytes_from_fd(paths, src_fd, dest_fd);
    free_file_resourcces(paths, src_fd, dest_fd);
    return NULL;
}

int create_thread_for_file(Paths *paths, struct dirent *next_directory_info) {
    Paths *next_path;
    int return_code = create_new_paths_from_old(paths, next_directory_info->d_name, &next_path);
    if (return_code != SUCCESS_CODE) {
        return return_code;
    }
    struct stat file_stat;
    return_code = stat(next_path->src, &file_stat);
    if (return_code != SUCCESS_CODE) {
        perror("stat");
        free_paths(next_path);
        return return_code;
    }
    return_code = create_thread_by_file_stat(file_stat, next_path);
    if (return_code != SUCCESS_CODE) {
        free_paths(next_path);
        return return_code;
    }
    return SUCCESS_CODE;
}

int wait_for_open_dir(DIR **dir_stream, char *path) {
    errno = SUCCESS_CODE;
    *dir_stream = opendir(path);

    while (dir_stream == NULL && errno == EMFILE) {
	sleep(NUM_OF_SECONDS_TO_WAIT_FILE);
	*dir_stream = opendir(path);
    }
    if (dir_stream == NULL) {
	return ERROR_CODE;
    }

    return SUCCESS_CODE;
}

int create_directory_struct(char *path, struct dirent **next_directory_info) {
    long name_length = pathconf(path, _PC_NAME_MAX);
    if (name_length == ERROR_CODE) {
	perror("pathconf");
	return ERROR_CODE;
    }

    *next_directory_info = malloc(offsetof(struct dirent, d_name) + name_length + END_OF_LINE_SIZE);
    if (*next_directory_info == NULL) {
	perror("malloc dirent");
	return ERROR_CODE;
    }

    return SUCCESS_CODE;
}

int create_dir_resources(DIR **dir_stream, struct dirent **next_directory_info, char *path) {
    int return_code = wait_for_open_dir(dir_stream, path);
    if (return_code != SUCCESS_CODE) {
        perror("opendir");
        return return_code;
    }

    return_code = create_directory_struct(path, next_directory_info);
    if (return_code != SUCCESS_CODE) {
	return ERROR_CODE;
    }
    return SUCCESS_CODE;
}

void *copy_directory(void *args) {
    Paths *paths = (Paths *)args;

    DIR *dir_stream;
    struct dirent *next_directory_info;
    int return_code = create_dir_resources(&dir_stream, &next_directory_info, paths->src);
    if (return_code != SUCCESS_CODE) {
	free_paths(paths);
	return NULL;
    }

    struct dirent *result;
    return_code = readdir_r(dir_stream, next_directory_info, &result);
    if (return_code != SUCCESS_CODE) {
	perror("readdir");
	free_dir_resources(paths, dir_stream, next_directory_info);
        return NULL;
    }
    while (result != NULL) {
 	if (strcmp(next_directory_info->d_name, LINK_TO_THIS_DIR) == EQUAL || strcmp(next_directory_info->d_name, DIR_LINK_ABOVE) == EQUAL) {
	    errno = SUCCESS_CODE;
            return_code = readdir_r(dir_stream, next_directory_info, &result);
            if (return_code != SUCCESS_CODE) {
		perror("readdir");
	    	free_dir_resources(paths, dir_stream, next_directory_info);
                return NULL;
            }
            continue;
        }

	return_code = create_thread_for_file(paths, next_directory_info);
	if (return_code != SUCCESS_CODE) {
	    free_dir_resources(paths, dir_stream, next_directory_info);
	    return NULL;
	}
	return_code = readdir_r(dir_stream, next_directory_info, &result);
	if (return_code != SUCCESS_CODE) {
	    perror("readdir");
	    free_dir_resources(paths, dir_stream, next_directory_info);
            return NULL;
        }
    }
    free_dir_resources(paths, dir_stream, next_directory_info);
    return NULL;
}

int create_thread_by_file_stat(struct stat file_stat, Paths *next_path) {
    pthread_t new_thread_id;
    int return_code;
    if (!S_ISREG(file_stat.st_mode) && !S_ISDIR(file_stat.st_mode)) {
	return SUCCESS_CODE;
    }

    if (S_ISREG(file_stat.st_mode)) {
        return_code = pthread_create(&new_thread_id, NULL, copy_regular_file, (void *)next_path); 
    }
    if (S_ISDIR(file_stat.st_mode)) { 
	return_code = mkdir(next_path->dest, file_stat.st_mode);
        if (return_code != SUCCESS_CODE) {
            perror("mkdir");
            return ERROR_CODE;
        }
        return_code = pthread_create(&new_thread_id, NULL, copy_directory, (void *)next_path);
    }

    if (return_code != SUCCESS_CODE) {
        print_error(return_code, "pthread_create");
        return return_code;
    }

    return_code = pthread_detach(new_thread_id);
    if (return_code != SUCCESS_CODE) {
	print_error(return_code, "pthread detach");
	return return_code;
    }

    return SUCCESS_CODE;
}

int get_file_stats(Paths *paths, struct stat *src_stat, struct stat *dest_stat) {
    int return_code = stat(paths->src, src_stat);
    if (return_code != SUCCESS_CODE) {
        perror("src stat");
        return return_code;
    }
    return_code = stat(paths->dest, dest_stat);
    if (return_code != SUCCESS_CODE) {
        perror("dst stat");
        return return_code;
    }
    return SUCCESS_CODE;
}

int handle_input_paths(Paths **paths) {
    struct stat src_stat;
    struct stat dest_stat;
    int return_code = get_file_stats(*paths, &src_stat, &dest_stat);
    if (return_code != SUCCESS_CODE) {
	return return_code;
    }

    if (S_ISDIR(src_stat.st_mode) && S_ISREG(dest_stat.st_mode)) {
	printf("cannot overwrite non-directory %s with directory %s\n", (*paths)->dest, (*paths)->src);
	return ERROR_CODE;
    }

    if (S_ISREG(src_stat.st_mode) && S_ISDIR(dest_stat.st_mode) || S_ISDIR(src_stat.st_mode) && S_ISDIR(dest_stat.st_mode)) {
	char *new_dest_path;
	return_code = add_name_in_path((*paths)->dest, basename((*paths)->src), &new_dest_path);
        if (return_code != SUCCESS_CODE) {
            return return_code;
        }
	free((*paths)->dest);
	(*paths)->dest = new_dest_path;
    }

    return SUCCESS_CODE; 
}

int cp_r(char **argv) {
    Paths *paths;
    int return_code = create_paths(&paths, argv[INDEX_FOR_SRC_PATH], argv[INDEX_FOR_DEST_PATH]);
    if (return_code != SUCCESS_CODE) {
	perror("create paths");
	return return_code;
    }
    return_code = handle_input_paths(&paths);
    if (return_code != SUCCESS_CODE) {
	free_paths(paths);
	return return_code;
    }
    struct stat file_stat;
    return_code = stat(paths->src, &file_stat);
    if (return_code != SUCCESS_CODE) {
        perror("stat");
        free_paths(paths);
        return return_code;
    }
    return_code = create_thread_by_file_stat(file_stat, paths);
    if (return_code != SUCCESS_CODE) {
	free_paths(paths);
        return return_code;
    }
    return SUCCESS_CODE;
}

void print_error(int return_code, char *additional_message) {
    char buf[BUF_SIZE];
    strerror_r(return_code, buf, sizeof buf);
    fprintf(stderr, "%s: %s\n", additional_message, buf);
}

int main(int argc, char **argv) {
    int return_code;
    if (argc != NUM_OF_ARGS) {
	printf(MESSAGE_FOR_INVALID_ARGS_NUM);
	pthread_exit(NULL);
    }

    return_code = cp_r(argv);
    if (return_code != SUCCESS_CODE) {
	pthread_exit(NULL);
    }
    pthread_exit(NULL);
}

