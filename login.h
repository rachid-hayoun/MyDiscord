#ifndef LOGIN_H
#define LOGIN_H

#include <gtk/gtk.h>

typedef struct {
    GtkWidget *login_box;
    GtkWidget *register_box;
    GtkWidget *header_title;
    GtkWidget *email_entry;
    GtkWidget *password_entry;
    GtkWindow *parent_window;
} AppWidgets;

void show_error_dialog(GtkWindow *parent, const char *message);
void show_success_dialog(GtkWindow *parent, const char *message);

#endif // LOGIN_H
