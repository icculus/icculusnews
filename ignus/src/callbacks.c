#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <IcculusNews.h>
#include <stdio.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

void set_queue_info_labels(int qid);
void set_submit_story_window_caption();

void
on_login_button_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *hostname_entry, *port_entry, *username_entry, *password_entry;
	INEWS_init();
	
	hostname_entry = lookup_widget(LoginWindow, "hostname_entry");
	port_entry = lookup_widget(LoginWindow, "port_entry");
	username_entry = lookup_widget(LoginWindow, "username_entry");
	password_entry = lookup_widget(LoginWindow, "password_entry");

	hostname = gtk_editable_get_chars(GTK_EDITABLE(hostname_entry), 0, -1);
	port = (guint)gtk_spin_button_get_value(GTK_SPIN_BUTTON(port_entry));
	username = gtk_editable_get_chars(GTK_EDITABLE(username_entry), 0, -1);
	password = gtk_editable_get_chars(GTK_EDITABLE(password_entry), 0, -1);		
	
	if (INEWS_connect(hostname, port) < 0) {
		/* error dialog box goes here. */
	} else if (INEWS_auth((username && strcmp(username, "")) ? username : NULL, 
												password) < 0) {
		/* different error dialog box goes here. */
		INEWS_disconnect();
	} else {
		StorySubmitWindow = create_StorySubmitWindow();
		gtk_widget_hide_all(LoginWindow);
		gtk_widget_show_all(StorySubmitWindow);
	}

	INEWS_retrQueueInfo();

	set_submit_story_window_caption();	
}


void
on_quit_button_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
	INEWS_disconnect();
	gtk_main_quit();
}

void
on_queues_button_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *queue_list;
	char newtitle[256];
	int i = 0;	
	
	QueueListWindow = create_QueueListWindow();
	queue_list = lookup_widget(QueueListWindow, "queue_list");

	if (!queues) {
		queues = INEWS_getAllQueuesInfo();
		queue_list_store = gtk_list_store_new(2, G_TYPE_INT, G_TYPE_STRING);
	
		do {
			GtkTreeIter iter;

			gtk_list_store_append(queue_list_store, &iter);
		
			gtk_list_store_set(queue_list_store, &iter, 0, queues[i]->qid,
												 1, queues[i]->name, -1);
		} while (queues[++i]);
	}
	
	gtk_tree_view_set_model(GTK_TREE_VIEW(queue_list), 
													GTK_TREE_MODEL(queue_list_store));

	qlist_renderer = gtk_cell_renderer_text_new();
	qid_column = gtk_tree_view_column_new_with_attributes("Queue ID",
																												qlist_renderer,
																												"text", 0, NULL);
	qname_column = gtk_tree_view_column_new_with_attributes("Queue Name",
																													qlist_renderer,
																													"text", 1, NULL);
		
	gtk_tree_view_append_column(GTK_TREE_VIEW(queue_list), qid_column);
	gtk_tree_view_append_column(GTK_TREE_VIEW(queue_list), qname_column);

	memset(newtitle, 0, 256);
	
	sprintf(newtitle, "Queues [inews://%s@%s:%i]", INEWS_getUserName(),
					INEWS_getHost(), INEWS_getPort());

	gtk_window_set_title(GTK_WINDOW(QueueListWindow), newtitle);
	
	gtk_widget_show_all(QueueListWindow);
}


void
on_queue_list_cursor_changed           (GtkTreeView     *treeview,
                                        gpointer         user_data)
{
	GtkTreeSelection *new_selection;
	GtkTreeIter new_selection_iter;
	guint sel_queue_num;

	new_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	gtk_tree_selection_get_selected(new_selection, NULL, &new_selection_iter);
	gtk_tree_model_get(GTK_TREE_MODEL(queue_list_store), &new_selection_iter, 0,
										 &sel_queue_num, -1);
	
	set_queue_info_labels(sel_queue_num);
}


void
on_selectq_button_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *queue_list;
	GtkTreePath *selected_queue_path;
	GtkTreeIter selected_queue_iter;
	guint sel_queue_num;
	
	queue_list = lookup_widget(QueueListWindow, "queue_list");
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(queue_list), &selected_queue_path, 
													 NULL);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(queue_list_store), 
													&selected_queue_iter, selected_queue_path);
	gtk_tree_model_get(GTK_TREE_MODEL(queue_list_store), &selected_queue_iter, 0,
										 &sel_queue_num, -1);
	
	INEWS_changeQueue(sel_queue_num);

	set_submit_story_window_caption();
}


void
on_art_list_button_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_close_button_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_hide_all(QueueListWindow);
}

void set_queue_info_labels (int qid) {
	GtkWidget *qid_label, *qname_label, *desc_label, *ctime_label, *owner_label,
						*digest_label, *singleitem_label, *home_label, *rdf_label;
	gchar temp[512];
	int i = 0;
						
	qid_label = lookup_widget(QueueListWindow, "qid_label");
	qname_label = lookup_widget(QueueListWindow, "qname_label");
	desc_label = lookup_widget(QueueListWindow, "desc_label");
	ctime_label = lookup_widget(QueueListWindow, "ctime_label");
	owner_label = lookup_widget(QueueListWindow, "owner_label");
	digest_label = lookup_widget(QueueListWindow, "digest_label");
	singleitem_label = lookup_widget(QueueListWindow, "singleitem_label");
	home_label = lookup_widget(QueueListWindow, "home_label");
	rdf_label = lookup_widget(QueueListWindow, "rdf_label");

	do {
		if (queues[i]->qid == qid) {
			memset(temp, 0, 512);
			sprintf(temp, "QID: %i", queues[i]->qid);
			gtk_label_set_text(GTK_LABEL(qid_label), temp);
			memset(temp, 0, 512);
			sprintf(temp, "Name: %s", queues[i]->name);
			gtk_label_set_text(GTK_LABEL(qname_label), temp);
			memset(temp, 0, 512);
			sprintf(temp, "Description: %s", queues[i]->description);
			gtk_label_set_text(GTK_LABEL(desc_label), temp);
			memset(temp, 0, 512);
			sprintf(temp, "Owner: %s (UID %i)", queues[i]->ownername, 
							queues[i]->owneruid);
			gtk_label_set_text(GTK_LABEL(owner_label), temp);
			memset(temp, 0, 512);
			strftime(temp, 511, "Created: %Y-%m-%d %T", &(queues[i]->ctime));
			gtk_label_set_text(GTK_LABEL(ctime_label), temp);
			memset(temp, 0, 512);
			sprintf(temp, "Digest: %s", queues[i]->digest);
			gtk_label_set_text(GTK_LABEL(digest_label), temp);
      memset(temp, 0, 512);
			sprintf(temp, "Single Item: %s", queues[i]->singleitem);
			gtk_label_set_text(GTK_LABEL(singleitem_label), temp);
      memset(temp, 0, 512);
			sprintf(temp, "Home: %s", queues[i]->home);
			gtk_label_set_text(GTK_LABEL(home_label), temp);
      memset(temp, 0, 512);
			sprintf(temp, "RDF: %s", queues[i]->rdf);
			gtk_label_set_text(GTK_LABEL(rdf_label), temp);
		}
	} while (queues[++i]);
}

void
on_clear_button_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *story_view, *topic_entry;
	GtkTextBuffer *story_buffer;

	story_view = lookup_widget(StorySubmitWindow, "story_entry");
	topic_entry = lookup_widget(StorySubmitWindow, "topic_entry");
	
	story_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(story_view));
	gtk_text_buffer_set_text(story_buffer, "", -1);
	gtk_entry_set_text(GTK_ENTRY(topic_entry), "");
}

void set_submit_story_window_caption () {
  char newtitle[256];

	memset(newtitle, 0, 256);
	
	sprintf(newtitle, "Submit Story [inews://%s@%s:%i/%s]", INEWS_getUserName(),
          INEWS_getHost(), INEWS_getPort(),
          INEWS_getQID() ? INEWS_getQueueInfo(INEWS_getQID())->name : "");

  gtk_window_set_title(GTK_WINDOW(StorySubmitWindow), newtitle);		
}

void
on_submit_button_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *story_entry, *topic_entry;
	char *topic, *body;
	GtkTextBuffer *story_buffer;
	GtkTextIter buffer_start_iter, buffer_end_iter;
	
	story_entry = lookup_widget(StorySubmitWindow, "story_entry");
	topic_entry = lookup_widget(StorySubmitWindow, "topic_entry");
	
	story_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(story_entry));

	gtk_text_buffer_get_start_iter(story_buffer, &buffer_start_iter);
	gtk_text_buffer_get_end_iter(story_buffer, &buffer_end_iter);
	
	topic = gtk_editable_get_chars(GTK_EDITABLE(topic_entry), 0, -1);
	
	body = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(story_buffer),
																	&buffer_start_iter, &buffer_end_iter, FALSE);
	
	printf("In on_submit_button_clicked().\n");
	
	if (strcmp(topic, "")) {
		printf("INEWS_submitArticle() returned %i.\n", INEWS_submitArticle(topic, body));
	}
}

