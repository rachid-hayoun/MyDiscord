// server.c
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 1234
#define MAX_CLIENTS  FD_SETSIZE
#define BUFFER_SIZE 1024

int main() {
    WSADATA wsa;
    SOCKET listen_sock, client_socks[MAX_CLIENTS];
    struct sockaddr_in server_addr;
    fd_set readfds, masterfds;
    int max_sd, activity, i, valread;
    char buffer[BUFFER_SIZE];

    // Initialisation Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    // Création socket d'écoute
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) {
        printf("socket failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Configuration adresse serveur
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind
    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("bind failed: %d\n", WSAGetLastError());
        closesocket(listen_sock);
        WSACleanup();
        return 1;
    }

    // Listen
    if (listen(listen_sock, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen failed: %d\n", WSAGetLastError());
        closesocket(listen_sock);
        WSACleanup();
        return 1;
    }

    printf("Serveur en écoute sur le port %d\n", PORT);

    // Initialiser tableau clients
    for (i = 0; i < MAX_CLIENTS; i++) client_socks[i] = 0;

    FD_ZERO(&masterfds);
    FD_SET(listen_sock, &masterfds);
    max_sd = listen_sock;

    while (1) {
        readfds = masterfds;

        activity = select(max_sd +1, &readfds, NULL, NULL, NULL);
        if (activity == SOCKET_ERROR) {
            printf("select error: %d\n", WSAGetLastError());
            break;
        }

        // Nouvelle connexion ?
        if (FD_ISSET(listen_sock, &readfds)) {
            SOCKET new_socket;
            struct sockaddr_in client_addr;
            int addrlen = sizeof(client_addr);

            new_socket = accept(listen_sock, (struct sockaddr *)&client_addr, &addrlen);
            if (new_socket == INVALID_SOCKET) {
                printf("accept failed: %d\n", WSAGetLastError());
                continue;
            }

            printf("Nouvelle connexion, socket fd: %d, IP: %s, Port: %d\n",
                (int)new_socket,
                inet_ntoa(client_addr.sin_addr),
                ntohs(client_addr.sin_port));

            // Ajouter nouveau socket dans tableau clients
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_socks[i] == 0) {
                    client_socks[i] = new_socket;
                    FD_SET(new_socket, &masterfds);
                    if (new_socket > max_sd) max_sd = new_socket;
                    break;
                }
            }
            if (i == MAX_CLIENTS) {
                printf("Nombre maximal de clients atteint, connexion refusée.\n");
                closesocket(new_socket);
            }
        }

        // Vérifier les IO sur les clients existants
        for (i = 0; i < MAX_CLIENTS; i++) {
            SOCKET sd = client_socks[i];
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                valread = recv(sd, buffer, BUFFER_SIZE -1, 0);
                if (valread <= 0) {
                    // Client déconnecté
                    printf("Client sur socket %d déconnecté.\n", (int)sd);
                    closesocket(sd);
                    FD_CLR(sd, &masterfds);
                    client_socks[i] = 0;
                } else {
                    buffer[valread] = '\0';
                    printf("Message reçu du client %d: %s\n", (int)sd, buffer);

                    // Diffuser à tous les clients sauf l'expéditeur
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        SOCKET out_sd = client_socks[j];
                        if (out_sd > 0 && out_sd != sd) {
                            send(out_sd, buffer, valread, 0);
                        }
                    }
                }
            }
        }
    }

    // Nettoyage
    closesocket(listen_sock);
    WSACleanup();
    return 0;
}
