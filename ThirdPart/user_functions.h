#include <stdint.h>
#include <stdbool.h>

#define MAX_BUFFER_SIZE (1025)
#define DEFAULT_SEPERATOR (',')
#define SEPERATOR_SIZE (1)

typedef enum user_functions_result_e
{
    USER_FUNCTIONS_RESULT_SUCCESS = 0,
    USER_FUNCTIONS_RESULT_FAILURE,
    USER_FUNCTIONS_RESULT_EXIT,
    USER_FUNCTIONS_RESULT_NODE_FAILURE,
    USER_FUNCTIONS_RESULT_INVALID_ARGUMENTS,
} user_functions_result_t;

typedef enum utils_result_e {
    UTILS_RESULT_SUCCESS = 0,
    UTILS_RESULT_FAILURE,
    UTILS_RESULT_SEPERATOR_NOT_FOUND,
    UTILS_RESULT_ARGUMENT_TOO_LARGE,
} utils_result_t;

char *find_nth_occurance(const char *str, char ch, int n);
bool is_port_valid(int num);
utils_result_t string_to_port(const char *str, uint16_t *port);
utils_result_t get_string_param(const char *input, char *output, unsigned int output_size, int index, char seperator);
utils_result_t get_int_param(const char *input, int *out, int index, char seperator);
user_functions_result_t handle_user_function();