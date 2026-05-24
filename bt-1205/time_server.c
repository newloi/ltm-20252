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

void *client_thread(void *params);

void getTimeWithFormat(char *format, char *date_str, int size)
{
    time_t t = time(NULL);
    struct tm *info = localtime(&t);

    if (strcmp(format, "dd/mm/yyyy") == 0)
        strftime(date_str, size, "%d/%m/%Y", info);
    else if (strcmp(format, "dd/mm/yy") == 0)
        strftime(date_str, size, "%d/%m/%y", info);
    else if (strcmp(format, "mm/dd/yyyy") == 0)
        strftime(date_str, size, "%m/%d/%Y", info);
    else if (strcmp(format, "mm/dd/yy") == 0)
        strftime(date_str, size, "%m/%d/%y", info);
    else
        strcpy(date_str, "Invalid format.\0");
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

    while (1)
    {
        int client = accept(listener, NULL, NULL);
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, client_thread, &client);
        pthread_detach(thread_id);
    }

    return 0;
}

void *client_thread(void *params)
{
    int client = *(int *)params;

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
        printf("Command from client %d: %s", client, buf);

        // check command
        char cmd[50], format[50], tmp[50];
        if (sscanf(buf, "%s %s %s", cmd, format, tmp) != 2 || strcmp(cmd, "GET_TIME") != 0)
        {
            char *res = "Invalid command.\n";
            send(client, res, strlen(res), 0);
            continue;
        }
        else
        {
            // check format
            char result[20];
            getTimeWithFormat(format, result, sizeof(result));
            send(client, result, strlen(result), 0);
        }
    }
    close(client);
}