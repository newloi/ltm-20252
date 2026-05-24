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

int num_acc = 0;
char username[100][50];
char password[100][50];

void *client_thread(void *params);

// login with account received from client
int login(char *client_username, char *client_password)
{
    for (int i = 0; i < num_acc; i++)
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
    fscanf(accounts, "%d", &num_acc);
    for (int i = 0; i < num_acc; i++)
    {
        fscanf(accounts, "%s %s", username[i], password[i]);
    }
    fclose(accounts);

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

    while (1)
    {
        int client = accept(listener, NULL, NULL);
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, client_thread, &client);
        pthread_detach(thread_id);
    }

    close(listener);
    return 0;
}

void *client_thread(void *params)
{
    int client = *(int *)params;

    // Required username/pasword
    printf("New client connected: %d\n", client);
    char *qs = "Enter your account (username password): ";
    send(client, qs, strlen(qs), 0);

    int isLogin = 0;
    char buf[256];
    while (1)
    {
        int ret = recv(client, buf, sizeof(buf), 0);
        // client disconnect
        if (ret <= 0)
        {
            printf("Client %d disconnected.\n", client);
            break;
        }
        buf[ret - 1] = 0;

        // check login
        if (isLogin)
        {
            char command[512];
            snprintf(command, sizeof(command), "%s > out.txt", buf);
            // run command
            if (system(command) != 0)
            {
                char *res = "Cannot execute command.\n";
                send(client, res, strlen(res), 0);
                perror("Cannot execute command.");
                continue;
            }
            printf("Execute command %s\n", buf);
        }
        else
        {
            char client_username[50], client_password[50], tmp[50];
            // get account
            if (sscanf(buf, "%s %s %s", client_username, client_password, tmp) != 2)
            {
                char *res = "Please login first.\n";
                send(client, res, strlen(res), 0);
                continue;
            }
            // login
            if (login(client_username, client_password))
            {
                isLogin = 1;
                char *res = "Login successful.\n";
                send(client, res, strlen(res), 0);
                printf("Client %d login successful.\n", client);
            }
            else
            {
                char *res = "Username or password is incorrect.\n";
                send(client, res, strlen(res), 0);
            }
        }
    }
    close(client);
}