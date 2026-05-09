#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>

#define MAX_CLIENTS 1000

// unsubscribe client
void unsubscribe(int clients[100], int client)
{
    if (clients[0] <= 0)
        return;
    if (clients[0] == 1)
    {
        clients[0]--;
        return;
    }

    for (int i = 1; i <= clients[0]; i++)
    {
        if (clients[i] == client)
        {
            clients[i] = clients[clients[0]];
            clients[0]--;
            return;
        }
    }
}

// remove client from client list
void remove_client(int ntopics, int topics_clients[100][100], struct pollfd *fds, int *nfds, int i)
{
    // close client
    close(fds[i].fd);

    // unsub all topic
    for (int i = 0; i < ntopics; i++)
    {
        unsubscribe(topics_clients[i], fds[i].fd);
    }

    if (i < *nfds - 1)
    {
        fds[i] = fds[*nfds - 1];
    }
    (*nfds)--;
}

// check command SUB/PUB/UNSUB
int is_valid(char *buf, char *topic, char *message)
{
    char cmd[50];
    char tmp_topic[100];
    char tmp_message[100];

    int ret = sscanf(buf, "%s %s %[^\n]", cmd, tmp_topic, tmp_message);

    if (strcmp(cmd, "SUB") == 0 && ret == 2)
    {
        strcpy(topic, tmp_topic);
        return 1;
    }

    if (strcmp(cmd, "PUB") == 0 && ret == 3)
    {
        strcpy(topic, tmp_topic);
        strcpy(message, tmp_message);
        return 2;
    }

    if (strcmp(cmd, "UNSUB") == 0 && ret == 2)
    {
        strcpy(topic, tmp_topic);
        return 3;
    }

    return 0;
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
    int nfds = 0;

    fds[nfds].fd = listener;
    fds[nfds].events = POLLIN;
    nfds++;

    // list topics (topic i has name topics[i])
    char topics[100][100];
    // topic i has topics_clients[i][0] clients subscribed: topics_clients[i][1], ... , topics_clients[i][topics_clients[i][0]]
    int topics_clients[100][100];
    int ntopics = 0;
    char buf[256];

    while (1)
    {
        int ret = poll(fds, nfds, -1);
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
            if (nfds == MAX_CLIENTS)
            {
                printf("Too many connections.\n");
                char *msg = "Sorry. Out of slots.\n";
                send(client, msg, strlen(msg), 0);
                close(client);
            }
            // add new client
            else
            {
                fds[nfds].fd = client;
                fds[nfds].events = POLLIN;
                nfds++;
                printf("New client connected: %d\n", client);
            }
        }

        // handle clients
        for (int i = 1; i < nfds; i++)
        {
            if (fds[i].revents & POLLIN)
            {
                int sender = fds[i].fd;
                ret = recv(sender, buf, sizeof(buf), 0);
                // client disconnect
                if (ret <= 0)
                {
                    printf("Client %d disconnected.\n", sender);
                    remove_client(ntopics, topics_clients, fds, &nfds, i);
                    i--;
                    continue;
                }

                buf[ret - 1] = 0;

                // client exit
                if (strcmp(buf, "exit") == 0)
                {
                    printf("Client %d disconnected.\n", sender);
                    char *res = "Goodbye!";
                    send(sender, res, strlen(res), 0);
                    remove_client(ntopics, topics_clients, fds, &nfds, i);
                    i--;
                    continue;
                }

                char topic[100], message[100];
                int valid = is_valid(buf, topic, message);
                // client sub topic
                if (valid == 1)
                {
                    int had_topic = 0;
                    // check topic is existed
                    for (int j = 0; j < ntopics; j++)
                    {
                        if (strcmp(topics[j], topic) == 0)
                        {
                            printf("Client %d da sub vao topic %s\n", sender, topic);
                            topics_clients[j][0]++;
                            topics_clients[j][topics_clients[j][0]] = sender;
                            had_topic = 1;
                            break;
                        }
                    }
                    // new topic
                    if (had_topic == 0)
                    {
                        strcpy(topics[ntopics], topic);
                        topics_clients[ntopics][0] = 1;
                        topics_clients[ntopics][1] = sender;
                        ntopics++;
                        printf("Co topic moi %s\n", topic);
                    }
                }
                // publish message
                else if (valid == 2)
                {
                    for (int j = 0; j < ntopics; j++)
                    {
                        // exist topic and has over 1 clients
                        if (strcmp(topics[j], topic) == 0 && topics_clients[j][0] > 0)
                        {
                            printf("Client %d da sub vao topic %s va gui message %s\n", sender, topic, message);
                            for (int k = 1; k <= topics_clients[j][0]; k++)
                            {
                                send(topics_clients[j][k], message, strlen(message), 0);
                            }
                        }
                    }
                }
                // client unsub topic
                else if (valid == 3)
                {
                    for (int j = 0; j < ntopics; j++)
                    {
                        if (strcmp(topics[j], topic) == 0)
                        {
                            printf("Client %d da unsub topic %s\n", sender, topic);
                            unsubscribe(topics_clients[j], sender);
                        }
                    }
                }
            }
        }
    }

    close(listener);

    return 0;
}