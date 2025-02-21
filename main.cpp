#include <cstdio>
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

// Вспомогательная функция для вычисления длины строки
template<typename T>
size_t calculate_length(T arg) {
    return strlen(arg);
}

// Рекурсивная функция для вычисления общей длины всех строк
template<typename T, typename... Args>
size_t calculate_length(T first, Args... args) {
    return strlen(first) + calculate_length(args...);
}

// Базовый случай: когда аргументов больше нет, просто завершаем строку
void concatenate_impl(char* result) {
    // Ничего не делаем, просто завершаем рекурсию
}

char* mystrcat(char* dest, const char* src) {
     while (*dest) dest++;
     while (*dest++ = *src++);
     return --dest;
}

// Рекурсивная функция для конкатенации строк
template<typename T, typename... Args>
void concatenate_impl(char* result, T first, Args ... args) {
    result = mystrcat(result, first); // Добавляем текущую строку
    concatenate_impl(result, args...); // Рекурсивно обрабатываем оставшиеся аргументы
}

// Основная функция для конкатенации
template<typename... Args>
char* concatenate(Args... args) {
    // Вычисляем общую длину всех строк + 1 для нулевого символа
    size_t total_length = calculate_length(args...) + 1;
    // Выделяем память под результирующую строку
    char* result = new char[total_length];
    result[0] = '\0'; // Инициализируем пустую строку

    // Конкатенируем строки
    concatenate_impl(result, args...);

    return result;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s\n <q>", argv[0]);
        return 1;
    }
    struct ifaddrs *myaddrs;
    char buf[64], local_ip[16];
    local_ip[0] = '\0';
    if(getifaddrs(&myaddrs) != 0) {
        perror("getifaddrs");
        return 1;
    }
    for (auto ptr = myaddrs; ptr != NULL; ptr = ptr->ifa_next) {
        if (ptr->ifa_addr->sa_family == AF_INET) {   
            struct sockaddr_in *s4 = (struct sockaddr_in *)ptr->ifa_addr;
            if (inet_ntop(ptr->ifa_addr->sa_family, &s4->sin_addr, buf, sizeof(buf))) {
                if (strstr(buf, "192.168")) {
                    strncpy(local_ip, buf, 16);
                    break;
                }
            }
        }
    }
    freeifaddrs(myaddrs);
    if (local_ip[0] == '\0') {
        printf("No local IP found with prefix 192.168\n");
        return 1;
    }

    char old_ip[16];
    if (FILE *file = fopen("ip4.txt", "r")) {
        fgets(old_ip, 16, file);
        fclose(file);
    }

    if (strcmp(local_ip, old_ip) == 0) {
        printf("previous ip is equal to current\n");
        return 0;
    }

    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc < 0) {
        printf("failed to create socket\n");
        return 0;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(80);
    struct hostent *server = gethostbyname("ipv4.cloudns.net");
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    if (connect(socket_desc, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        printf("connection failed :(\n");
        close(socket_desc);
        return 0;
    }

    const auto request = concatenate("GET /api/dynamicURL/?ip=", local_ip, "&q=", argv[1], " HTTP/1.1\r\nHost: ipv4.cloudns.net\r\n\r\n");

    if (send(socket_desc, request, strlen(request) + 1, 0) < 0){
        printf("failed to send request...\n");
        close(socket_desc);
        return 0;
    }

    char buffer [256];
    int bytesrecv = recv(socket_desc, buffer, sizeof(buffer), 0);

    close(socket_desc);

    if (FILE *file = fopen("ip4.txt", "w")) {
        fprintf(file, local_ip);
        fclose(file);
    }

    return 0;
}