#ifndef __HAVE_GLOBALS_H
#define __HAVE_GLOBALS_H

#include <gtk/gtk.h>
#include <IcculusNews.h>

gchar *hostname;
guint port;
gchar *username;
gchar *password;

QueueInfo **queues;
GtkListStore *queue_list_store;

GtkWidget *LoginWindow;
GtkWidget *StorySubmitWindow;
GtkWidget *QueueListWindow;

GtkCellRenderer *qlist_renderer;
GtkTreeViewColumn *qid_column;
GtkTreeViewColumn *qname_column;

#endif
