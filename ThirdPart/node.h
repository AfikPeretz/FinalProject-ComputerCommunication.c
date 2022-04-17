#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>

#define ACK_PRINT "ack\n"
#define NACK_PRINT "nack\n"

typedef enum node_result_e {
    NODE_RESULT_ACK = 0,
    NODE_RESULT_NACK,
} node_result_t;

typedef enum node_functions_e {
    FUNCTION_ACK = 1,
    FUNCTION_NACK = 1 << 1,
    FUNCTION_CONNECT = 1 << 2,
    FUNCTION_DISCOVER = 1 << 3,
    FUNCTION_ROUTE = 1 << 4,
    FUNCTION_SEND = 1 << 5,
    FUNCTION_RELAY = 1 << 6,
} node_functions_t;

typedef enum node_function_result_e {
    NODE_FUNCTION_RESULT_SUCCESS = 0,
    NODE_FUNCTION_RESULT_FAILURE,
    NODE_FUNCTION_RESULT_NODE_NOT_SET,
    NODE_FUNCTION_RESULT_INVALID_SOCKET,
    NODE_FUNCTION_RESULT_FAILED_CONNECT,
    NODE_FUNCTION_RESULT_FAILED_SEND,
    NODE_FUNCTION_RESULT_FAILED_RECV,
    NODE_FUNCTION_RESULT_FAILED_ALLOC,
    NODE_FUNCTION_RESULT_INVALID_RESPONSE,
    NODE_FUNCTION_RESULT_INVALID_MESSAGE,
    NODE_FUNCTION_RESULT_INVALID_DESTINATION,
    NODE_FUNCTION_RESULT_INVALID_FUNCTION_ID,
    NODE_FUNCTION_RESULT_FAIL_MAX_ROUTES,
    NODE_FUNCTION_RESULT_NO_ROUTE_FOUND,
} node_function_result_t;

node_function_result_t node_setid(uint32_t id);
node_function_result_t node_connect(const struct sockaddr_in sa);
node_function_result_t node_send(uint32_t id, uint32_t len, char *message);
node_function_result_t node_discover(uint32_t id);
node_function_result_t node_route(uint32_t id);
node_function_result_t node_peers();
node_function_result_t handle_node_function(int sock_fd);
node_function_result_t destroy();