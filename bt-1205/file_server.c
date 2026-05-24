#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_CLIENTS 100

// close process
void signalHandler()
{
    int pid = wait(NULL);
    printf("Child process %d terminated.\n", pid);
    return;
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

    // open folder
    DIR *d;
    struct dirent *dir;
    d = opendir("./files");

    // handler close process
    signal(SIGCHLD, signalHandler);
    while (1)
    {
        int client = accept(listener, NULL, NULL);
        if (fork() == 0) // on child process
        {
            close(listener);

            printf("New client connected: %d\n", client);

            char buf[1024];
            int buf_size = 0;

            if (d)
            {
                int count_file = 0;
                // all files in folder
                while ((dir = readdir(d)) != NULL)
                {
                    if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
                        continue;

                    count_file++;
                    char file_name[300];
                    // get file name and push to buf
                    int file_name_size = sprintf(file_name, "%s\r\n", dir->d_name);
                    memcpy(buf + buf_size, file_name, file_name_size);
                    buf_size += file_name_size;
                }
                // close folder
                closedir(d);

                // folder has no file
                if (count_file == 0)
                {
                    strcpy(buf, "ERROR No files to download \r\n");
                    send(client, buf, strlen(buf), 0);
                }
                else
                {
                    // message to client
                    char files_msg[2048];
                    sprintf(files_msg, "OK %d\r\n%s\r\n\r\n", count_file, buf);
                    send(client, files_msg, strlen(files_msg), 0);

                    char msg[100];
                    while (1)
                    {
                        // get command from client
                        int ret = recv(client, msg, sizeof(msg), 0);
                        if (ret == 0)
                            break;
                        if (ret > 0)
                        {
                            char file_path[1024];
                            msg[strcspn(msg, "\r\n")] = 0;
                            sprintf(file_path, "./files/%s", msg);
                            // open file that required from client
                            FILE *f = fopen(file_path, "rb");
                            if (f == NULL)
                            {
                                char *res = "File doesn't exist.\n";
                                send(client, res, strlen(res), 0);
                            }
                            else
                            {
                                // get file size
                                fseek(f, 0, SEEK_END);
                                long file_size = ftell(f);
                                fseek(f, 0, SEEK_SET); // back to start file

                                char header[64];
                                int header_len = sprintf(header, "OK %ld\r\n", file_size);
                                send(client, header, header_len, 0);

                                char file_buf[1024];
                                int bytes_read;

                                // read file and send to client
                                while ((bytes_read = fread(file_buf, 1, sizeof(file_buf), f)) > 0)
                                {
                                    int bytes_sent = 0;
                                    while (bytes_sent < bytes_read)
                                    {
                                        int ret = send(client, file_buf + bytes_sent, bytes_read - bytes_sent, 0);
                                        bytes_sent += ret;
                                    }
                                }

                                // close file
                                fclose(f);
                            }
                        }
                    }
                }
            }
            else
            {
                perror("Khong the mo thu muc");
                return 1;
            }

            close(client);
            exit(0);
        }

        close(client);
    }

    close(listener);

    return 0;
}