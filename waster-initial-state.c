#include <gtk/gtk.h>
#include <rest/rest-proxy.h>
#include <rest/rest-proxy-call.h>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>
#include <string.h>

#include "waster-initial-state.h"
#include "waster.h"




struct _WsInitialState
{
  GtkBox parent_instance;
  GtkWidget *pin_entry;
  GtkWidget *request_pin_button;
  GtkWidget *submit_button;
};

typedef struct _WsInitialState WsInitialState;

G_DEFINE_TYPE (WsInitialState, ws_initial_state, GTK_TYPE_BOX);



GtkWidget *
ws_initial_state_new (void)
{
  return GTK_WIDGET (g_object_new (WS_TYPE_INITIAL_STATE, NULL));
}


void
request_pin_button_clicked_cb ()
{
  Waster *app = (Waster *)g_application_get_default ();
  gchar *login_url = waster_get_login_url (app);

  g_app_info_launch_default_for_uri (login_url, NULL, NULL);
  g_free (login_url);
}

void
submit_button_clicked_cb (GtkWidget      *source,
                          WsInitialState *initial_state)
{
  Waster *app = (Waster *) (g_application_get_default ());
  const gchar *pin = gtk_entry_get_text (GTK_ENTRY (initial_state->pin_entry));

  waster_maybe_refresh_token (app, pin);
}


void
ws_initial_state_init (WsInitialState *state)
{
  gtk_widget_init_template (GTK_WIDGET (state));
}

void
ws_initial_state_class_init (WsInitialStateClass *class)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/baedert/waster/ui/initial-state.ui");

  gtk_widget_class_bind_template_child (widget_class, WsInitialState, pin_entry);
  gtk_widget_class_bind_template_child (widget_class, WsInitialState, request_pin_button);
  gtk_widget_class_bind_template_child (widget_class, WsInitialState, submit_button);

  gtk_widget_class_bind_template_callback (widget_class, request_pin_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, submit_button_clicked_cb);
}
