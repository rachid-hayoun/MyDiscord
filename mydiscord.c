#include "mydiscord.h"
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <string.h>
#include <glib.h>
#include <time.h>
#include <libpq-fe.h>

#define PORT 1234
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

#define BG_COLOR "#5865F2"
#define DARK_BG_COLOR "#313338"
#define BUTTON_COLOR "#5865F2"
#define LINK_COLOR "#00AFF4"
#define BG_IMG "Fok.png"
#define HEADERBAR_COLOR "#1A237E"

SOCKET client_socks[MAX_CLIENTS];
GAsyncQueue *msg_queue;
fd_set masterfds;

GtkWidget *header_title = NULL;
GtkWidget *servers_container = NULL;
GtkWidget *chat_box = NULL;
GtkWidget *input_box = NULL;
GtkWidget *header_label = NULL;

int dynamic_server_count = 2;

Conversation conversations[MAX_CONVERSATIONS];
int conversation_count = 0;
Conversation *current_conversation = NULL;

// ==================== UTILITAIRES ====================

static const char* get_current_time() {
    time_t t;
    struct tm *tm_info;
    static char time_str[9];
    time(&t);
    tm_info = localtime(&t);
    strftime(time_str, 9, "%H:%M:%S", tm_info);
    return time_str;
}

static Conversation* get_conversation(const char *name) {
    for (int i = 0; i < conversation_count; i++) {
        if (strcmp(conversations[i].name, name) == 0) {
            return &conversations[i];
        }
    }
    if (conversation_count < MAX_CONVERSATIONS) {
        strncpy(conversations[conversation_count].name, name, sizeof(conversations[conversation_count].name) - 1);
        conversations[conversation_count].count = 0;
        return &conversations[conversation_count++];
    }
    return NULL;
}

static void refresh_chat() {
    while (gtk_widget_get_first_child(chat_box)) {
        gtk_widget_unparent(gtk_widget_get_first_child(chat_box));
    }
    if (!current_conversation) return;
    for (int i = 0; i < current_conversation->count; i++) {
        GtkWidget *label = gtk_label_new(NULL);
        char label_text[256];
        snprintf(label_text, sizeof(label_text), "[%s] %s: %s",
            current_conversation->messages[i].time,
            current_conversation->messages[i].user,
            current_conversation->messages[i].message);
        gtk_label_set_text(GTK_LABEL(label), label_text);
        if (strcmp(current_conversation->messages[i].user, "Moi") == 0) {
            gtk_widget_add_css_class(label, "mine");
        }
        gtk_box_append(GTK_BOX(chat_box), label);
    }
}

// ==================== SERVEUR ====================

gpointer server_thread(gpointer data) {
    WSADATA wsa;
    SOCKET listen_sock;
    struct sockaddr_in server_addr;
    fd_set readfds;
    int max_sd, activity, i, valread;
    char buffer[BUFFER_SIZE];

    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
        return NULL;

    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)); 
    listen(listen_sock, SOMAXCONN);// Permet d'écouter les sockets

    for (i = 0; i < MAX_CLIENTS; i++) client_socks[i] = 0;

    FD_ZERO(&masterfds);
    FD_SET(listen_sock, &masterfds);
    max_sd = listen_sock;

    while (1) {
        readfds = masterfds;
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity == SOCKET_ERROR) break;

        // Nouvelle connexion
        if (FD_ISSET(listen_sock, &readfds)) {
            SOCKET new_socket;
            struct sockaddr_in client_addr;
            int addrlen = sizeof(client_addr);
            new_socket = accept(listen_sock, (struct sockaddr *)&client_addr, &addrlen);
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_socks[i] == 0) {
                    client_socks[i] = new_socket;
                    FD_SET(new_socket, &masterfds);
                    if (new_socket > max_sd) max_sd = new_socket;
                    g_async_queue_push(msg_queue, g_strdup_printf("Nouveau client connecté : %d", (int)new_socket));
                    break;
                }
            }
        }
        // Messages des clients
        for (i = 0; i < MAX_CLIENTS; i++) {
            SOCKET sd = client_socks[i];
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                valread = recv(sd, buffer, BUFFER_SIZE - 1, 0);
                if (valread <= 0) {
                    closesocket(sd);
                    FD_CLR(sd, &masterfds);
                    client_socks[i] = 0;
                    g_async_queue_push(msg_queue, g_strdup_printf("Client %d déconnecté.", (int)sd));
                } else {
                    buffer[valread] = '\0';
                    g_async_queue_push(msg_queue, g_strdup_printf("Client %d: %s", (int)sd, buffer));
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
    closesocket(listen_sock);
    WSACleanup();
    return NULL;
}

// ==================== INTERFACE GTK ====================

void insert_message_into_db(const char *content, const char *user, const char *time) {
    PGconn *conn = PQconnectdb("dbname=mydiscord user=postgres password=rachidsql host=localhost");
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Connexion à la base échouée: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        return;
    }

    const char *query = "INSERT INTO messages (content, created_at, time) VALUES ($1, $2, $3);";
    const char *paramValues[3];
    paramValues[0] = content;
    paramValues[1] = user;
    paramValues[2] = time;

    PGresult *res = PQexecParams(conn, query, 3, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Échec de l'insertion : %s\n", PQerrorMessage(conn));
    }

    PQclear(res);
    PQfinish(conn);
}

static void send_message(GtkWidget *widget, gpointer data) {
    const char *msg = gtk_editable_get_text(GTK_EDITABLE(input_box));
    const char *user = "Moi"; 
    const char *time = get_current_time();
    insert_message_into_db(msg, user, time);

    for(int i=0; i<MAX_CLIENTS; i++) {
        if(client_socks[i] > 0) {
            send(client_socks[i], msg, strlen(msg), 0);
        }
    }
    if(current_conversation && current_conversation->count < MAX_MESSAGES) {
        strncpy(current_conversation->messages[current_conversation->count].message, msg, 255);
        strncpy(current_conversation->messages[current_conversation->count].user, "Moi", 31);
        strncpy(current_conversation->messages[current_conversation->count].time, get_current_time(), 8);
        current_conversation->count++;
    }
    gtk_editable_set_text(GTK_EDITABLE(input_box), "");
    refresh_chat();
}

static void on_conversation_selected(GtkWidget *widget, gpointer data) {
    const char *name = (const char *)data;
    char title[64];
    snprintf(title, sizeof(title), "Discussion: %s", name);
    gtk_label_set_text(GTK_LABEL(header_title), title);
    current_conversation = get_conversation(name);
    refresh_chat();
}

static void on_server_button_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *channels_box = GTK_WIDGET(data);
    gboolean is_visible = gtk_widget_get_visible(channels_box);
    gtk_widget_set_visible(channels_box, !is_visible);
    gtk_widget_queue_resize(channels_box);
}

static void remove_server(GtkWidget *widget, gpointer user_data) {
    GtkWidget *server_box = GTK_WIDGET(user_data);
    GtkWidget *parent = gtk_widget_get_parent(server_box);
    if (parent) {
        gtk_box_remove(GTK_BOX(parent), server_box);
    }
}

static void add_new_server(GtkWidget *button, gpointer user_data) {
    dynamic_server_count++;
    char server_name[32];
    snprintf(server_name, sizeof(server_name), "Serveur%d", dynamic_server_count);
    GtkWidget *server_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_margin_bottom(server_box, 10);

    GtkWidget *server_button = gtk_button_new_with_label("Nouveau Serveur");
    gtk_widget_add_css_class(server_button, "square-button");
    gtk_widget_set_margin_start(server_button, 20);
    gtk_box_append(GTK_BOX(server_box), server_button);

    GtkWidget *channels_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_visible(channels_box, FALSE);
    const char *channels[] = {"#Général", "#Code", "#Mèmes"};
    char full_name[128];
    for (int i = 0; i < 3; i++) {
        snprintf(full_name, sizeof(full_name), "%s-%s", server_name, channels[i]);
        GtkWidget *btn = gtk_button_new_with_label(channels[i]);
        gtk_widget_add_css_class(btn, "square-button");
        gtk_widget_set_margin_start(btn, 20);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_conversation_selected), g_strdup(full_name));
        gtk_box_append(GTK_BOX(channels_box), btn);
    }
    gtk_box_append(GTK_BOX(server_box), channels_box);
    g_signal_connect(server_button, "clicked", G_CALLBACK(on_server_button_clicked), channels_box);

    GtkWidget *delete_button = gtk_button_new_with_label("Supprimer");
    gtk_widget_add_css_class(delete_button, "square-button");
    gtk_widget_set_margin_start(delete_button, 20);
    gtk_widget_set_tooltip_text(delete_button, "Supprimer le serveur");
    g_signal_connect(delete_button, "clicked", G_CALLBACK(remove_server), server_box);
    gtk_box_append(GTK_BOX(server_box), delete_button);

    gtk_box_append(GTK_BOX(servers_container), server_box);
}

static GtkWidget* create_friends_sidebar(void) {
    GtkWidget *friends_sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *friends_label = gtk_label_new("Amis");
    gtk_widget_add_css_class(friends_label, "sidebar-title");
    gtk_box_append(GTK_BOX(friends_sidebar), friends_label);

    const char *friends[] = {"Zaky", "Rachid", "Djacem"};
    char full_name[128];
    for (int i = 0; i < 3; i++) {
        snprintf(full_name, sizeof(full_name), "Ami:%s", friends[i]);
        GtkWidget *friend_button = gtk_button_new_with_label(friends[i]);
        gtk_widget_set_margin_end(friend_button, 20);
        gtk_widget_set_tooltip_text(friend_button, friends[i]);
        g_signal_connect(friend_button, "clicked", G_CALLBACK(on_conversation_selected), g_strdup(full_name));
        gtk_box_append(GTK_BOX(friends_sidebar), friend_button);
    }
    return friends_sidebar;
}

static GtkWidget* create_sidebar(void) {
    GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_size_request(sidebar, 200, -1);

    GtkWidget *servers_label = gtk_label_new("Serveurs");
    gtk_widget_add_css_class(servers_label, "sidebar-title");
    gtk_box_append(GTK_BOX(sidebar), servers_label);

    servers_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_append(GTK_BOX(sidebar), servers_container);

    GtkWidget *add_server_button = gtk_button_new_with_label("+ Nouveau Serveur");
    gtk_widget_add_css_class(add_server_button, "square-button");
    gtk_widget_set_margin_start(add_server_button, 20);
    gtk_box_append(GTK_BOX(sidebar), add_server_button);
    g_signal_connect(add_server_button, "clicked", G_CALLBACK(add_new_server), NULL);

    return sidebar;
}

// ==================== MISE À JOUR DES MESSAGES ====================

gboolean refresh_messages() {
    while (g_async_queue_length(msg_queue) > 0) {
        char *msg = g_async_queue_pop(msg_queue);
        Conversation *c = get_conversation("Serveur");
        if (c && c->count < MAX_MESSAGES) {
            strncpy(c->messages[c->count].message, msg, 255);
            strncpy(c->messages[c->count].user, "Serveur", 31);
            strncpy(c->messages[c->count].time, get_current_time(), 8);
            c->count++;
        }
        g_free(msg);
        refresh_chat();
    }
    return G_SOURCE_CONTINUE;
}

// ==================== MAIN GTK INTERFACE ====================

void build_mydiscord_interface(GtkWindow *window) {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider,
        "window { background-color: " BG_COLOR "; }"
        "headerbar { min-height: 24px; padding: 0; }"
        "headerbar * { margin: 0; padding: 0; }"
        "headerbar.titlebar { padding-top: 2px; padding-bottom: 2px; }"
        "headerbar button.titlebutton { min-height: 24px; min-width: 24px; }"
        ".login-box, .register-box { background-color: " DARK_BG_COLOR "; border-radius: 15px; padding: 32px; }"
        ".title { color: white; font-size: 24px; font-weight: bold; margin-bottom: 8px; }"
        ".subtitle { color: #B9BBBE; font-size: 16px; margin-bottom: 20px; }"
        ".input-label { color: #B9BBBE; font-size: 12px; font-weight: bold; margin-bottom: 8px; }"
        "entry { background-color: #1E1F22; color: white; border-radius: 3px; margin-bottom: 16px; padding: 10px; }"
        ".login-button, .register-button { background-color: " BUTTON_COLOR "; color: black; border-radius: 3px; padding: 10px; }"
        ".link { color: " LINK_COLOR "; font-size: 14px; }"
        ".linkr { color: " LINK_COLOR "; font-size: 14px; }"
        ".date-dropdown { background-color: #1E1F22; color: white; border-radius: 3px; min-height: 50px; min-width: 120px; }"
        "checkbutton { color: white; }"
        "checkbutton check { background-color: #000000; border: 1px solid #72767d; border-radius: 3px; }"
        "checkbutton:checked check { background-color: " BUTTON_COLOR "; border: none; }"
        ".left-button { margin-left: 20px; }"
        ".right-button { margin-right: 20px; }"
    );
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    header_title = gtk_label_new("MyDiscord");
    GtkWidget *header_bar = gtk_header_bar_new();
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header_bar), header_title);
    gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(header_bar), TRUE);
    gtk_window_set_titlebar(GTK_WINDOW(window), header_bar);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_window_set_child(window, main_box);

    // Serveurs et channels
    GtkWidget *sidebar = create_sidebar();
    gtk_box_append(GTK_BOX(main_box), sidebar);

    // Fil de discussion
    GtkWidget *chat_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_hexpand(chat_container, TRUE);
    gtk_box_append(GTK_BOX(main_box), chat_container);

    chat_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_vexpand(chat_box, TRUE);
    gtk_box_append(GTK_BOX(chat_container), chat_box);

    // Entry et bouton Valider
    GtkWidget *input_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_append(GTK_BOX(chat_container), input_container);

    GtkSizeGroup *size_group = gtk_size_group_new(GTK_SIZE_GROUP_VERTICAL);

    input_box = gtk_entry_new();
    gtk_widget_set_hexpand(input_box, TRUE);
    gtk_size_group_add_widget(size_group, input_box);
    gtk_box_append(GTK_BOX(input_container), input_box);

    GtkWidget *send_button = gtk_button_new_with_label("Envoyer");
    gtk_widget_add_css_class(send_button, "square-buttons");
    gtk_size_group_add_widget(size_group, send_button);
    gtk_box_append(GTK_BOX(input_container), send_button);
    g_signal_connect(send_button, "clicked", G_CALLBACK(send_message), NULL);

    // Amis
    GtkWidget *friends_sidebar = create_friends_sidebar();
    gtk_box_append(GTK_BOX(main_box), friends_sidebar);

    msg_queue = g_async_queue_new();
    g_thread_new("server", server_thread, NULL);
    g_timeout_add(100, (GSourceFunc)refresh_messages, NULL);
}

// ==================== MAIN ====================

void start_server() {
    GtkWindow *window = GTK_WINDOW(gtk_window_new());
    gtk_window_set_default_size(window, 900, 600);
    build_mydiscord_interface(window);
    gtk_window_present(window);

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
    g_main_loop_unref(loop);
}