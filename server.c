#include <gtk/gtk.h>
#include <string.h>

typedef struct {
    GtkWidget *message_list;
    GtkWidget *entry;
    GHashTable *server_messages;
    GtkWidget *server_list;  // List of servers in sidebar
    GtkWidget *main_panel;
    char current_server[128];
} AppWidgets;

// Déclarations
void on_create_server(GtkButton *button, AppWidgets *widgets);
void on_server_selected(GtkButton *button, AppWidgets *widgets);
void on_send_message(GtkButton *button, AppWidgets *widgets);

// Créer un nouveau serveur
void on_create_server(GtkButton *button, AppWidgets *widgets) {
    static int server_count = 1;
    char server_name[128];
    snprintf(server_name, sizeof(server_name), "Serveur %d", server_count++);

    GtkWidget *btn = gtk_button_new_with_label(server_name);
    gtk_list_box_append(GTK_LIST_BOX(widgets->server_list), btn);

    g_signal_connect(btn, "clicked", G_CALLBACK(on_server_selected), widgets);

    GList *initial_messages = NULL;
    g_hash_table_insert(widgets->server_messages, g_strdup(server_name), initial_messages);

    strcpy(widgets->current_server, server_name);

    // Mettre à jour le titre du serveur dans le panneau principal
    gtk_label_set_text(GTK_LABEL(gtk_widget_get_first_child(widgets->main_panel)), server_name);
    gtk_list_box_remove_all(GTK_LIST_BOX(widgets->message_list));
}

// Sélection d’un serveur
void on_server_selected(GtkButton *button, AppWidgets *widgets) {
    const char *server_name = gtk_button_get_label(button);
    strcpy(widgets->current_server, server_name);

    gtk_label_set_text(GTK_LABEL(gtk_widget_get_first_child(widgets->main_panel)), server_name);
    gtk_list_box_remove_all(GTK_LIST_BOX(widgets->message_list));

    GList *messages = g_hash_table_lookup(widgets->server_messages, widgets->current_server);
    for (GList *l = messages; l != NULL; l = l->next) {
        GtkWidget *msg = gtk_label_new((char *)l->data);
        gtk_list_box_append(GTK_LIST_BOX(widgets->message_list), msg);
    }
}

// Envoi d’un message
void on_send_message(GtkButton *button, AppWidgets *widgets) {
    const char *text = gtk_editable_get_text(GTK_EDITABLE(widgets->entry));
    if (text[0] == '\0') return;

    GtkWidget *msg = gtk_label_new(text);
    gtk_list_box_append(GTK_LIST_BOX(widgets->message_list), msg);

    GList *messages = g_hash_table_lookup(widgets->server_messages, widgets->current_server);
    messages = g_list_append(messages, g_strdup(text));
    g_hash_table_replace(widgets->server_messages, g_strdup(widgets->current_server), messages);

    gtk_editable_set_text(GTK_EDITABLE(widgets->entry), "");
}

// Barre latérale avec serveurs
GtkWidget *create_sidebar(AppWidgets *widgets) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_size_request(box, 100, -1);
    gtk_widget_add_css_class(box, "sidebar");

    GtkWidget *create_btn = gtk_button_new_with_label("+ Créer serveur");
    gtk_box_append(GTK_BOX(box), create_btn);
    g_signal_connect(create_btn, "clicked", G_CALLBACK(on_create_server), widgets);

    widgets->server_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(widgets->server_list), GTK_SELECTION_NONE);
    gtk_box_append(GTK_BOX(box), widgets->server_list);

    return box;
}

// Panneau principal avec messages + entrée
GtkWidget *create_main_panel(AppWidgets *widgets) {
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(main_box, "main-content");

    GtkWidget *server_title = gtk_label_new("Aucun serveur");
    gtk_widget_set_halign(server_title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(main_box), server_title);

    widgets->message_list = gtk_list_box_new();
    gtk_widget_set_vexpand(widgets->message_list, TRUE);
    gtk_box_append(GTK_BOX(main_box), widgets->message_list);

    GtkWidget *entry_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    widgets->entry = gtk_entry_new();
    GtkWidget *send_btn = gtk_button_new_with_label("Envoyer");

    gtk_box_append(GTK_BOX(entry_box), widgets->entry);
    gtk_box_append(GTK_BOX(entry_box), send_btn);
    gtk_box_append(GTK_BOX(main_box), entry_box);

    g_signal_connect(send_btn, "clicked", G_CALLBACK(on_send_message), widgets);

    widgets->main_panel = main_box;
    return main_box;
}

static void activate(GtkApplication *app, gpointer user_data) {
    AppWidgets *widgets = g_malloc0(sizeof(AppWidgets));
    strcpy(widgets->current_server, "");

    widgets->server_messages = g_hash_table_new_full(
        g_str_hash, g_str_equal, g_free, (GDestroyNotify)g_list_free_full);

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "style.css");

    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Discord GTK Clone");
    gtk_window_set_default_size(GTK_WINDOW(window), 1280, 720);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), main_box);

    gtk_box_append(GTK_BOX(main_box), create_sidebar(widgets));
    GtkWidget *main_panel = create_main_panel(widgets);
    gtk_widget_set_hexpand(main_panel, TRUE);
    gtk_box_append(GTK_BOX(main_box), main_panel);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.discord.servers", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}