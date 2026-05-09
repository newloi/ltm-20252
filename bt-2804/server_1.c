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
void remove_client(struct pollfd *fds, int *count, int i)
{
    close(fds[i].fd);

    if (i < *count - 1)
    {
        fds[i] = fds[*count - 1];
    }
    (*count)--;
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
            perror("select() failed");
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
                fds[count].fd = client;
                fds[count].events = POLLIN;
                count++;
                printf("New client connected: %d\n", client);
                char res[100];
                sprintf(res, "Hello. There are currently %d clients connected.\n", count - 1);
                send(client, res, strlen(res), 0);
            }
        }

        // handle clients
        for (int i = 1; i < count; i++)
        {
            if (fds[i].revents & POLLIN)
            {
                ret = recv(fds[i].fd, buf, sizeof(buf), 0);
                // client disconnect
                if (ret <= 0)
                {
                    printf("Client %d disconnected.\n", fds[i].fd);
                    remove_client(fds, &count, i);
                    i--;
                    continue;
                }

                buf[ret - 1] = 0;

                if (strcmp(buf, "exit") == 0)
                {
                    printf("Client %d disconnected.\n", fds[i].fd);
                    char *res = "Goodbye!";
                    send(fds[i].fd, res, strlen(res), 0);
                    remove_client(fds, &count, i);
                    i--;
                    continue;
                }

                for (int i = 0; i < ret - 1; i++)
                {
                    if ('0' <= buf[i] && buf[i] <= '9')
                        buf[i] = ('9' - buf[i]) + '0';
                    else if ('A' <= buf[i] && buf[i] <= 'Z')
                    {
                        if (buf[i] == 'Z')
                            buf[i] = 'A';
                        else
                            buf[i] = (buf[i] + 1);
                    }
                    else if ('a' <= buf[i] && buf[i] <= 'z')
                    {
                        if (buf[i] == 'z')
                            buf[i] = 'a';
                        else
                            buf[i] = (buf[i] + 1);
                    }
                }

                send(fds[i].fd, buf, strlen(buf), 0);
            }
        }
    }

    close(listener);

    return 0;
}