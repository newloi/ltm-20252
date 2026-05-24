#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>

#define MAX_CLIENTS 100

int count = 0;
int clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

// remove client from client list
void remove_client(int client)
{
    pthread_mutex_lock(&clients_mutex);
    pthread_mutex_lock(&count_mutex);
    if (count != 1)
    {
        for (int i = 0; i < count; i++)
        {
            if (clients[i] == client)
            {
                clients[i] = clients[count - 1];
                break;
            }
        }
    }
    count--;
    pthread_mutex_unlock(&clients_mutex);
    pthread_mutex_unlock(&count_mutex);
}

// check format name
int is_valid(char *msg, char *id)
{
    char tmp1[50];
    char tmp_id[50];
    char tmp2[50];

    if (sscanf(msg, "%s%s%s", tmp1, tmp_id, tmp2) != 2)
    {
        return 0;
    }

    if (strcmp(tmp1, "client_id:") != 0)
        return 0;

    strcpy(id, tmp_id);
    return 1;
}

void *client_thread(void *params);

int main()
{
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1)
    {
        perror("socket() failed");
        exit(1);
    }

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)))
    {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("bind() failed");
        close(listener);
        exit(1);
    }

    if (listen(listener, MAX_CLIENTS))
    {
        perror("listen() failed");
        close(listener);
        exit(1);
    }

    printf("Server is listening on port 9000...\n");

    char buf[256];
    while (1)
    {
        int client = accept(listener, NULL, NULL);
        printf("New client connected: %d\n", client);

        // max connection
        if (count == MAX_CLIENTS + 1)
        {
            printf("Too many connections.\n");
            char *msg = "Sorry. Out of slots.\n";
            send(client, msg, strlen(msg), 0);
            close(client);
        }
        // add new client
        else
        {
            pthread_mutex_lock(&clients_mutex);
            pthread_mutex_lock(&count_mutex);
            clients[count] = client;
            count++;
            pthread_mutex_unlock(&clients_mutex);
            pthread_mutex_unlock(&count_mutex);
            pthread_t thread_id;
            pthread_create(&thread_id, NULL, client_thread, &client);
            pthread_detach(thread_id);
        }
    }

    close(listener);

    return 0;
}

void *client_thread(void *params)
{
    int client = *(int *)params;

    char *qs = "Enter your name: ";
    send(client, qs, strlen(qs), 0);

    char buf[256];
    char id[50] = {0};
    while (1)
    {
        // client disconnect
        int ret = recv(client, buf, sizeof(buf), 0);
        if (ret <= 0)
        {
            printf("Client %d disconnected.\n", client);
            remove_client(client);
            break;
        }

        buf[ret - 1] = 0;

        // check client enter name
        if (is_valid(buf, id))
        {
            printf("Save client: id=%s\n", id);
        }
        // handle message received from client
        else if (id[0] != 0)
        {
            printf("Receive: %s\n", buf);
            printf("From client: %d (%s)\n", client, id);
            char msg[512];
            snprintf(msg, sizeof(msg), "%s: %s\n", id, buf);
            // send to other
            for (int i = 0; i < count; i++)
            {
                if (i != client)
                {
                    pthread_mutex_lock(&clients_mutex);
                    send(clients[i], msg, strlen(msg), 0);
                    pthread_mutex_unlock(&clients_mutex);
                }
            }
            printf("Sent to other clients.\n");
        }
        // wrong format
        else
        {
            char *msg = "Invalid form, please retry: ";
            send(client, msg, strlen(msg), 0);
        }
    }
}