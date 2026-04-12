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
void remove_client(int *clients, int *count, int i)
{
    if (i < *count - 1)
    {
        clients[i] = clients[*count - 1];
    }
    (*count)--;
}

// login with account received from client
int login(int num_account, char username[][50], char password[][50], char *client_username, char *client_password)
{
    for (int i = 0; i < num_account; i++)
    {
        if (strcmp(username[i], client_username) == 0)
        {
            if (strcmp(password[i], client_password) == 0)
                return 1;
            else
                break;
        }
    }

    return 0;
}

int main()
{
    // open file
    FILE *accounts = fopen("accounts.txt", "rb");
    if (accounts == NULL)
    {
        perror("Open file error: ");
        exit(1);
    }

    // get account list
    int num_acc;
    fscanf(accounts, "%d", &num_acc);

    char username[num_acc][50];
    char password[num_acc][50];
    for (int i = 0; i < num_acc; i++)
    {
        fscanf(accounts, "%s %s", username[i], password[i]);
    }

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

    int clients[MAX_CLIENTS] = {0};
    int count = 0;
    int is_login[MAX_CLIENTS] = {0};

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
                char *qs = "Enter your account (username password): ";
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
                    remove_client(clients, &count, i);
                    i--;
                }

                buf[ret - 1] = 0;

                // check login
                if (is_login[i])
                {
                    char command[512];
                    snprintf(command, sizeof(command), "%s > out.txt", buf);
                    if (system(command) != 0)
                    {
                        char *res = "Cannot execute command.\n";
                        send(clients[i], res, strlen(res), 0);
                        perror("Cannot execute command.");
                        continue;
                    }
                    printf("Execute command %s\n", buf);
                }
                else
                {
                    char client_username[50], client_password[50];
                    // get account
                    if (sscanf(buf, "%s %s", client_username, client_password) != 2)
                    {
                        char *res = "Please login first.\n";
                        send(clients[i], res, strlen(res), 0);
                        continue;
                    }
                    // login
                    if (login(num_acc, username, password, client_username, client_password))
                    {
                        is_login[i] = 1;
                        char *res = "Login successful.\n";
                        send(clients[i], res, strlen(res), 0);
                        printf("Client %d login successful.\n", clients[i]);
                    }
                    else
                    {
                        char *res = "Username or password is incorrect.\n";
                        send(clients[i], res, strlen(res), 0);
                    }
                }
            }
        }
    }

    close(listener);
    fclose(accounts);

    return 0;
}