#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

typedef struct
{
    int client_src;
    int client_dst;
} client_params;

// Hàm gửi phản hồi HTTP về cho Client
void send_http_response(int client_sock, int status_code, const char *status_text, const char *html_content)
{
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

// Hàm xử lý yêu cầu HTTP của từng Client (Thread con)
void *handle_client(void *arg)
{
    client_params *params = (client_params *)arg;
    int client_sock = params->client_dst;

    // Giải phóng bọc params ngay sau khi lấy được socket, tránh memory leak
    free(params);

    char req_buf[2048] = {0};
    int bytes_received = recv(client_sock, req_buf, sizeof(req_buf) - 1, 0);

    if (bytes_received > 0)
    {
        // Phân tích dòng đầu tiên của HTTP Request (Ví dụ: "GET /me HTTP/1.1")
        char method[10] = {0};
        char url[100] = {0};
        sscanf(req_buf, "%s %s", method, url);

        printf("[Log] Request: %s %s\n", method, url);

        // Routing các Endpoint
        if (strcmp(url, "/me") == 0)
        {
            const char *me_html =
                "<!DOCTYPE html>"
                "<html>"
                "<head><title>Thông tin cá nhân</title></head>"
                "<body>"
                "  <h2>Thông tin bản thân</h2>"
                "  <p><b>Họ và tên:</b> Lưu Ngọc Lợi</p>"
                "  <p><b>MSSV:</b> 20225357</p>"
                "</body>"
                "</html>";
            send_http_response(client_sock, 200, "OK", me_html);
        }
        else if (strcmp(url, "/friends") == 0)
        {
            const char *friends_html =
                "<!DOCTYPE html>"
                "<html>"
                "<head><title>Danh sách bạn bè</title></head>"
                "<body>"
                "  <h2>Danh sách bạn bè xung quanh</h2>"
                "  <ul>"
                "    <li>"
                "      <h3>Bạn bên trái:</h3>"
                "      <p>Họ và tên: Phan Minh Vượng</p>"
                "      <p>MSSV: 20225241</p>"
                "    </li>"
                "    <li>"
                "      <h3>Bạn bên phải:</h3>"
                "      <p>Họ và tên: Nguyễn Khổng Duy Hoàng</p>"
                "      <p>MSSV: 20225130</p>"
                "    </li>"
                "    <li>"
                "      <h3>Bạn phía trước:</h3>"
                "      <p>Họ và tên: Trần Thanh Anh Tài</p>"
                "      <p>MSSV: 20225392</p>"
                "    </li>"
                "    <li>"
                "      <h3>Bạn phía sau:</h3>"
                "      <p>Họ và tên: Trịnh Hoàng Chi</p>"
                "      <p>MSSV: 20225169</p>"
                "    </li>"
                "  </ul>"
                "</body>"
                "</html>";
            send_http_response(client_sock, 200, "OK", friends_html);
        }
        else
        {
            // Không tìm thấy Endpoint tương ứng (404 Not Found)
            const char *not_found_json = "{\"error\": \"Endpoint not found.\"}";
            send_http_response(client_sock, 404, "Not Found", not_found_json);
        }
    }

    close(client_sock);
    return NULL;
}

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("Tạo socket thất bại");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind thất bại");
        return 1;
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("Listen thất bại");
        return 1;
    }

    printf("HTTP Server đang chạy tại cổng 8080...\n");

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

        if (client_fd >= 0)
        {
            // Đóng gói tham số vào struct client_params
            client_params *args = malloc(sizeof(client_params));
            args->client_src = server_fd;
            args->client_dst = client_fd;

            // Tạo thread con xử lý request của client độc lập
            pthread_t tid;
            if (pthread_create(&tid, NULL, handle_client, (void *)args) != 0)
            {
                perror("Tạo thread thất bại");
                close(client_fd);
                free(args);
            }
            else
            {
                // Tách luồng để hệ thống tự thu hồi tài nguyên khi thread kết thúc, không cần pthread_join
                pthread_detach(tid);
            }
        }
    }

    close(server_fd);
    return 0;
}