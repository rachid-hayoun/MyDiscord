#include <gtk/gtk.h>

GtkWidget *create_sidebar() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_size_request(box, 72, -1);
    gtk_widget_add_css_class(box, "sidebar");

    GtkWidget *discord_btn = gtk_button_new();
    GtkWidget *discord_img = gtk_image_new_from_file("discord.png");
    gtk_button_set_child(GTK_BUTTON(discord_btn), discord_img);
    gtk_box_append(GTK_BOX(box), discord_btn);

    GtkWidget *add_btn = gtk_button_new();
    GtkWidget *add_img = gtk_image_new_from_file("add.png");
    gtk_button_set_child(GTK_BUTTON(add_btn), add_img);
    gtk_box_append(GTK_BOX(box), add_btn);

    return box;
}

GtkWidget *create_left_panel() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_size_request(box, 200, -1);
    gtk_widget_add_css_class(box, "navbar");

    GtkWidget *friends = gtk_button_new_with_label("Amis");
    gtk_box_append(GTK_BOX(box), friends);

    // Suppression de la boucle pour créer les 9 amis
    // Le panneau "Amis" est maintenant vide à part le bouton "Amis"

    return box;
}

GtkWidget *create_main_panel() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(box, "main-content");

    GtkWidget *label = gtk_label_new("Ajouter");
    gtk_box_append(GTK_BOX(box), label);

    GtkWidget *subtitle = gtk_label_new("Tu peux ajouter des amis grâce à leurs noms d'utilisateur Discord.");
    gtk_label_set_wrap(GTK_LABEL(subtitle), TRUE);
    gtk_box_append(GTK_BOX(box), subtitle);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Nom d'utilisateur Discord");
    gtk_widget_add_css_class(entry, "entry");
    gtk_box_append(GTK_BOX(box), entry);

    GtkWidget *add_btn = gtk_button_new_with_label("Envoyer une demande d'ami");
    gtk_widget_add_css_class(add_btn, "button");
    gtk_box_append(GTK_BOX(box), add_btn);

    return box;
}

GtkWidget *create_right_panel() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_size_request(box, 200, -1);
    gtk_widget_add_css_class(box, "right-panel");

    GtkWidget *label = gtk_label_new("En ligne");
    gtk_widget_add_css_class(label, "title");
    gtk_box_append(GTK_BOX(box), label);

    const char *users[][3] = {
        {"Alice", "Alice.png", "online"},
        {"Bob",   "Bob.png", "idle"},
        {"Eve",   "Eve.png", "dnd"}
    };

    for (int i = 0; i < 3; i++) {
        GtkWidget *user_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_widget_add_css_class(user_box, "user");

        GtkWidget *avatar = gtk_image_new_from_file(users[i][1]);
        gtk_widget_set_size_request(avatar, 32, 32);
        gtk_widget_add_css_class(avatar, "avatar");

        GtkWidget *name = gtk_label_new(users[i][0]);
        gtk_widget_add_css_class(name, users[i][2]);  // Classe CSS : "online", "idle", "dnd"
        gtk_widget_set_halign(name, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(name, GTK_ALIGN_CENTER);

        gtk_box_append(GTK_BOX(user_box), avatar);
        gtk_box_append(GTK_BOX(user_box), name);
        gtk_box_append(GTK_BOX(box), user_box);
    }

    return box;
}

static void activate(GtkApplication *app, gpointer user_data) {
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

    GtkWidget *sidebar = create_sidebar();
    gtk_widget_set_hexpand(sidebar, FALSE);
    gtk_box_append(GTK_BOX(main_box), sidebar);

    GtkWidget *left_panel = create_left_panel();
    gtk_widget_set_hexpand(left_panel, FALSE);
    gtk_box_append(GTK_BOX(main_box), left_panel);

    GtkWidget *main_panel = create_main_panel();
    gtk_widget_set_hexpand(main_panel, TRUE); // Permet à ce panneau de s'étendre
    gtk_box_append(GTK_BOX(main_box), main_panel);

    GtkWidget *right_panel = create_right_panel();
    gtk_widget_set_hexpand(right_panel, FALSE); // Ne s'étend pas, reste à droite
    gtk_widget_set_valign(right_panel, GTK_ALIGN_FILL);
    gtk_box_append(GTK_BOX(main_box), right_panel);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.discord.clone", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
