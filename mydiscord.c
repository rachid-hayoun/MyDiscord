#include <gtk/gtk.h>
#include <time.h>
#include <string.h>

#define MAX_MESSAGES 100
#define MAX_CONVERSATIONS 50

typedef struct {
    const char *user;
    const char *message;
    const char *time;
} ChatMessage;

typedef struct {
    char name[64];
    ChatMessage messages[MAX_MESSAGES];
    int count;
} Conversation;

Conversation conversations[MAX_CONVERSATIONS];
int conversation_count = 0;
Conversation *current_conversation = NULL;

GtkWidget *chat_box;
GtkWidget *input_box;
GtkWidget *header_label;
GtkWidget *server_button_1;
GtkWidget *server_button_2;

const char* get_current_time() {
    time_t t;
    struct tm *tm_info;
    static char time_str[9];
    time(&t);
    tm_info = localtime(&t);
    strftime(time_str, 9, "%H:%M:%S", tm_info);
    return time_str;
}

Conversation* get_conversation(const char *name) {
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
        gtk_box_append(GTK_BOX(chat_box), label);
    }
}

static void send_message(GtkWidget *widget, gpointer data) {
    const char *message_text = gtk_editable_get_text(GTK_EDITABLE(input_box));
    if (!current_conversation || current_conversation->count >= MAX_MESSAGES || g_strcmp0(message_text, "") == 0) return;

    current_conversation->messages[current_conversation->count].user = "Moi";
    current_conversation->messages[current_conversation->count].message = g_strdup(message_text);
    current_conversation->messages[current_conversation->count].time = g_strdup(get_current_time());
    current_conversation->count++;

    gtk_editable_set_text(GTK_EDITABLE(input_box), "");
    refresh_chat();
}

static void on_conversation_selected(GtkWidget *widget, gpointer data) {
    const char *name = (const char *)data;
    char title[64];
    snprintf(title, sizeof(title), "Discussion: %s", name);
    gtk_label_set_text(GTK_LABEL(header_label), title);

    current_conversation = get_conversation(name);
    refresh_chat();
}

static void on_server_button_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *channels_box = GTK_WIDGET(data);
    if (!GTK_IS_WIDGET(channels_box)) {
        g_warning("channels_box n'est pas un widget valide.");
        return;
    }
    gboolean is_visible = gtk_widget_get_visible(channels_box);
    gtk_widget_set_visible(channels_box, !is_visible);
    gtk_widget_queue_resize(channels_box);
}

static GtkWidget* create_sidebar() {
    GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_size_request(sidebar, 200, -1);

    GtkWidget *servers_label = gtk_label_new("Serveurs");
    gtk_box_append(GTK_BOX(sidebar), servers_label);

    const char *channels[] = {"#Général", "#Code", "#Mèmes"};
    char full_name[128];

    // Serveur 1
    GtkWidget *server_box_1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    server_button_1 = gtk_button_new();
    gtk_widget_add_css_class(server_button_1, "round-server");
    GtkWidget *server_icon_1 = gtk_image_new_from_file("server_icon.png");
    gtk_button_set_child(GTK_BUTTON(server_button_1), server_icon_1);
    gtk_box_append(GTK_BOX(server_box_1), server_button_1);

    GtkWidget *channels_box_1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_visible(channels_box_1, FALSE);

    for (int i = 0; i < 3; i++) {
        snprintf(full_name, sizeof(full_name), "Serveur1-%s", channels[i]);
        GtkWidget *btn = gtk_button_new_with_label(channels[i]);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_conversation_selected), g_strdup(full_name));
        gtk_box_append(GTK_BOX(channels_box_1), btn);
    }
    gtk_box_append(GTK_BOX(server_box_1), channels_box_1);

    // Serveur 2
    GtkWidget *server_box_2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    server_button_2 = gtk_button_new();
    gtk_widget_add_css_class(server_button_2, "round-server");
    GtkWidget *server_icon_2 = gtk_image_new_from_file("server_icon.png");
    gtk_button_set_child(GTK_BUTTON(server_button_2), server_icon_2);
    gtk_box_append(GTK_BOX(server_box_2), server_button_2);

    GtkWidget *channels_box_2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_visible(channels_box_2, FALSE);

    for (int i = 0; i < 3; i++) {
        snprintf(full_name, sizeof(full_name), "Serveur2-%s", channels[i]);
        GtkWidget *btn = gtk_button_new_with_label(channels[i]);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_conversation_selected), g_strdup(full_name));
        gtk_box_append(GTK_BOX(channels_box_2), btn);
    }
    gtk_box_append(GTK_BOX(server_box_2), channels_box_2);

    g_signal_connect(server_button_1, "clicked", G_CALLBACK(on_server_button_clicked), channels_box_1);
    g_signal_connect(server_button_2, "clicked", G_CALLBACK(on_server_button_clicked), channels_box_2);

    gtk_box_append(GTK_BOX(sidebar), server_box_1);
    gtk_box_append(GTK_BOX(sidebar), server_box_2);

    // ---- Amis ----
    GtkWidget *friends_label = gtk_label_new("Amis");
    gtk_box_append(GTK_BOX(sidebar), friends_label);

    const char *friends[] = {"Alice", "Bob", "Charlie"};
    for (int i = 0; i < 3; i++) {
        snprintf(full_name, sizeof(full_name), "Ami:%s", friends[i]);

        GtkWidget *friend_button = gtk_button_new();
        gtk_widget_add_css_class(friend_button, "round-friend");

        char image_path[64];
        snprintf(image_path, sizeof(image_path), "%s.png", friends[i]);
        GtkWidget *friend_icon = gtk_image_new_from_file(image_path);
        gtk_button_set_child(GTK_BUTTON(friend_button), friend_icon);

        gtk_widget_set_tooltip_text(friend_button, friends[i]);

        g_signal_connect(friend_button, "clicked", G_CALLBACK(on_conversation_selected), g_strdup(full_name));
        gtk_box_append(GTK_BOX(sidebar), friend_button);
    }

    return sidebar;
}

static void create_main_window(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "MyDiscord");
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 700);

    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(css_provider, "style.css");
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), main_box);

    GtkWidget *sidebar = create_sidebar();
    gtk_box_append(GTK_BOX(main_box), sidebar);

    GtkWidget *chat_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_hexpand(chat_container, TRUE);
    gtk_box_append(GTK_BOX(main_box), chat_container);

    header_label = gtk_label_new("Bienvenue sur MyDiscord");
    gtk_box_append(GTK_BOX(chat_container), header_label);

    chat_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_vexpand(chat_box, TRUE);
    gtk_box_append(GTK_BOX(chat_container), chat_box);

    input_box = gtk_entry_new();
    gtk_box_append(GTK_BOX(chat_container), input_box);

    GtkWidget *send_button = gtk_button_new_with_label("Envoyer");
    g_signal_connect(send_button, "clicked", G_CALLBACK(send_message), NULL);
    gtk_box_append(GTK_BOX(chat_container), send_button);

    gtk_window_present(GTK_WINDOW(window));
}

static void activate(GtkApplication *app, gpointer user_data) {
    create_main_window(app, user_data);
}

int main(int argc, char *argv[]) {
    GtkApplication *app = gtk_application_new("com.example.MyDiscordGTK4", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}