#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include "node.h"
#include "user_functions.h"
#include <sys/time.h>

#define NUM_ARGUMENTS (2)
#define PORT_ARGUMENT_INDEX (1)
#define NUM_REQUESTS (10)
#define ERROR (-1)
#define TRUE (1)
#define FALSE (0)
static fd_set rfds, rfds_copy;
static int max_fd = 0;
static int initialized = FALSE;
static int *alloced_fds = NULL;
static int alloced_fds_num = 0;

static int add_fd_to_monitoring_internal(const unsigned int fd) {
    int *tmp_alloc;
    tmp_alloc = realloc(alloced_fds, sizeof(int) * (alloced_fds_num + 1));
    if (tmp_alloc == NULL)
        return -1;
    alloced_fds = tmp_alloc;
    alloced_fds[alloced_fds_num++] = fd;
    FD_SET(fd, &rfds_copy);
    if (max_fd < fd)
        max_fd = fd;
    return 0;
}

int init() {
    FD_ZERO(&rfds_copy);
    if (add_fd_to_monitoring_internal(0) < 0)
        return -1; // monitoring standard input
    initialized = TRUE;
    return 0;
}

int add_fd_to_monitoring(const unsigned int fd) {
    if (!initialized) {
        if (init() == -1) {
            return -1;
        }
    }
    if (fd > 0) {
        return add_fd_to_monitoring_internal(fd); }
    return 0;
}

int wait_for_input() {
    int i, retval;
    memcpy(&rfds, &rfds_copy, sizeof(rfds_copy));
    retval = select(max_fd + 1, &rfds, NULL, NULL, NULL);
    if (retval > 0) {
        for (i = 0; i < alloced_fds_num; ++i) {
            if (FD_ISSET(alloced_fds[i], &rfds))
                return alloced_fds[i];
        }
    }
    return ERROR;
}

typedef enum program_result_e {
    PROGRAM_RESULT_SUCCESS = 0,
    PROGRAM_RESULT_FAILURE,
    PROGRAM_RESULT_ARGUMENT_ERROR,
    PROGRAM_RESULT_SOCKET_ERROR,
} program_result_t;

int main(int argc, char *argv[]) {
    program_result_t status = PROGRAM_RESULT_SUCCESS;
    uint16_t port = 0;
    int listenfd = 0, clientfd = 0, reuse = 1;
    struct sockaddr_in serv_addr = {0}, client_addr = {0};
    socklen_t client_len = 0;
    int ret = 0, i = 0;
    char buff[MAX_BUFFER_SIZE];
    user_functions_result_t user_functions_result = USER_FUNCTIONS_RESULT_SUCCESS;
    if (argc != NUM_ARGUMENTS) {
        status = PROGRAM_RESULT_ARGUMENT_ERROR;
        goto cleanup;
    }
    if (string_to_port(argv[PORT_ARGUMENT_INDEX], &port) != UTILS_RESULT_SUCCESS) {
        status = PROGRAM_RESULT_ARGUMENT_ERROR;
        goto cleanup;
    }
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("socket");
        status = PROGRAM_RESULT_SOCKET_ERROR;
        goto cleanup;
    }
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        status = PROGRAM_RESULT_SOCKET_ERROR;
        goto cleanup;
    }
    memset(&serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind");
        status = PROGRAM_RESULT_SOCKET_ERROR;
        goto cleanup;
    }
    add_fd_to_monitoring(listenfd);
    errno = 0;
    if (listen(listenfd, NUM_REQUESTS) == -1) {
        perror("listen");
        status = PROGRAM_RESULT_SOCKET_ERROR;
        goto cleanup;
    }
    while (user_functions_result != USER_FUNCTIONS_RESULT_EXIT) {
        ret = wait_for_input();
        if (ret == ERROR) {
            status = PROGRAM_RESULT_FAILURE;
            goto cleanup;
        }
        else if (ret == STDIN_FILENO) {
            user_functions_result = handle_user_function();
        }
        else {
            clientfd = accept(ret, (struct sockaddr *)&client_addr, &client_len);
            if (clientfd < 0) {
                status = PROGRAM_RESULT_SOCKET_ERROR;
                goto cleanup;
            }
            handle_node_function(clientfd);
            close(clientfd);
        }
        memset(buff, '\0', MAX_BUFFER_SIZE);
    }

cleanup:
    if (listenfd != 0) {
        if (close(listenfd) == -1) {
            perror("close");
        }
    }
    return status;
}