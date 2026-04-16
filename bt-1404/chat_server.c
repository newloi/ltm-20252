#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>

#define MAX_CLIENTS 100

// remove client from client list
void remove_client(struct pollfd *fds, char ids[][50], int *count, int i)
{
    if (i < *count - 1)
    {
        fds[i] = fds[*count - 1];
        strcpy(ids[i], ids[*count - 1]);
    }
    (*count)--;
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

    struct pollfd fds[MAX_CLIENTS + 1];
    char ids[MAX_CLIENTS + 1][50] = {0};
    int count = 0;

    fds[count].fd = listener;
    fds[count].events = POLLIN;
    count++;

    char buf[256];

    while (1)
    {
        int ret = poll(fds, count, -1);
        if (ret < 0)
        {
            perror("poll() failed");
            break;
        }
        else if (ret == 0)
        {
            printf("Time out.\n");
            continue;
        }

        // check new connection
        if (fds[0].revents & POLLIN)
        {
            int client = accept(listener, NULL, NULL);
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
                fds[count].fd = client;
                fds[count].events = POLLIN;
                count++;
                printf("New client connected: %d\n", client);
                char *qs = "Enter your name: ";
                send(client, qs, strlen(qs), 0);
            }
        }

        // handle clients
        for (int i = 1; i < count; i++)
        {
            if (fds[i].revents & POLLIN)
            {
                // client disconnect
                ret = recv(fds[i].fd, buf, sizeof(buf), 0);
                if (ret <= 0)
                {
                    printf("Client %d disconnected.\n", fds[i].fd);
                    remove_client(fds, ids, &count, i);
                    i--;
                    continue;
                }

                buf[ret - 1] = 0;

                // check client enter name
                if (is_valid(buf, ids[i]))
                {
                    printf("Save client: id=%s\n", ids[i]);
                }
                // handle message received from client
                else if (ids[i][0] != '\0')
                {
                    printf("Receive: %s\n", buf);
                    printf("From client: %d (%s)\n", fds[i].fd, ids[i]);
                    char msg[512];
                    snprintf(msg, sizeof(msg), "%s: %s\n", ids[i], buf);
                    // send to other
                    for (int j = 1; j < count; j++)
                    {
                        if (j != i)
                            send(fds[j].fd, msg, strlen(msg), 0);
                    }
                    printf("Sent to other clients.\n");
                }
                // wrong format
                else
                {
                    char *msg = "Invalid form, please retry: ";
                    send(fds[i].fd, msg, strlen(msg), 0);
                }
            }
        }
    }

    close(listener);

    return 0;
}