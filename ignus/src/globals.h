#ifndef __HAVE_GLOBALS_H
#define __HAVE_GLOBALS_H

#include <gtk/gtk.h>
#include <IcculusNews.h>

gchar *hostname;
guint port;
gchar *username;
gchar *password;

QueueInfo **queues;
ArticleInfo **articles;
GtkListStore *queue_list_store;
GtkListStore *article_list_store;

GtkWidget *LoginWindow;
GtkWidget *StorySubmitWindow;
GtkWidget *QueueListWindow;
GtkWidget *ArticleListWindow;

GtkCellRenderer *qlist_renderer;
GtkTreeViewColumn *qid_column;
GtkTreeViewColumn *qname_column;

GtkCellRenderer *alist_renderer;
GtkTreeViewColumn *aid_column;
GtkTreeViewColumn *atitle_column;

#endif
