#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// send response to client
void send_http_response(int client_sock, int status_code, const char* status_text, const char* html_content) {
    char header[512];
    int header_len = sprintf(header,
                             "HTTP/1.1 %d %s\r\n"
                             "Content-Type: text/html; charset=utf-8\r\n"
                             "Content-Length: %ld\r\n"
                             "Connection: close\r\n"
                             "\r\n",
                             status_code, status_text, strlen(html_content));
    send(client_sock, header, header_len, 0);
    send(client_sock, html_content, strlen(html_content), 0);
}

void* client_thread(void* params) {
    int client = *(int*)params;
    printf("New request from client: %d\n", client);

    char request[1024];
    int ret = recv(client, request, sizeof(request), 0);
    if (ret > 0) {
        request[ret] = '\0';
        char method[10];
        char endpoint[100];
        sscanf(request, "%s %s", method, endpoint);  // get method and endpoint
        printf("Request received: %s %s\n", method, endpoint);

        // method GET
        if (strcmp(method, "GET") == 0) {
            char* param_at = strchr(endpoint, '?');
            char* params;

            if (param_at != NULL) {     // has query params
                *param_at = '\0';       // split endpoint and query params
                params = param_at + 1;  // get query params

                if (strcmp(endpoint, "/calculator") == 0) {
                    double a, b;
                    char operator[5];
                    int has_a = 0, has_b = 0, has_op = 0;

                    // get each query param
                    while (param_at != NULL) {
                        param_at = strchr(params, '&');
                        if (param_at != NULL) *param_at = '\0';
                        if (strncmp(params, "a=", 2) == 0) {  // a
                            has_a = 1;
                            a = atof(params + 2);
                        } else if (strncmp(params, "b=", 2) == 0) {  // b
                            has_b = 1;
                            b = atof(params + 2);
                        } else if (strncmp(params, "op=", 3) == 0) {  // operator
                            has_op = 1;
                            strcpy(operator, params + 3);
                        }
                        params = param_at + 1;
                    }

                    // response the result to client
                    char response_html[1024];
                    if (has_a && has_b && has_op) {
                        double result = 0;
                        int valid_op = 1;

                        // calculation
                        if (strcmp(operator, "+") == 0)
                            result = a + b;
                        else if (strcmp(operator, "-") == 0)
                            result = a - b;
                        else if (strcmp(operator, "*") == 0)
                            result = a * b;
                        else if (strcmp(operator, "/") == 0) {
                            if (b != 0)
                                result = a / b;
                            else
                                valid_op = 0;
                        } else {
                            valid_op = 0;
                        }

                        if (valid_op) {
                            sprintf(response_html,
                                    "<html><body><h2>Kết quả (GET): %.1f %s %.1f = %.1f</h2></body></html>", a,
                                    operator, b, result);
                            send_http_response(client, 200, "OK", response_html);
                        } else {
                            send_http_response(client, 400, "Bad Request",
                                               "<html><body><h2>Phép tính không hợp lệ</h2></body></html>");
                        }
                    } else {  // some params is empty
                        const char* response_html =
                            "<html><body>"
                            "<h2>Thiếu tham số</h2>"
                            "</body></html>";
                        send_http_response(client, 400, "Bad request", response_html);
                    }
                } else {  // other endpoint not supported
                    send_http_response(client, 404, "Not Found", "<html><body><h2>404 Not Found</h2></body></html>");
                }
            } else {  // has no query params
                const char* response_html =
                    "<html><body>"
                    "<h2>Thiếu tham số</h2>"
                    "</body></html>";
                send_http_response(client, 400, "Bad request", response_html);
            }
        } else if (strcmp(method, "POST") == 0) {  // method POST
            if (strcmp(endpoint, "/calculator") == 0) {
                char* body = strstr(request, "\r\n\r\n");  // get body request
                if (body != NULL) {                        // has body
                    body += 4;                             // skip "\r\n\r\n"
                    char* params = body;
                    char* param_at = body;

                    double a, b;
                    char operator[5];
                    int has_a = 0, has_b = 0, has_op = 0;

                    // get each query param
                    while (param_at != NULL) {
                        param_at = strchr(params, '&');
                        if (param_at != NULL) *param_at = '\0';
                        if (strncmp(params, "a=", 2) == 0) {
                            has_a = 1;
                            a = atof(params + 2);
                        } else if (strncmp(params, "b=", 2) == 0) {
                            has_b = 1;
                            b = atof(params + 2);
                        } else if (strncmp(params, "op=", 3) == 0) {
                            has_op = 1;
                            strcpy(operator, params + 3);
                        }
                        params = param_at + 1;
                    }

                    // send response to client
                    char response_html[1024];
                    if (has_a && has_b && has_op) {
                        double result = 0;
                        int valid_op = 1;

                        // claculation
                        if (strcmp(operator, "+") == 0)
                            result = a + b;
                        else if (strcmp(operator, "-") == 0)
                            result = a - b;
                        else if (strcmp(operator, "*") == 0)
                            result = a * b;
                        else if (strcmp(operator, "/") == 0) {
                            if (b != 0)
                                result = a / b;
                            else
                                valid_op = 0;
                        } else {
                            valid_op = 0;
                        }

                        if (valid_op) {
                            sprintf(response_html,
                                    "<html><body><h2>Kết quả (POST): %.1f %s %.1f = %.1f</h2></body></html>", a,
                                    operator, b, result);
                            send_http_response(client, 200, "OK", response_html);
                        } else {
                            send_http_response(client, 400, "Bad Request",
                                               "<html><body><h2>Phép tính không hợp lệ</h2></body></html>");
                        }
                    } else {
                        const char* response_html =
                            "<html><body>"
                            "<h2>Thiếu tham số</h2>"
                            "</body></html>";
                        send_http_response(client, 400, "Bad request", response_html);
                    }
                } else {  // has body
                    const char* response_html =
                        "<html><body>"
                        "<h2>Yêu cầu không hợp lệ</h2>"
                        "</body></html>";
                    send_http_response(client, 400, "Bad request", response_html);
                }
            } else {  // other endpoint not supported
                send_http_response(client, 404, "Not Found", "<html><body><h2>404 Not Found</h2></body></html>");
            }
        }
    }

    close(client);
    return NULL;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == -1) {
        perror("socket() failed");
        exit(1);
    }

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);
    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr))) {
        perror("bind() failed");
        close(listener);
        exit(1);
    }

    if (listen(listener, 5)) {
        perror("listen() failed");
        close(listener);
        exit(1);
    }

    printf("Server is listening on port 9000...\n");

    while (1) {
        int client = accept(listener, NULL, NULL);
        // create a new thread for new client
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, client_thread, &client);
        pthread_detach(thread_id);
    }

    close(listener);
    return 0;
}