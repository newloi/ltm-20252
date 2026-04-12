#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
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
    // open socket
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // server address IPv4
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    // connect to server
    if (connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr)))
    {
        perror("Connect to server error: ");
        exit(1);
    }

    // send data to server
    struct student sv;
    while (1)
    {
        // enter data
        printf("Nhập thông tin sinh viên:\n");
        printf("\tMSSV: ");
        scanf("%d", &sv.id);
        getchar(); // remove '\n' in buffer
        printf("\tHọ và tên: ");
        fgets(sv.name, sizeof(sv.name), stdin);
        printf("\tNgày sinh: ");
        fgets(sv.dob, sizeof(sv.dob), stdin);
        printf("\tCPA: ");
        scanf("%f", &sv.cpa);
        getchar();

        // send data to server
        int n = send(client, &sv, sizeof(sv), 0);
        if (n <= 0)
        {
            perror("Send data error: ");
        }
    }

    // close socket
    close(client);
}