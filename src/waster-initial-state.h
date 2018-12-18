#ifndef WASTER_INITIAL_STATE__
#define WASTER_INITIAL_STATE__

#include <glib-object.h>
#include <gtk/gtk.h>


#define WS_TYPE_INITIAL_STATE ws_initial_state_get_type ()

G_DECLARE_FINAL_TYPE (WsInitialState, ws_initial_state, ws, INITIAL_STATE, GtkBox)


GtkWidget * ws_initial_state_new (void);

#endif
