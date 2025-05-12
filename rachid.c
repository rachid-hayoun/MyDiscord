// client.c
#include <stdio.h>
#include <winsock2.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1234
#define BUFFER_SIZE 1024

int main() {
    WSADATA wsa;
    SOCKET sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int valread;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        printf("socket failed\n");
        WSACleanup();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("connect failed: %d\n", WSAGetLastError());
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    printf("Connecté au serveur. Tape un message et appuie sur Entrée.\n");

    while (1) {
        // Lire message utilisateur
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) break;

        // Envoyer au serveur
        send(sockfd, buffer, (int)strlen(buffer), 0);

        // Recevoir broadcast serveur
        valread = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
        if (valread <= 0) {
            printf("Connexion fermée par le serveur.\n");
            break;
        }
        buffer[valread] = '\0';
        printf("Message reçu: %s", buffer);
    }

    closesocket(sockfd);
    WSACleanup();
    return 0;
}
