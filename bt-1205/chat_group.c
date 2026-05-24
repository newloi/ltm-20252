#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>

struct client_params
{
    int client_src;
    int client_dst;
};

void *client_thread(void *);

int main()
{
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1)
    {
        perror("socket() failed");
        return 1;
    }

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)))
    {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("bind() failed");
        close(listener);
        return 1;
    }

    if (listen(listener, 5))
    {
        perror("listen() failed");
        close(listener);
        return 1;
    }

    // Server is now listening for incoming connections
    printf("Server is listening on port 9000...\n");

    int other_client = 0; // client queue
    while (1)
    {
        int client = accept(listener, NULL, NULL);
        printf("New client connected: %d\n", client);

        if (other_client == 0) // queue empty
        {
            char *res = "Please wait for another client to join the conversation.\n";
            send(client, res, strlen(res), 0);
            other_client = client; // update client queue
        }
        else // queue had 1 client
        {
            // create thread for client 1
            pthread_t thread_client_1;
            struct client_params client_params_1;
            client_params_1.client_src = client;
            client_params_1.client_dst = other_client;
            pthread_create(&thread_client_1, NULL, client_thread, &client_params_1);
            pthread_detach(thread_client_1);

            // create thread for client 2
            pthread_t thread_client_2;
            struct client_params client_params_2;
            client_params_2.client_src = other_client;
            client_params_2.client_dst = client;
            pthread_create(&thread_client_2, NULL, client_thread, &client_params_2);
            pthread_detach(thread_client_2);

            other_client = 0; // reset client queue
        }
    }

    close(listener);
    return 0;
}

void *client_thread(void *params)
{
    struct client_params client = *(struct client_params *)params;
    char buf[256];

    char *welcom = "Start the conversation.\n";
    send(client.client_src, welcom, strlen(welcom), 0);
    while (1)
    {
        int len = recv(client.client_src, buf, sizeof(buf), 0);
        if (len == 0)
            break;
        else if (len > 0)
        {
            buf[len] = 0;
            send(client.client_dst, buf, strlen(buf), 0); // fordward message to other client
        }
    }

    close(client.client_src); // close current client
    char *res = "The other client has disconnected, this client will be automatically disconnected.\n";
    send(client.client_dst, res, strlen(res), 0);
    close(client.client_dst); // close other client
}