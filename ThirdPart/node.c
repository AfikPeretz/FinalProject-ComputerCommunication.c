#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "node.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>

typedef struct connected_node_s {
    struct sockaddr_in addr;
    uint32_t id;
} connected_node_t;

#define PAYLOAD_SIZE (492)

typedef struct message_s {
    uint32_t message_id;
    uint32_t source_id;
    uint32_t destination_id;
    uint32_t trailing_message;
    uint32_t function_id;
    char payload[PAYLOAD_SIZE];
} message_t;

#define CHECK_NODE_SET                                \
    do {                                              \
        if (node_set == false)                        \
        {                                             \
            return NODE_FUNCTION_RESULT_NODE_NOT_SET; \
        }                                             \
    } while (0)

#define INVALID_SOCKET (-1)
#define INVALID_NODE_ID (0)
#define MAX_ROUTES (1000)
#define MAX_ROUTE_SIZE (PAYLOAD_SIZE)
static connected_node_t *connected_nodes = NULL;
static uint32_t routes[MAX_ROUTES][MAX_ROUTE_SIZE] = {};
static int num_connections = 0;
static int num_routes = 0;
static int relaying = 0;
static uint32_t relaying_id = 0;
static bool node_set = false;
static int node_id = INVALID_NODE_ID;
static int current_message_id = 0;

int get_route_size(uint32_t route[MAX_ROUTE_SIZE]) {
    int i = 0;
    for (i = 0; i < MAX_ROUTE_SIZE; ++i) {
        if (route[i] == INVALID_NODE_ID) {
            break;
        }
    }
    return i;
}

void print_route(uint32_t route[MAX_ROUTE_SIZE]) {
    int size = 0, i = 0;
    size = get_route_size(route);
    printf("%d->", node_id);
    for (i = size - 1; i > 0; i--) {
        printf("%d->", route[i]);
    }
    printf("%d\n", route[i]);
}

node_function_result_t add_route(uint32_t route[MAX_ROUTE_SIZE]) {
    if (num_routes < MAX_ROUTES) {
        memmove(routes[num_routes], route, MAX_ROUTE_SIZE);
        num_routes++;
        return NODE_FUNCTION_RESULT_SUCCESS;
    }
    else {
        return NODE_FUNCTION_RESULT_FAIL_MAX_ROUTES;
    }
}

int lexicographically_smallest_route(uint32_t route1[MAX_ROUTE_SIZE], uint32_t route2[MAX_ROUTE_SIZE]) {
    int i = 0;
    if (get_route_size(route1) < get_route_size(route2)) {
        return 1;
    }
    for (i = 0; i < MAX_ROUTE_SIZE; ++i) {
        if (route1[i] > route2[i]) {
            return -1;
        }
        if (route1[i] < route2[i]) {
            return 1;
        }
    }
    return 0;
}

uint32_t *find_route(uint32_t id) {
    int i = 0;
    for (i = 0; i < MAX_ROUTES; ++i) {
        if (routes[i][0] == id) {
            return routes[i];
        }
    }
    return NULL;
}

node_function_result_t add_connected_node(struct sockaddr_in addr, uint32_t id) {
    connected_node_t *temp = NULL;
    connected_node_t new_node = {0};
    if (connected_nodes == NULL) {
        connected_nodes = malloc(sizeof(*connected_nodes));
        num_connections = 1;
        if (connected_nodes == NULL) {
            return NODE_FUNCTION_RESULT_FAILED_ALLOC;
        }
    }
    else {
        num_connections++;
        temp = realloc(connected_nodes, sizeof(connected_node_t) * num_connections);
        if (temp == NULL) {
            return NODE_FUNCTION_RESULT_FAILED_ALLOC;
        }
        connected_nodes = temp;
    }
    new_node = (connected_node_t) {
        .id = id,
        .addr = addr,
    };
    memmove(connected_nodes + (num_connections - 1), &new_node, sizeof(new_node));
    return NODE_FUNCTION_RESULT_SUCCESS;
}

connected_node_t *get_connected_node(uint32_t id) {
    int i = 0;
    connected_node_t *temp = connected_nodes;
    for (i = 0; i < num_connections; ++i) {
        if (temp->id == id) {
            return temp;
        }
        temp++;
    }
    return NULL;
}

node_function_result_t send_discover_to_connected_nodes(uint32_t id, uint32_t exclude_id) {
    node_function_result_t status = NODE_FUNCTION_RESULT_SUCCESS;
    message_t discover_message = {0}, response = {0};
    connected_node_t *connected = connected_nodes;
    uint32_t route[MAX_ROUTE_SIZE] = {0}, *temp_route = NULL;
    int sock = INVALID_SOCKET;
    int i = 0;
    discover_message = (message_t){
        .message_id = current_message_id++,
        .source_id = node_id,
        .destination_id = 0,
        .trailing_message = 0,
        .function_id = FUNCTION_DISCOVER,
    };
    memmove(discover_message.payload, &id, sizeof(id));
    for (i = 0; i < num_connections; ++i) {
        if (connected->id == exclude_id) {
            connected++;
            continue;
        }
        discover_message.destination_id = connected->id;
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            status = NODE_FUNCTION_RESULT_INVALID_SOCKET;
            goto cleanup;
        }
        if (connect(sock, (struct sockaddr *)&connected->addr, sizeof(connected->addr)) < 0) {
            status = NODE_FUNCTION_RESULT_FAILED_CONNECT;
            goto cleanup;
        }
        if (send(sock, &discover_message, sizeof(discover_message), 0) < 0) {
            status = NODE_FUNCTION_RESULT_FAILED_SEND;
            goto cleanup;
        }
        if (recv(sock, &response, sizeof(response), 0) < 0) {
            status = NODE_FUNCTION_RESULT_FAILED_RECV;
            goto cleanup;
        }
        if (response.function_id == FUNCTION_ACK) {
            route[0] = id;
            route[1] = connected->id;
            add_route(route);
            status = NODE_FUNCTION_RESULT_SUCCESS;
        }
        else if (response.function_id == FUNCTION_ROUTE) {
            temp_route = find_route(id);
            if (temp_route == NULL) {
                add_route(response.payload);
            }
            else {
                if (lexicographically_smallest_route(temp_route, response.payload) < 0) {
                    memmove(temp_route, response.payload, MAX_ROUTE_SIZE);
                }
            }
            status = NODE_FUNCTION_RESULT_SUCCESS;
        }
        close(sock);
        sock = INVALID_SOCKET;
        connected++;
    }

cleanup:
    if (sock != INVALID_SOCKET)
    {
        close(sock);
    }

    return status;
}

uint32_t *discover_route(uint32_t id)
{
    uint32_t *route = NULL;

    route = find_route(id);
    if (route != NULL)
    {
        return route;
    }
    else
    {
        send_discover_to_connected_nodes(id, INVALID_NODE_ID);
        route = find_route(id);
        if (route != NULL)
        {
            return route;
        }
    }
    return NULL;
}

node_function_result_t handle_node_connect(message_t message, int client_fd) {
    CHECK_NODE_SET;
    message_t response = {0};
    response = (message_t){
        .message_id = current_message_id++,
        .source_id = node_id,
        .destination_id = message.source_id,
        .trailing_message = 0,
        .function_id = FUNCTION_ACK,
    };
    memset(response.payload, '\0', PAYLOAD_SIZE);
    memmove(response.payload, &message.message_id, sizeof(message.message_id));
    if (send(client_fd, &response, sizeof(message), 0) != sizeof(message)) {
        return NODE_FUNCTION_RESULT_FAILED_SEND;
    }
    return NODE_FUNCTION_RESULT_SUCCESS;
}

node_function_result_t handle_node_send(message_t message, int client_fd) {
    CHECK_NODE_SET;
    message_t response = {0};
    uint32_t len = 0;
    char *payload_message = NULL;
    len = *((uint32_t *)message.payload);
    payload_message = message.payload + sizeof(len);
    printf("%.*s\n", len, payload_message);
    response = (message_t){
        .message_id = current_message_id++,
        .source_id = node_id,
        .destination_id = message.source_id,
        .trailing_message = 0,
        .function_id = FUNCTION_ACK,
    };
    memset(response.payload, '\0', PAYLOAD_SIZE);
    if (send(client_fd, &response, sizeof(message), 0) != sizeof(message)) {
        return NODE_FUNCTION_RESULT_FAILED_SEND;
    }
    return NODE_FUNCTION_RESULT_SUCCESS;
}

node_function_result_t handle_node_discover(message_t message, int client_fd) {
    CHECK_NODE_SET;
    message_t response = {0};
    uint32_t id = 0, *route = NULL;
    int route_size = 0;
    char *payload_message = NULL;
    id = *((uint32_t *)message.payload);
    response = (message_t){
        .message_id = current_message_id++,
        .source_id = node_id,
        .destination_id = message.source_id,
        .trailing_message = 0,
        .function_id = FUNCTION_ACK,
    };
    memset(response.payload, '\0', PAYLOAD_SIZE);
    if (get_connected_node(id) != NULL) {
        response.function_id = FUNCTION_ACK;
    }
    else {
        send_discover_to_connected_nodes(id, message.source_id);
        route = find_route(id);
        if (route != NULL) {
            response.function_id = FUNCTION_ROUTE;
            memmove(response.payload, route, MAX_ROUTE_SIZE);
            memmove(response.payload + (get_route_size(route) * sizeof(*route)), &node_id, sizeof(node_id));
        }
        else {
            response.function_id = FUNCTION_NACK;
        }
    }
    if (send(client_fd, &response, sizeof(message), 0) != sizeof(message)) {
        return NODE_FUNCTION_RESULT_FAILED_SEND;
    }
    return NODE_FUNCTION_RESULT_SUCCESS;
}

node_function_result_t handle_node_relay(message_t message, int client_fd) {
    CHECK_NODE_SET;
    connected_node_t *relay_to = NULL;
    message_t response = {0};
    relaying_id = *((uint32_t *)message.payload);
    relaying = *(((uint32_t *)message.payload) + 1);
    response = (message_t){
        .message_id = current_message_id++,
        .source_id = node_id,
        .destination_id = message.source_id,
        .trailing_message = 0,
        .function_id = FUNCTION_ACK,
    };
    memset(response.payload, '\0', PAYLOAD_SIZE);
    if (send(client_fd, &response, sizeof(message), 0) != sizeof(message)) {
        return NODE_FUNCTION_RESULT_FAILED_SEND;
    }
    return NODE_FUNCTION_RESULT_SUCCESS;
}

node_function_result_t node_setid(uint32_t id) {
    node_id = id;
    node_set = true;
    printf(ACK_PRINT);
    return NODE_FUNCTION_RESULT_SUCCESS;
}

node_function_result_t node_connect(struct sockaddr_in sa) {
    CHECK_NODE_SET;
    node_function_result_t status = NODE_FUNCTION_RESULT_SUCCESS;
    message_t connect_message = {0}, response = {0};
    int sock = INVALID_SOCKET;
    uint32_t response_id = 0;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        status = NODE_FUNCTION_RESULT_INVALID_SOCKET;
        goto cleanup;
    }
    if (connect(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        status = NODE_FUNCTION_RESULT_FAILED_CONNECT;
        goto cleanup;
    }
    connect_message = (message_t){
        .message_id = current_message_id++,
        .source_id = node_id,
        .destination_id = 0,
        .trailing_message = 0,
        .function_id = FUNCTION_CONNECT,
    };
    memset(connect_message.payload, '\0', PAYLOAD_SIZE);
    if (send(sock, &connect_message, sizeof(connect_message), 0) < 0) {
        status = NODE_FUNCTION_RESULT_FAILED_SEND;
        goto cleanup;
    }
    if (recv(sock, &response, sizeof(response), 0) < 0) {
        status = NODE_FUNCTION_RESULT_FAILED_RECV;
        goto cleanup;
    }
    if (response.function_id != FUNCTION_ACK) {
        status = NODE_FUNCTION_RESULT_FAILURE;
        goto cleanup;
    }
    response_id = *((uint32_t *)response.payload);
    if (response_id != current_message_id - 1) {
        status = NODE_FUNCTION_RESULT_INVALID_RESPONSE;
        goto cleanup;
    }
    add_connected_node(sa, response.source_id);
    printf(ACK_PRINT);
    printf("%d\n", response.source_id);

cleanup:
    if (sock != INVALID_SOCKET) {
        close(sock);
    }
    return status;
}

node_function_result_t node_send(uint32_t id, uint32_t len, char *message) {
    CHECK_NODE_SET;
    node_function_result_t status = NODE_FUNCTION_RESULT_SUCCESS;
    message_t send_message = {0}, relay_message = {0}, response = {0};
    connected_node_t *connected = NULL;
    int sock = INVALID_SOCKET;
    uint route_size = 0, i = 0, trailing = 0;
    uint32_t next_id = 0, response_id = 0, next_node = INVALID_NODE_ID, *route = NULL;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        status = NODE_FUNCTION_RESULT_INVALID_SOCKET;
        goto cleanup;
    }
    send_message = (message_t){
        .message_id = current_message_id++,
        .source_id = node_id,
        .destination_id = id,
        .trailing_message = 0,
        .function_id = FUNCTION_SEND,
    };
    memset(send_message.payload, '\0', PAYLOAD_SIZE);
    memmove(send_message.payload, &len, sizeof(len));
    memmove(send_message.payload + sizeof(len), message, len);
    connected = get_connected_node(id);
    if (connected != NULL) {
        if (connect(sock, (struct sockaddr *)&connected->addr, sizeof(connected->addr)) < 0) {
            status = NODE_FUNCTION_RESULT_FAILED_CONNECT;
            goto cleanup;
        }
    }
    else {
        route = discover_route(id);
        if (route == NULL) {
            status = NODE_FUNCTION_RESULT_NO_ROUTE_FOUND;
            goto cleanup;
        }
        route_size = get_route_size(route);
        next_node = route[route_size - 1];
        connected = get_connected_node(next_node);
        if (connected == NULL) {
            status = NODE_FUNCTION_RESULT_FAILURE;
            goto cleanup;
        }
        for (trailing = route_size - 1; trailing > 0; trailing--) {
            next_id = route[route_size - 2];
            relay_message = (message_t){
                .message_id = current_message_id++,
                .source_id = node_id,
                .destination_id = next_node,
                .trailing_message = trailing,
                .function_id = FUNCTION_RELAY,
            };
            memset(relay_message.payload, '\0', PAYLOAD_SIZE);
            memmove(relay_message.payload, &next_id, sizeof(next_id));
            memmove(relay_message.payload + sizeof(id), &trailing, sizeof(trailing));
            if (connect(sock, (struct sockaddr *)&connected->addr, sizeof(connected->addr)) < 0) {
                status = NODE_FUNCTION_RESULT_FAILED_CONNECT;
                goto cleanup;
            }
            if (send(sock, &relay_message, sizeof(relay_message), 0) < 0) {
                status = NODE_FUNCTION_RESULT_FAILED_SEND;
                goto cleanup;
            }
            if (recv(sock, &response, sizeof(response), 0) < 0) {
                status = NODE_FUNCTION_RESULT_FAILED_RECV;
                goto cleanup;
            }
            if (response.function_id != FUNCTION_ACK) {
                status = NODE_FUNCTION_RESULT_FAILURE;
                goto cleanup;
            }
            close(sock);
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock == INVALID_SOCKET) {
                status = NODE_FUNCTION_RESULT_INVALID_SOCKET;
                goto cleanup;
            }
        }
        if (connect(sock, (struct sockaddr *)&connected->addr, sizeof(connected->addr)) < 0) {
            status = NODE_FUNCTION_RESULT_FAILED_CONNECT;
            goto cleanup;
        }
    }
    if (send(sock, &send_message, sizeof(send_message), 0) < 0) {
        status = NODE_FUNCTION_RESULT_FAILED_SEND;
        goto cleanup;
    }
    if (recv(sock, &response, sizeof(response), 0) < 0) {
        status = NODE_FUNCTION_RESULT_FAILED_RECV;
        goto cleanup;
    }
    if (response.function_id != FUNCTION_ACK) {
        status = NODE_FUNCTION_RESULT_FAILURE;
        goto cleanup;
    }
    printf(ACK_PRINT);

cleanup:
    if (sock != INVALID_SOCKET) {
        close(sock);
    }
    return status;
}

node_function_result_t node_discover(uint32_t id) {
    CHECK_NODE_SET;
    node_function_result_t status = NODE_FUNCTION_RESULT_SUCCESS;
    message_t discover_message = {0}, response = {0};
    connected_node_t *connected = connected_nodes;
    uint32_t route[MAX_ROUTE_SIZE] = {0}, *temp_route = NULL;
    int sock = INVALID_SOCKET;
    if (get_connected_node(id) != NULL) {
        return NODE_FUNCTION_RESULT_SUCCESS;
    }
    discover_message = (message_t){
        .message_id = current_message_id++,
        .source_id = node_id,
        .destination_id = id,
        .trailing_message = 0,
        .function_id = FUNCTION_DISCOVER,
    };
    memset(discover_message.payload, '\0', PAYLOAD_SIZE);
    memmove(discover_message.payload, &id, sizeof(id));
    send_discover_to_connected_nodes(id, INVALID_NODE_ID);
    if (find_route(id) == NULL) {
        status = NODE_FUNCTION_RESULT_NO_ROUTE_FOUND;
    }
    else {
        status = NODE_FUNCTION_RESULT_SUCCESS;
    }

cleanup:
    if (sock != INVALID_SOCKET) {
        close(sock);
    }
    return status;
}

node_function_result_t node_route(uint32_t id) {
    CHECK_NODE_SET;
    uint32_t *route = NULL;
    route = discover_route(id);
    if (route != NULL) {
        printf(ACK_PRINT);
        print_route(route);
    }
    else {
        return NODE_FUNCTION_RESULT_NO_ROUTE_FOUND;
    }
    return NODE_FUNCTION_RESULT_SUCCESS;
}

node_function_result_t node_peers() {
    CHECK_NODE_SET;
    if (num_connections < 1) {
        return NODE_FUNCTION_RESULT_FAILURE;
    }
    int i = 0;
    connected_node_t *temp = connected_nodes;
    for (i = 0; i < num_connections - 1; ++i) {
        printf("%d,", temp->id);
        temp++;
    }
    printf("%d\n", temp->id);
    return NODE_FUNCTION_RESULT_SUCCESS;
}

node_function_result_t relay_message(message_t message, int client_fd) {
    node_function_result_t status = NODE_FUNCTION_RESULT_SUCCESS;
    connected_node_t *relay_to = NULL;
    message_t response = {0};
    uint32_t result_function = FUNCTION_ACK, *route = NULL;
    int sock = INVALID_SOCKET;
    relay_to = get_connected_node(relaying_id);
    if (relay_to == NULL) {
        status = NODE_FUNCTION_RESULT_FAILURE;
        result_function = FUNCTION_NACK;
    }
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        status = NODE_FUNCTION_RESULT_INVALID_SOCKET;
        result_function = FUNCTION_NACK;
    }
    if (connect(sock, (struct sockaddr *)&relay_to->addr, sizeof(relay_to->addr)) < 0) {
        status = NODE_FUNCTION_RESULT_FAILED_CONNECT;
        result_function = FUNCTION_NACK;
    }
    if (send(sock, &message, sizeof(message), 0) != sizeof(message)) {
        status = NODE_FUNCTION_RESULT_INVALID_MESSAGE;
        result_function = FUNCTION_NACK;
    }
    close(sock);
    sock = INVALID_SOCKET;
    response = (message_t){
        .message_id = current_message_id++,
        .source_id = node_id,
        .destination_id = message.source_id,
        .trailing_message = 0,
        .function_id = result_function,
    };
    memset(response.payload, '\0', PAYLOAD_SIZE);
    if (send(client_fd, &response, sizeof(message), 0) != sizeof(message)) {
        return NODE_FUNCTION_RESULT_FAILED_SEND;
    }
    return status;
}

node_function_result_t handle_node_function(int client_fd) {
    CHECK_NODE_SET;
    node_function_result_t status = NODE_FUNCTION_RESULT_SUCCESS;
    struct sockaddr_in sender = {0};
    connected_node_t *send_to = NULL;
    socklen_t sender_len = sizeof(sender);
    message_t message = {0}, response = {0};
    ssize_t bytes_read = 0;
    bytes_read = recvfrom(client_fd, &message, sizeof(message), 0, (struct sockaddr *)&sender, &sender_len);
    if (bytes_read != sizeof(message)) {
        return NODE_FUNCTION_RESULT_INVALID_MESSAGE;
    }
    if (relaying > 0) {
        relay_message(message, client_fd);
        relaying--;
    }
    if (message.destination_id != node_id && message.function_id != FUNCTION_CONNECT) {
        response = (message_t) {
            .message_id = current_message_id++,
            .source_id = node_id,
            .destination_id = message.source_id,
            .trailing_message = 0,
            .function_id = FUNCTION_NACK,
        };
        memset(response.payload, '\0', PAYLOAD_SIZE);
        if (send(client_fd, &response, sizeof(message), 0) != sizeof(message)) {
            return NODE_FUNCTION_RESULT_FAILED_SEND;
        }
        return NODE_FUNCTION_RESULT_INVALID_DESTINATION;
    }
    switch (message.function_id) {
    case FUNCTION_CONNECT:
        status = handle_node_connect(message, client_fd);
        break;
    case FUNCTION_SEND:
        status = handle_node_send(message, client_fd);
        break;
    case FUNCTION_DISCOVER:
        status = handle_node_discover(message, client_fd);
        break;
    case FUNCTION_RELAY:
        status = handle_node_relay(message, client_fd);
        break;
    default:
        status = NODE_FUNCTION_RESULT_INVALID_FUNCTION_ID;
        break;
    }
    return status;
}

node_function_result_t destroy() {
    if (connected_nodes != NULL) {
        free(connected_nodes);
    }
}