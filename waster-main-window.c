#include <waster-main-window.h>


struct _WsMainWindow
{
  GtkWindow parent_instance;
  GtkWidget *header_bar;
  GtkWidget *next_button;
  GtkWidget *prev_button;

};

G_DEFINE_TYPE (WsMainWindow, ws_main_window, GTK_TYPE_WINDOW);


WsMainWindow *
ws_main_window_new ()
{
  return g_object_new (WS_TYPE_MAIN_WINDOW, NULL);
}

void
ws_main_window_init (WsMainWindow *win)
{
  win->header_bar = gtk_header_bar_new ();
  win->next_button = gtk_button_new_with_label ("Next");
  win->prev_button = gtk_button_new_with_label ("Prev");

  gtk_container_add (GTK_CONTAINER (win->header_bar), win->next_button);
  gtk_container_add (GTK_CONTAINER (win->header_bar), win->prev_button);
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (win->header_bar), TRUE);

  gtk_window_set_titlebar (GTK_WINDOW (win), win->header_bar);
}

void
ws_main_window_class_init (WsMainWindowClass *class)
{

}
