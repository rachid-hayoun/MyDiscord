#ifndef MYDISCORD_H
#define MYDISCORD_H

#include <gtk/gtk.h>

#define MAX_MESSAGES 100
#define MAX_CONVERSATIONS 50

typedef struct {
    char user[32];
    char message[256];
    char time[9];
} Message;

typedef struct {
    char name[64];
    Message messages[MAX_MESSAGES];
    int count;
} Conversation;

// Variables globales (déclarées ici, définies dans mydiscord.c)
extern Conversation conversations[MAX_CONVERSATIONS];
extern int conversation_count;
extern Conversation *current_conversation;

extern GtkWidget *chat_box;
extern GtkWidget *input_box;
extern GtkWidget *header_label;
extern GtkWidget *servers_container;
extern int dynamic_server_count;

// Prototypes
void build_mydiscord_interface(GtkWindow *window);

#endif // MYDISCORD_H
