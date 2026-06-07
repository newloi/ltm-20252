#include <arpa/inet.h>
#include <dirent.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// send response to client
void send_http_response(int client_sock, int status_code, const char* status_text, const char* html_content) {
    char header[512];
    int header_len = sprintf(header,
                             "HTTP/1.1 %d %s\r\n"
                             "Content-Type: text/html; charset=utf-8\r\n"
                             "Content-Length: %ld\r\n"
                             "Connection: close\r\n"
                             "\r\n",
                             status_code, status_text, strlen(html_content));
    send(client_sock, header, header_len, 0);
    send(client_sock, html_content, strlen(html_content), 0);
}

// send list file and child folder of dir_path
void send_folder(int client_sock, const char* dir_path, const char* endpoint) {
    DIR* dir = opendir(dir_path);
    if (dir == NULL) {
        send_http_response(client_sock, 500, "Internal Server Error", 
            "<html><body><h2>Không thể mở thư mục</h2></body></html>");
        return;
    }

    char html[8192];
    sprintf(html, 
        "<html><head><meta charset=\"utf-8\"><title>Thư mục</title></head><body><h2>Danh sách file và folder trong %s:</h2><ul>", dir_path);
    struct dirent* entry;

    // get list file and folder
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {
            char item[1024];
            if (entry->d_type == DT_DIR) { // folder
                sprintf(item, "<li><b><a href=\"%s/%s\">%s</a></b></li>", endpoint, entry->d_name, entry->d_name);
            } else { // file
                sprintf(item, "<li><i><a href=\"%s/%s\">%s</a></i></li>", endpoint, entry->d_name, entry->d_name);
            }
            strcat(html, item);
        }
    }
    strcat(html, "</ul></body></html>");
    closedir(dir);

    send_http_response(client_sock, 200, "OK", html);
}

// send content of file
void send_file(int client_sock, const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (file == NULL) {
        send_http_response(client_sock, 404, "Not Found", "<html><body><h2>404 File Not Found</h2></body></html>");
        return;
    }

    // get file size (byte)
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // check file type (audio, image, video, text)
    const char* content_type = "application/octet-stream";
    if (strstr(filepath, ".html") != NULL) content_type = "text/html";
    else if (strstr(filepath, ".txt") != NULL) content_type = "text/plain";
    else if (strstr(filepath, ".mp4") != NULL) content_type = "video/mp4";
    else if (strstr(filepath, ".jpg") != NULL || strstr(filepath, ".jpeg") != NULL) content_type = "image/jpeg";
    else if (strstr(filepath, ".png") != NULL) content_type = "image/png";
    else if (strstr(filepath, ".mp3") != NULL) content_type = "audio/mpeg";

    char header[512];
    int header_len = sprintf(header,
                             "HTTP/1.1 200 OK\r\n"
                             "Content-Type: %s; charset=utf-8\r\n"
                             "Content-Length: %ld\r\n"
                             "Connection: close\r\n"
                             "\r\n",
                             content_type, file_size);
    send(client_sock, header, header_len, 0);

    // send content of file by chunk 4096 bytes
    char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        // Sử dụng MSG_NOSIGNAL để tránh sinh tín hiệu SIGPIPE khi client đóng kết nối nửa chừng
        if (send(client_sock, buffer, bytes_read, MSG_NOSIGNAL) < 0) {
            break; // Ngừng gửi dữ liệu nếu client đã ngắt kết nối
        }
    }

    fclose(file);
}

void* client_thread(void* params) {
    int client = *(int*)params;
    printf("New request from client: %d\n", client);

    char request[1024];
    int ret = recv(client, request, sizeof(request), 0);
    if (ret > 0) {
        request[ret] = '\0';
        char method[10];
        char endpoint[100];
        sscanf(request, "%s %s", method, endpoint);  // get method and endpoint
        printf("Request received: %s %s\n", method, endpoint);

        if (strcmp(method, "GET") == 0) {
            char filepath[512];
            sprintf(filepath, ".%s", endpoint);

            // check filepath is folder or file
            struct stat path_stat;
            if (stat(filepath, &path_stat) == 0) {
                if (S_ISDIR(path_stat.st_mode)) { // filepath is folder
                    send_folder(client, filepath, endpoint);
                } else { // filepath is file
                    send_file(client, filepath);
                }
            } else { // filepath not found
                send_http_response(client, 404, "Not Found", "<html><body><h2>404 File Not Found</h2></body></html>");
            }
        }
    }

    close(client);
    return NULL;
}

int main() {
    // Bỏ qua tín hiệu SIGPIPE để tránh server bị tắt đột ngột khi trình duyệt đóng socket nửa chừng
    signal(SIGPIPE, SIG_IGN);

    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == -1) {
        perror("socket() failed");
        exit(1);
    }

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);
    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr))) {
        perror("bind() failed");
        close(listener);
        exit(1);
    }

    if (listen(listener, 5)) {
        perror("listen() failed");
        close(listener);
        exit(1);
    }

    printf("Server is listening on port 9000...\n");

    while (1) {
        int client = accept(listener, NULL, NULL);
        // create a new thread for new client
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, client_thread, &client);
        pthread_detach(thread_id);
    }

    close(listener);
    return 0;
}