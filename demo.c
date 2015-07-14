#include "bt-listbox.h"


int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *scroller = gtk_scrolled_window_new (NULL, NULL);
  GtkWidget *list = bt_list_box_new ();

  gtk_container_add (GTK_CONTAINER (scroller), list);


  gtk_container_add (GTK_CONTAINER (window), scroller);

  gtk_widget_show_all (window);
  gtk_main ();

  return 0;
}
