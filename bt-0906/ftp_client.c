#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int passive_and_connect(int client) {
    char buf[1024], cmd[100];

    // send PASV to switch to passive mode
    sprintf(cmd, "PASV\r\n");
    send(client, cmd, strlen(cmd), 0);
    int ret = recv(client, buf, sizeof(buf), 0);
    if (strncmp(buf, "227", 3) != 0) {
        buf[ret] = 0;
        printf("PASV failed: %s\n", buf);
        return -1;
    }

    char data_ip[64];
    int data_port;
    // get ip and port from response
    const char* start = strchr(buf, '(');
    if (!start) return -1;
    int ip1, ip2, ip3, ip4, p1, p2;
    if (sscanf(start, "(%d,%d,%d,%d,%d,%d)", &ip1, &ip2, &ip3, &ip4, &p1, &p2) == 6) {
        sprintf(data_ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
        data_port = p1 * 256 + p2;
    }
    // printf("Connecting to data socket at %s:%d\n", data_ip, data_port);
    // connect to data socket
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        perror("socket");
        return -1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(data_port);
    if (inet_pton(AF_INET, data_ip, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return -1;
    }
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Connect to data socket error: ");
        close(sock);
        return -1;
    }
    return sock;
}

int main() {
    int client = socket(AF_INET, SOCK_STREAM, 0);
    if (client < 0) {
        perror("socket");
        return 1;
    }

    // get address info of hostname lebavui.io.vn
    struct addrinfo* res;
    if (getaddrinfo("lebavui.io.vn", "21", NULL, &res) != 0) {
        perror("getaddrinfo");
        close(client);
        return 1;
    }

    // connect to FTP server
    if (connect(client, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Connect to FTP server error: ");
        freeaddrinfo(res);
        close(client);
        return 1;
    }
    freeaddrinfo(res);

    char buf[4096];

    // get connect status
    recv(client, buf, sizeof(buf), 0);
    if (strncmp(buf, "220", 3) != 0) {
        printf("Unexpected greeting: %s\n", buf);
        close(client);
        return 1;
    }

    char user[100];
    char pass[100];
    char cmd[1024];

    printf("Enter your account:\n");
    scanf("%s", user);
    scanf("%s", pass);

    // send USER
    sprintf(cmd, "USER %s\r\n", user);
    send(client, cmd, strlen(cmd), 0);
    recv(client, buf, sizeof(buf), 0);
    if (strncmp(buf, "331", 3) != 0) {
        printf("USER command failed: %s\n", buf);
        close(client);
        return 1;
    }

    // send PASS
    sprintf(cmd, "PASS %s\r\n", pass);
    send(client, cmd, strlen(cmd), 0);
    recv(client, buf, sizeof(buf), 0);
    if (strncmp(buf, "230", 3) != 0) {
        printf("PASS command failed: %s\n", buf);
        close(client);
        return 1;
    }

    printf("Logged in successfully!\n");

    int data_sock = passive_and_connect(client);
    if (data_sock < 0) {
        printf(("Switch to passive mode failed\n"));
        close(client);
        return 1;
    }

    // send LIST to get list files, folders in FTP server
    sprintf(cmd, "LIST\r\n");
    send(client, cmd, strlen(cmd), 0);
    recv(client, buf, sizeof(buf), 0);
    recv(client, buf, sizeof(buf), 0);
    if (strncmp(buf, "226", 3) != 0) {
        printf("LIST failed: %s\n", buf);
        close(data_sock);
        close(client);
        return 1;
    }

    int ret = recv(data_sock, buf, sizeof(buf), 0);
    buf[ret] = 0;
    printf("Files in FTP server:\n%s\nChoose a file to download: ", buf);
    char filename[100];
    scanf("%s", filename);

    data_sock = passive_and_connect(client);
    if (data_sock < 0) {
        printf(("Switch to passive mode failed\n"));
        close(client);
        return 1;
    }

    // send RETR to download file from server
    sprintf(cmd, "RETR %s\r\n", filename);
    send(client, cmd, strlen(cmd), 0);
    recv(client, buf, sizeof(buf), 0);
    recv(client, buf, sizeof(buf), 0);
    if (strncmp(buf, "226", 3) != 0) {
        printf("Download failed: %s\n", buf);
        close(data_sock);
        close(client);
        return 1;
    }

    ret = recv(data_sock, buf, sizeof(buf), 0);
    buf[ret - 1] = 0;
    // printf("File content: %s\n", buf);

    // create file question_xxxxx
    FILE* qs = fopen(filename, "w");
    if (qs != NULL) {
        fwrite(buf, 1, ret, qs);
        fclose(qs);
        printf("Saved to file: %s\n", filename);
    } else {
        perror("Failed to create file");
    }

    // create file answer_xxxxx
    char answer_file[512];
    char* underscore = strchr(filename, '_');
    sprintf(answer_file, "answer%s", underscore);

    char answer_content[512];
    for (int i = 0; i < ret - 1; i++) {
        answer_content[i] = buf[ret - 2 - i];
    }
    answer_content[ret - 1] = '\0';
    // printf("Reversed content: %s\n", answer_content);

    // save file answer
    FILE* as = fopen(answer_file, "w");
    if (as != NULL) {
        fwrite(answer_content, 1, ret, as);
        fclose(as);
        printf("Saved to file: %s\n", answer_file);
    } else {
        perror("Failed to create file");
    }

    data_sock = passive_and_connect(client);

    // send STOR to upload file to server
    sprintf(cmd, "STOR %s\r\n", answer_file);
    send(client, cmd, strlen(cmd), 0);
    recv(client, buf, sizeof(buf), 0);
    send(data_sock, answer_content, strlen(answer_content), 0);
    close(data_sock);
    recv(client, buf, sizeof(buf), 0);
    if (strncmp(buf, "226", 3) != 0) {
        printf("Upload failed: %s\n", buf);
        close(data_sock);
        close(client);
        return 1;
    }
    printf("Uploaded file %s to server\n", answer_file);

    close(client);
    printf("Done!\n");
    return 0;
}