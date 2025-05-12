#include <gtk/gtk.h>
#include <libpq-fe.h>
#include <string.h>
#include "login.h"
#include "channel/mydiscord.h"
#include <openssl/sha.h>

#define BG_COLOR "#5865F2"
#define DARK_BG_COLOR "#313338"
#define BUTTON_COLOR "#5865F2"
#define LINK_COLOR "#00AFF4"
#define BG_IMG "Fok.png"
#define HEADERBAR_COLOR "#1A237E"

void on_login_success(AppWidgets *app_widgets) {
    GtkWidget *child = gtk_window_get_child(app_widgets->parent_window);
    if (child) gtk_window_set_child(app_widgets->parent_window, NULL);
    build_mydiscord_interface(GTK_WINDOW(app_widgets->parent_window));
}

void hash_password(const char *password, char *output) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)password, strlen(password), hash);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
    output[64] = 0;
}

void show_error_dialog(GtkWindow *parent, const char *message) {
    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Erreur");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 100);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(box, 10);
    gtk_widget_set_margin_bottom(box, 10);
    gtk_widget_set_margin_start(box, 10);
    gtk_widget_set_margin_end(box, 10);
    GtkWidget *label = gtk_label_new(message);
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_box_append(GTK_BOX(box), label);
    GtkWidget *button = gtk_button_new_with_label("OK");
    gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), button);
    g_signal_connect(button, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
    gtk_window_set_child(GTK_WINDOW(dialog), box);
    gtk_window_present(GTK_WINDOW(dialog));
}

static void on_login_button_clicked(GtkButton *button, gpointer user_data) {
    AppWidgets *app_widgets = (AppWidgets *)user_data;
    const char *email = gtk_editable_get_text(GTK_EDITABLE(app_widgets->email_entry));
    const char *password = gtk_editable_get_text(GTK_EDITABLE(app_widgets->password_entry));
    if (strlen(email) == 0) {
        show_error_dialog(app_widgets->parent_window, "L'adresse email ou le numéro est obligatoire.");
        return;
    }
    if (strlen(password) == 0) {
        show_error_dialog(app_widgets->parent_window, "Le mot de passe est obligatoire.");
        return;
    }
    PGconn *conn = PQconnectdb("host=localhost port=5432 dbname=mydiscord user=postgres password=rachidsql");
    if (PQstatus(conn) == CONNECTION_BAD) {
        show_error_dialog(app_widgets->parent_window, "Erreur de connexion à la base de données. Veuillez réessayer plus tard.");
        PQfinish(conn);
        return;
    }
    const char *paramValues[2] = {email, password};
    PGresult *res = PQexecParams(conn,
        "SELECT id, nom, prenom FROM public.utilisateur WHERE email = $1 AND mot_de_passe_chiffre = $2",
        2, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        char error_message[512];
        snprintf(error_message, sizeof(error_message),
            "Erreur lors de la vérification des identifiants: %s",
            PQresultErrorMessage(res));
        show_error_dialog(app_widgets->parent_window, error_message);
        PQclear(res);
        PQfinish(conn);
        return;
    }
    if (PQntuples(res) == 0) {
        show_error_dialog(app_widgets->parent_window, "Identifiants incorrects. Veuillez vérifier votre email et mot de passe.");
        PQclear(res);
        PQfinish(conn);
        return;
    }
    const char *user_id = PQgetvalue(res, 0, 0);
    const char *nom = PQgetvalue(res, 0, 1);
    const char *prenom = PQgetvalue(res, 0, 2);
    if (g_strcmp0(prenom, "Rachid") == 0) {
        system("start cmd /c rachid.exe");
    }
    
    PQclear(res);
    const char *updateParamValues[1] = {user_id};
    PGresult *update_res = PQexecParams(conn,
        "UPDATE public.utilisateur SET est_connecte = TRUE, derniere_connexion = NOW() WHERE id = $1::integer",
        1, NULL, updateParamValues, NULL, NULL, 0);
    if (PQresultStatus(update_res) != PGRES_COMMAND_OK) {
        g_print("Erreur lors de la mise à jour du statut de connexion: %s\n", PQresultErrorMessage(update_res));
    }
    PQclear(update_res);
    PQfinish(conn);
    gtk_editable_set_text(GTK_EDITABLE(app_widgets->email_entry), "");
    gtk_editable_set_text(GTK_EDITABLE(app_widgets->password_entry), "");
    g_print("✅ Connexion réussie pour l'utilisateur %s %s (ID: %s)\n", nom, prenom, user_id);
    on_login_success(app_widgets);
}

static void show_login_view(GtkWidget *widget, gpointer user_data) {
    AppWidgets *app_widgets = (AppWidgets *)user_data;
    
    gtk_label_set_text(GTK_LABEL(app_widgets->header_title), "Connexion");
    
    // Cache la vue d'inscription et affiche la vue de connexion
    gtk_widget_set_visible(app_widgets->register_box, FALSE);
    gtk_widget_set_visible(app_widgets->login_box, TRUE);
}

static void show_register_view(GtkWidget *widget, gpointer user_data) {
    AppWidgets *app_widgets = (AppWidgets *)user_data;
    
    // Change le titre de la barre d'en-tête
    gtk_label_set_text(GTK_LABEL(app_widgets->header_title), "Inscription");
    
    // Cache la vue de connexion et affiche la vue d'inscription
    gtk_widget_set_visible(app_widgets->login_box, FALSE);
    gtk_widget_set_visible(app_widgets->register_box, TRUE);
}

static void on_activate(GtkApplication *app, gpointer user_data)
{
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_default_size(GTK_WINDOW(window), 1550, 900);

    AppWidgets *app_widgets = g_new(AppWidgets, 1);
    app_widgets->parent_window = GTK_WINDOW(window);

    // Créer la barre d'en-tête
    GtkWidget *header_bar = gtk_header_bar_new();
    gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(header_bar), TRUE);
    app_widgets->header_title = gtk_label_new("Connexion");
    gtk_widget_set_valign(app_widgets->header_title, GTK_ALIGN_CENTER);
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header_bar), app_widgets->header_title);
    gtk_window_set_titlebar(GTK_WINDOW(window), header_bar);

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
    );

    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    GtkWidget *overlay = gtk_overlay_new();

    GtkWidget *bg_image = gtk_picture_new_for_filename(BG_IMG);
    gtk_picture_set_content_fit(GTK_PICTURE(bg_image), GTK_CONTENT_FIT_FILL);
    gtk_picture_set_can_shrink(GTK_PICTURE(bg_image), TRUE);
    gtk_widget_set_hexpand(bg_image, TRUE);
    gtk_widget_set_vexpand(bg_image, TRUE);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(main_box, TRUE);
    gtk_widget_set_vexpand(main_box, TRUE);

    // Création de la boîte de connexion
    app_widgets->login_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(app_widgets->login_box, "login-box");
    gtk_widget_set_size_request(app_widgets->login_box, 480, -1);
    gtk_widget_set_halign(app_widgets->login_box, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(app_widgets->login_box, 150);
    gtk_widget_set_margin_bottom(app_widgets->login_box, 100);

    GtkWidget *login_title = gtk_label_new("Bon Retour!");
    gtk_widget_add_css_class(login_title, "title");
    gtk_widget_set_halign(login_title, GTK_ALIGN_CENTER);

    GtkWidget *login_subtitle = gtk_label_new("Nous sommes heureux de vous revoir !");
    gtk_widget_add_css_class(login_subtitle, "subtitle");
    gtk_widget_set_halign(login_subtitle, GTK_ALIGN_CENTER);

    GtkWidget *email_label = gtk_label_new("EMAIL OU NUMERO *");
    gtk_widget_add_css_class(email_label, "input-label");
    gtk_widget_set_halign(email_label, GTK_ALIGN_START);

    app_widgets->email_entry = gtk_entry_new();

    GtkWidget *password_label = gtk_label_new("Mot de passe *");
    gtk_widget_add_css_class(password_label, "input-label");
    gtk_widget_set_halign(password_label, GTK_ALIGN_START);

    app_widgets->password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(app_widgets->password_entry), FALSE);

    GtkWidget *forgot_link = gtk_label_new("Mot de passe oublié?");
    gtk_widget_add_css_class(forgot_link, "link");
    gtk_widget_set_halign(forgot_link, GTK_ALIGN_START);

    GtkWidget *login_button = gtk_button_new_with_label("Connexion");
    gtk_widget_add_css_class(login_button, "login-button");
    
    // Connecter le bouton de connexion à la fonction de vérification
    g_signal_connect(login_button, "clicked", G_CALLBACK(on_login_button_clicked), app_widgets);

    // Bouton pour passer à la vue d'inscription 
    GtkWidget *login_to_register = gtk_button_new_with_label("Pas de compte? S'inscrire");
    gtk_widget_add_css_class(login_to_register, "linkr");
    gtk_widget_set_halign(login_to_register, GTK_ALIGN_START);
    gtk_button_set_has_frame(GTK_BUTTON(login_to_register), FALSE); 

    // Connecter le bouton à la fonction de changement de vue
    g_signal_connect(login_to_register, "clicked", G_CALLBACK(show_register_view), app_widgets);

    gtk_box_append(GTK_BOX(app_widgets->login_box), login_title);
    gtk_box_append(GTK_BOX(app_widgets->login_box), login_subtitle);
    gtk_box_append(GTK_BOX(app_widgets->login_box), email_label);
    gtk_box_append(GTK_BOX(app_widgets->login_box), app_widgets->email_entry);
    gtk_box_append(GTK_BOX(app_widgets->login_box), password_label);
    gtk_box_append(GTK_BOX(app_widgets->login_box), app_widgets->password_entry);
    gtk_box_append(GTK_BOX(app_widgets->login_box), forgot_link);
    gtk_box_append(GTK_BOX(app_widgets->login_box), login_button);
    gtk_box_append(GTK_BOX(app_widgets->login_box), login_to_register);

    // Création de la boîte d'inscription
    app_widgets->register_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(app_widgets->register_box, "register-box");
    gtk_widget_set_size_request(app_widgets->register_box, 600, -1);
    gtk_widget_set_halign(app_widgets->register_box, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(app_widgets->register_box, 50);
    gtk_widget_set_margin_bottom(app_widgets->register_box, 100);
    gtk_widget_set_visible(app_widgets->register_box, FALSE);

    GtkWidget *register_title = gtk_label_new("Créer un compte");
    gtk_widget_add_css_class(register_title, "title");
    gtk_widget_set_halign(register_title, GTK_ALIGN_CENTER);

    GtkWidget *register_email_label = gtk_label_new("EMAIL OU NUMERO *");
    gtk_widget_add_css_class(register_email_label, "input-label");
    gtk_widget_set_halign(register_email_label, GTK_ALIGN_START);

    GtkWidget *register_email_entry = gtk_entry_new();
    gtk_widget_set_size_request(register_email_entry, 400, -1);
    gtk_widget_add_css_class(register_email_entry, "entry");

    GtkWidget *name_display = gtk_label_new("NOM D'AFFICHAGE *");
    gtk_widget_add_css_class(name_display, "input-label");
    gtk_widget_set_halign(name_display, GTK_ALIGN_START);

    GtkWidget *name_entry = gtk_entry_new();
    gtk_widget_add_css_class(name_entry, "entry");

    GtkWidget *user_name = gtk_label_new("NOM D'UTILISATEUR *");
    gtk_widget_add_css_class(user_name, "input-label");
    gtk_widget_set_halign(user_name, GTK_ALIGN_START);

    GtkWidget *user_entry = gtk_entry_new();
    gtk_widget_add_css_class(user_entry, "entry");

    GtkWidget *register_password_label = gtk_label_new("MOT DE PASSE *");
    gtk_widget_add_css_class(register_password_label, "input-label");
    gtk_widget_set_halign(register_password_label, GTK_ALIGN_START);

    GtkWidget *register_password_entry = gtk_entry_new();
    gtk_widget_add_css_class(register_password_entry, "entry");
    gtk_entry_set_visibility(GTK_ENTRY(register_password_entry), FALSE);

    GtkWidget *date_label = gtk_label_new("DATE DE NAISSANCE");
    gtk_widget_add_css_class(date_label, "input-label");
    gtk_widget_set_halign(date_label, GTK_ALIGN_START);

    GtkWidget *date_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_bottom(date_box, 16);

    GtkWidget *month_dropdown = gtk_drop_down_new_from_strings((const char*[]){"Janvier", "Fevrier", "Mars", "Avril", "Mai", "Juin", "Juilet", "Aout", "Septembre", "Octobre", "Novembre", "Decembre", NULL});
    gtk_widget_set_hexpand(month_dropdown, TRUE);
    gtk_widget_add_css_class(month_dropdown, "date-dropdown");

    GtkStringList *days = gtk_string_list_new(NULL);
    for (int i = 1; i <= 31; i++) {
        char day_str[3];
        g_snprintf(day_str, sizeof(day_str), "%d", i);
        gtk_string_list_append(days, day_str);
    }
    GtkWidget *day_dropdown = gtk_drop_down_new(G_LIST_MODEL(days), NULL);
    gtk_widget_set_hexpand(day_dropdown, TRUE);
    gtk_widget_add_css_class(day_dropdown, "date-dropdown");

    GtkStringList *years = gtk_string_list_new(NULL);
    for (int i = 2024; i >= 1900; i--) {
        char year_str[5];
        g_snprintf(year_str, sizeof(year_str), "%d", i);
        gtk_string_list_append(years, year_str);
    }
    GtkWidget *year_dropdown = gtk_drop_down_new(G_LIST_MODEL(years), NULL);
    gtk_widget_set_hexpand(year_dropdown, TRUE);
    gtk_widget_add_css_class(year_dropdown, "date-dropdown");

    gtk_box_append(GTK_BOX(date_box), month_dropdown);
    gtk_box_append(GTK_BOX(date_box), day_dropdown);
    gtk_box_append(GTK_BOX(date_box), year_dropdown);

    GtkWidget *check_mail = gtk_check_button_new_with_label("Je veux recevoir des e-mails à propos des mises à jour Discord+");
    gtk_widget_add_css_class(check_mail, "check-box");
    gtk_widget_set_halign(check_mail, GTK_ALIGN_START);

    GtkWidget *register_button = gtk_button_new_with_label("Continuer");
    gtk_widget_add_css_class(register_button, "register-button");

    GtkWidget *register_to_login = gtk_button_new_with_label("Vous avez déjà un compte? Se connecter");
    gtk_widget_add_css_class(register_to_login, "linkr");
    gtk_widget_set_halign(register_to_login, GTK_ALIGN_START);
    gtk_widget_set_margin_top(register_to_login, 15);
    gtk_button_set_has_frame(GTK_BUTTON(register_to_login), FALSE);  
    
    // Connecter le bouton à la fonction de changement de vue
    g_signal_connect(register_to_login, "clicked", G_CALLBACK(show_login_view), app_widgets);

    gtk_box_append(GTK_BOX(app_widgets->register_box), register_title);
    gtk_box_append(GTK_BOX(app_widgets->register_box), register_email_label);
    gtk_box_append(GTK_BOX(app_widgets->register_box), register_email_entry);
    gtk_box_append(GTK_BOX(app_widgets->register_box), name_display);
    gtk_box_append(GTK_BOX(app_widgets->register_box), name_entry);
    gtk_box_append(GTK_BOX(app_widgets->register_box), user_name);
    gtk_box_append(GTK_BOX(app_widgets->register_box), user_entry);
    gtk_box_append(GTK_BOX(app_widgets->register_box), register_password_label);
    gtk_box_append(GTK_BOX(app_widgets->register_box), register_password_entry);
    gtk_box_append(GTK_BOX(app_widgets->register_box), date_label);
    gtk_box_append(GTK_BOX(app_widgets->register_box), date_box);
    gtk_box_append(GTK_BOX(app_widgets->register_box), check_mail);
    gtk_box_append(GTK_BOX(app_widgets->register_box), register_button);
    gtk_box_append(GTK_BOX(app_widgets->register_box), register_to_login);

    // Conteneur principal pour contenir les deux boîtes
    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(content_box, TRUE);
    gtk_widget_set_vexpand(content_box, TRUE);
    
    // Ajouter les deux boîtes au conteneur principal
    gtk_box_append(GTK_BOX(content_box), app_widgets->login_box);
    gtk_box_append(GTK_BOX(content_box), app_widgets->register_box);

    gtk_box_append(GTK_BOX(main_box), content_box);
    
    gtk_overlay_set_child(GTK_OVERLAY(overlay), bg_image);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), main_box);

    gtk_window_set_child(GTK_WINDOW(window), overlay);
    
    g_signal_connect_swapped(window, "destroy", G_CALLBACK(g_free), app_widgets);
    
    gtk_window_present(GTK_WINDOW(window));
}

// Fonction pour tester la connexion à PostgreSQL
void test_postgres_connection() {
    const char *conninfo = "host=localhost port=5432 dbname=mydiscord user=postgres password=rachidsql";

    PGconn *conn = PQconnectdb(conninfo);

    if (PQstatus(conn) == CONNECTION_BAD) {
        fprintf(stderr, "Connexion à PostgreSQL échouée : %s", PQerrorMessage(conn));
    } else {
        printf("✅ Connexion à PostgreSQL réussie !\n");
    }

    PQfinish(conn);
}

void start_server(void);
    
int main(int argc, char *argv[]) {
    GtkApplication *app = gtk_application_new("com.example.MyDiscordGTK4", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
