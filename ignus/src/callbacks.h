#include <gtk/gtk.h>

#include "globals.h"

void
on_login_button_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_hostname_entry_changed              (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_port_entry_change_value             (GtkSpinButton   *spinbutton,
                                        GtkScrollType    arg1,
                                        gpointer         user_data);

void
on_username_entry_changed              (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_password_entry_changed              (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_quit_button_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_queues_button_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_queue_list_row_activated            (GtkTreeView     *treeview,
                                        GtkTreePath     *arg1,
                                        GtkTreeViewColumn *arg2,
                                        gpointer         user_data);

void
on_selectq_button_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_art_list_button_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_close_button_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_queue_list_cursor_changed           (GtkTreeView     *treeview,
                                        gpointer         user_data);

void
on_clear_button_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_submit_button_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_edit_article_button_clicked         (GtkButton       *button,
                                        gpointer         user_data);

void
on_app_unapp_button_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_del_undel_button_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_purge_sel_button_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_purge_all_button_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_close_alist_button_clicked          (GtkButton       *button,
                                        gpointer         user_data);
