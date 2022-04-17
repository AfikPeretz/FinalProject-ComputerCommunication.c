#include <arpa/inet.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "user_functions.h"
#include "node.h"

#define FUNCTION_SETID "setid"
#define FUNCTION_CONNECT "connect"
#define FUNCTION_SEND "send"
#define FUNCTION_ROUTE "route"
#define FUNCTION_PEERS "peers"
#define FUNCTION_EXIT "exit"
#define STRING_BASE (10)
#define PORT_MAX_VALUE ((1 << 16) - 1)

#define CHECK(call)                                         \
    do                                                      \
    {                                                       \
        if ((call) != UTILS_RESULT_SUCCESS)                 \
        {                                                   \
            return USER_FUNCTIONS_RESULT_INVALID_ARGUMENTS; \
        }                                                   \
    } while (0)

char *find_nth_occurance(const char *str, char ch, int n) {
    char *search = str;
    int counter = 0;
    while (*search != '\0') {
        if (*search == ch) {
            if (counter == n) {
                return search;
            }
            else {
                counter++;
            }
        }
        search++;
    }
    if (ch == '\0' && counter == n) {
        return search;
    }
    return NULL;
}

bool is_port_valid(int num) {
    return num > 0 && num <= PORT_MAX_VALUE;
}

utils_result_t string_to_port(const char *str, uint16_t *port) {
    utils_result_t status = UTILS_RESULT_SUCCESS;
    unsigned long value = 0;
    char *endptr = NULL;
    errno = 0;
    value = strtoul(str, &endptr, STRING_BASE);
    if (endptr == str || (errno != 0 && value == 0) || !is_port_valid(value)) {
        status = UTILS_RESULT_FAILURE;
    }
    else {
        *port = (uint16_t)value;
    }
    return status;
}

utils_result_t get_string_param(const char *input, char *output, unsigned int output_size, int index, char seperator) {
    utils_result_t status = UTILS_RESULT_SUCCESS;
    char *seperator_pointer = NULL;
    char *param_start = input;
    unsigned long substring_size = 0;
    seperator_pointer = find_nth_occurance(input, seperator, index);
    if (index > 0) {
        param_start = find_nth_occurance(input, seperator, index - 1) + sizeof(seperator);
    }
    if (seperator_pointer != NULL && param_start != NULL) {
        substring_size = (unsigned long)(seperator_pointer - param_start);
        if (substring_size < output_size) {
            memmove(output, param_start, substring_size);
        }
        else {
            status = UTILS_RESULT_ARGUMENT_TOO_LARGE;
        }
    }
    else {
        status = UTILS_RESULT_SEPERATOR_NOT_FOUND;
    }
    return status;
}

utils_result_t get_int_param(const char *input, int *out, int index, char seperator) {
    utils_result_t status = UTILS_RESULT_SUCCESS;
    char temp_buffer[MAX_BUFFER_SIZE] = {0};
    char *endptr = NULL;
    long value = 0;
    status = get_string_param(input, temp_buffer, MAX_BUFFER_SIZE, index, seperator);
    if (status == UTILS_RESULT_SUCCESS) {
        errno = 0;
        value = strtol(temp_buffer, &endptr, STRING_BASE);
        if (endptr == temp_buffer || (errno != 0 && value == 0)) {
            status = UTILS_RESULT_FAILURE;
        }
        else {
            *out = (int)value;
        }
    }
    return status;
}

bool is_prefix(const char *str, const char *prefix_str) {
    return strncmp(prefix_str, str, strlen(prefix_str)) == 0;
}

user_functions_result_t handle_setid(const char buffer[MAX_BUFFER_SIZE]) {
    char *port_start = NULL;
    int id = 0;
    port_start = strchr(buffer, DEFAULT_SEPERATOR);
    if (port_start == NULL)
    {
        return USER_FUNCTIONS_RESULT_INVALID_ARGUMENTS;
    }
    CHECK(get_int_param(port_start + SEPERATOR_SIZE, &id, 0, '\0'));
    if (node_setid(id) != NODE_FUNCTION_RESULT_SUCCESS)
    {
        return USER_FUNCTIONS_RESULT_FAILURE;
    }
    return USER_FUNCTIONS_RESULT_SUCCESS;
}

user_functions_result_t handle_connect(const char buffer[MAX_BUFFER_SIZE]) {
    char *address_start = NULL;
    struct sockaddr_in socket_address = {0};
    char address[MAX_BUFFER_SIZE] = {0};
    char ip_string[MAX_BUFFER_SIZE] = {0};
    char port_string[MAX_BUFFER_SIZE] = {0};
    uint32_t ip = 0;
    uint16_t port = 0;
    address_start = strchr(buffer, DEFAULT_SEPERATOR);
    if (address_start == NULL) {
        return USER_FUNCTIONS_RESULT_INVALID_ARGUMENTS;
    }
    CHECK(get_string_param(address_start + SEPERATOR_SIZE, address, MAX_BUFFER_SIZE, 0, '\0'));
    CHECK(get_string_param(address, ip_string, MAX_BUFFER_SIZE, 0, ':'));
    CHECK(get_string_param(address + strlen(ip_string) + SEPERATOR_SIZE, port_string, MAX_BUFFER_SIZE, 0, '\0'));
    CHECK(string_to_port(port_string, &port));
    if (inet_pton(AF_INET, ip_string, &(socket_address.sin_addr)) <= 0) {
        return USER_FUNCTIONS_RESULT_INVALID_ARGUMENTS;
    }
    socket_address.sin_family = AF_INET;
    socket_address.sin_port = htons(port);
    if (node_connect(socket_address) != NODE_FUNCTION_RESULT_SUCCESS)
    {
        return USER_FUNCTIONS_RESULT_FAILURE;
    }
    return USER_FUNCTIONS_RESULT_SUCCESS;
}

user_functions_result_t handle_send(const char buffer[MAX_BUFFER_SIZE])
{
    char *command_start = NULL, *message_start = NULL;
    uint32_t id = 0;
    uint32_t len = 0;
    command_start = strchr(buffer, DEFAULT_SEPERATOR);
    if (command_start == NULL)
    {
        return USER_FUNCTIONS_RESULT_INVALID_ARGUMENTS;
    }
    CHECK(get_int_param(command_start + SEPERATOR_SIZE, &id, 0, DEFAULT_SEPERATOR));
    CHECK(get_int_param(command_start + SEPERATOR_SIZE, &len, 1, DEFAULT_SEPERATOR));
    message_start = find_nth_occurance(buffer, DEFAULT_SEPERATOR, 2);
    if (message_start == NULL)
    {
        return USER_FUNCTIONS_RESULT_INVALID_ARGUMENTS;
    }
    if (strlen(message_start + SEPERATOR_SIZE) != len)
    {
        return USER_FUNCTIONS_RESULT_INVALID_ARGUMENTS;
    }
    if (node_send(id, len, message_start + SEPERATOR_SIZE) != NODE_FUNCTION_RESULT_SUCCESS)
    {
        return USER_FUNCTIONS_RESULT_FAILURE;
    }
    return USER_FUNCTIONS_RESULT_SUCCESS;
}

user_functions_result_t handle_route(const char buffer[MAX_BUFFER_SIZE]) {
    char *command_start = NULL;
    uint32_t id = 0;
    command_start = strchr(buffer, DEFAULT_SEPERATOR);
    if (command_start == NULL)
    {
        return USER_FUNCTIONS_RESULT_INVALID_ARGUMENTS;
    }
    CHECK(get_int_param(command_start + SEPERATOR_SIZE, &id, 0, '\0'));
    if (node_route(id) != NODE_FUNCTION_RESULT_SUCCESS)
    {
        return USER_FUNCTIONS_RESULT_FAILURE;
    }
    return USER_FUNCTIONS_RESULT_SUCCESS;
}

user_functions_result_t handle_peers(const char buffer[MAX_BUFFER_SIZE])
{
    if (node_peers() != NODE_FUNCTION_RESULT_SUCCESS) {
        return USER_FUNCTIONS_RESULT_FAILURE;
    }
    return USER_FUNCTIONS_RESULT_SUCCESS;
}

user_functions_result_t handle_user_function()
{
    user_functions_result_t status = USER_FUNCTIONS_RESULT_SUCCESS;
    char buffer[MAX_BUFFER_SIZE] = {'\0'};
    char *result = 0;
    result = fgets(buffer, MAX_BUFFER_SIZE, stdin);
    if (result != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
        if (is_prefix(buffer, FUNCTION_SETID)) {
            status = handle_setid(buffer);
        }
        else if (is_prefix(buffer, FUNCTION_CONNECT)) {
            status = handle_connect(buffer);
        }
        else if (is_prefix(buffer, FUNCTION_SEND)) {
            status = handle_send(buffer);
        }
        else if (is_prefix(buffer, FUNCTION_ROUTE)) {
            status = handle_route(buffer);
        }
        else if (is_prefix(buffer, FUNCTION_PEERS)) {
            status = handle_peers(buffer);
        }
        else if (is_prefix(buffer, FUNCTION_EXIT)) {
            destroy();
            status = USER_FUNCTIONS_RESULT_EXIT;
        }
        if (status != USER_FUNCTIONS_RESULT_SUCCESS && status != USER_FUNCTIONS_RESULT_EXIT) {
            printf(NACK_PRINT);
        }
    }
    return status;
}