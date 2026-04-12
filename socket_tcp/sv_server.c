#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

struct student
{
    int id;
    char name[100];
    char dob[11];
    float cpa;
};

int main(int argc, char *argv[])
{
    // open log file
    FILE *log = fopen(argv[2], "a");
    if (log == NULL)
    {
        perror("Open file error: ");
        exit(1);
    }

    // open socket
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // server address IPv4
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(atoi(argv[1]));

    // bind socket
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("Bind socket error: ");
        exit(1);
    }

    // max 5 connection
    listen(listener, 5);

    // accept client
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    int client = accept(listener, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client > -1)
    {
        struct student sv;
        while (1)
        {
            // receive data from client
            int n = recv(client, &sv, sizeof(sv), 0);

            // socket is close or error
            if (n <= 0)
                break;

            // get time now
            time_t rawTime;
            time(&rawTime);
            struct tm *current;
            current = localtime(&rawTime);
            char now[100];
            strftime(now, sizeof(now), "%Y-%m-%d %H:%M:%S", current);

            // show
            printf("Received:\n");
            printf("\tMSSV: %d\n", sv.id);
            sv.name[strcspn(sv.name, "\n")] = 0;
            printf("\tHọ và tên: %s\n", sv.name);
            sv.dob[strcspn(sv.dob, "\n")] = 0;
            printf("\tNgày sinh: %s\n", sv.dob);
            printf("\tCPA: %.2f\n", sv.cpa);

            // write into log
            fprintf(log, "%s %s %d %s %s %.2f\n", inet_ntoa(client_addr.sin_addr), now, sv.id, sv.name, sv.dob, sv.cpa);
            fflush(log);
        }
    }

    // close socket
    close(listener);
    close(client);

    // close file
    fclose(log);
}