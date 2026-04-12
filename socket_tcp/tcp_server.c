#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[])
{
    // open file and get message
    FILE *f = fopen(argv[2], "rb");
    if (f == NULL)
    {
        perror("Open file error: ");
        exit(1);
    }
    char message[1024];
    fread(message, 1, sizeof(message), f);

    // open file to log
    FILE *log = fopen(argv[3], "a");
    if (log == NULL)
    {
        perror("Open log error: ");
        exit(1);
    }

    // open socket TCP
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // address IPv4
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(atoi(argv[1]));

    // bind socket to address
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("Bind socket error: ");
        exit(1);
    }

    // max 5 connection
    listen(listener, 5);

    int client = accept(listener, NULL, NULL);
    if (client != -1)
    {
        // send message to client
        send(client, message, sizeof(message), 0);

        // receive message from client
        char request[256];
        while (1)
        {
            int n = recv(client, request, sizeof(request), 0);

            // socket is close or error
            if (n <= 0)
                break;

            // write message to file
            request[n] = 0;
            fprintf(log, "%s", request);
            fflush(log);
        }
    }

    // close socket
    close(listener);
    close(client);

    // close files
    fclose(f);
    fclose(log);
}