#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#define MAX_CLIENTS 100

// remove client from client list
void remove_client(int *clients, char ids[][50], char names[][50], int *count, int i)
{
    if (i < *count - 1)
    {
        clients[i] = clients[*count - 1];
        strcpy(ids[i], ids[*count - 1]);
        strcpy(names[i], names[*count - 1]);
    }
    (*count)--;
}

// check format name
int is_valid(char *msg, char *id, char *name)
{
    char tmp_id[50];
    char tmp_name[50];

    if (sscanf(msg, "%[^:]: %[^\n]", tmp_id, tmp_name) != 2)
    {
        return 0;
    }

    strcpy(id, tmp_id);
    strcpy(name, tmp_name);
    return 1;
}

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

    int clients[MAX_CLIENTS];
    char ids[MAX_CLIENTS][50] = {0};
    char names[MAX_CLIENTS][50] = {0};
    int count = 0;

    fd_set fdread;
    char buf[256];

    while (1)
    {
        // init fdread
        FD_ZERO(&fdread);
        FD_SET(listener, &fdread);
        int max_client = listener;
        for (int i = 0; i < count; i++)
        {
            FD_SET(clients[i], &fdread);
            if (clients[i] > max_client)
            {
                max_client = clients[i];
            }
        }

        // select
        int ret = select(max_client + 1, &fdread, NULL, NULL, NULL);
        if (ret < 0)
        {
            perror("select() failed");
            break;
        }
        else if (ret == 0)
        {
            printf("Time out.\n");
            continue;
        }

        // check new connection
        if (FD_ISSET(listener, &fdread))
        {
            int client = accept(listener, NULL, NULL);
            // max connection
            if (count == MAX_CLIENTS)
            {
                printf("Too many connections.\n");
                char *msg = "Sorry. Out of slots.\n";
                send(client, msg, strlen(msg), 0);
                close(client);
            }
            // add new client
            else
            {
                clients[count] = client;
                count++;
                printf("New client connected: %d\n", client);
                char *qs = "Enter your name (id: name): ";
                send(client, qs, strlen(qs), 0);
            }
        }

        // handle clients
        for (int i = 0; i < count; i++)
        {
            if (FD_ISSET(clients[i], &fdread))
            {
                // client disconnect
                ret = recv(clients[i], buf, sizeof(buf), 0);
                if (ret <= 0)
                {
                    printf("Client %d disconnected.\n", clients[i]);
                    remove_client(clients, ids, names, &count, i);
                    i--;
                }

                buf[ret - 1] = 0;

                // check client enter name
                if (is_valid(buf, ids[i], names[i]))
                {
                    printf("Save client: id=%s, name=%s\n", ids[i], names[i]);
                }
                // handle message received from client
                else if (ids[i][0] != '\0')
                {
                    printf("Receive: %s", buf);
                    printf("From client: %d (id: %s, name: %s)\n", clients[i], ids[i], names[i]);
                    char msg[512];
                    snprintf(msg, sizeof(msg), "%s: %s\n", ids[i], buf);
                    // send to other
                    for (int j = 0; j < count; j++)
                    {
                        if (j != i)
                            send(clients[j], msg, strlen(msg), 0);
                    }
                    printf("Sent to other clients.\n");
                }
                // wrong format
                else
                {
                    char *msg = "Invalid form, please retry: ";
                    send(clients[i], msg, strlen(msg), 0);
                }
            }
        }
    }

    close(listener);

    return 0;
}