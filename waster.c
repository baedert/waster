
#include <gtk/gtk.h>

#include "waster-main-window.h"



int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);
  WsMainWindow *window = ws_main_window_new ();


  gtk_widget_show_all (GTK_WIDGET (window));
  gtk_main ();
  return 0;
}
