#ifndef WS_MAIN_WINDOW__
#define WS_MAIN_WINDOW__

#include <glib-object.h>
#include <gtk/gtk.h>


#define WS_TYPE_MAIN_WINDOW ws_main_window_get_type ()

G_DECLARE_FINAL_TYPE (WsMainWindow, ws_main_window, WS, MAIN_WINDOW, GtkWindow);


WsMainWindow *ws_main_window_new ();

#endif
