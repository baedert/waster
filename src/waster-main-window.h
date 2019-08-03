
#pragma once
#include <glib-object.h>
#include <gtk/gtk.h>


#define WS_TYPE_MAIN_WINDOW ws_main_window_get_type ()

G_DECLARE_FINAL_TYPE (WsMainWindow, ws_main_window, WS, MAIN_WINDOW, GtkApplicationWindow);

WsMainWindow *ws_main_window_new ();
