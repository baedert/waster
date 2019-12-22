#include "waster.h"
#include "waster-main-window.h"
#include "imgur-proxy.h"

#include <string.h>
#include <gtk/gtk.h>
#include <rest/rest-proxy.h>
#include <rest/oauth2-proxy.h>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#define CLIENT_ID "a4800ceccb6aaeb"
#define CLIENT_SECRET "e5687608c2100712648cb5946acd6c5d84ae97f1"


G_DEFINE_QUARK (ws-error-quark, ws_error);

G_DEFINE_TYPE (Waster, ws, GTK_TYPE_APPLICATION);


gboolean
waster_is_proxy_inited (Waster *waster)
{
  char *access_token;
  char *refresh_token;
  gboolean inited;

  access_token  = g_settings_get_string (waster->settings, "access-token");
  refresh_token = g_settings_get_string (waster->settings, "refresh-token");

  inited = strlen (access_token) > 0 &&
           strlen (refresh_token) > 0;

  g_free (access_token);
  g_free (refresh_token);

  return inited;
}

char *
waster_get_login_url (Waster *waster)
{
  GHashTable *params = g_hash_table_new (g_str_hash, g_str_equal);
  char *login_url;

  g_hash_table_insert (params, "client_id",     CLIENT_ID);
  g_hash_table_insert (params, "response_type", "pin");

  login_url = oauth2_proxy_build_login_url_full (OAUTH2_PROXY (waster->proxy),
                                                 "",
                                                 params);

  g_hash_table_destroy (params);

  return login_url;
}

void
waster_set_access_tokens (Waster       *waster,
                          const char  *access_token,
                          const char  *refresh_token)
{
  gint64 timestamp = g_date_time_to_unix (g_date_time_new_now_local ());
  oauth2_proxy_set_access_token (OAUTH2_PROXY (waster->proxy),
                                 access_token);
  g_settings_set_string (waster->settings, "access-token", access_token);
  g_settings_set (waster->settings, "refresh-timestamp", "x", timestamp, NULL);
  if (refresh_token)
    g_settings_set_string (waster->settings, "refresh-token", refresh_token);
}

static gboolean
waster_is_token_expired (Waster *waster)
{
  GDateTime *now_dt = g_date_time_new_now_local ();
  gint64 now = g_date_time_to_unix (now_dt);
  gint64 timestamp;

  g_date_time_unref (now_dt);

  g_settings_get (waster->settings, "refresh-timestamp", "x", &timestamp);

  g_assert (now >= timestamp);

  /* imgur tokens last 3600 seconds, i.e. an hour. */
  return (now - 3600 > timestamp);
}

void
waster_maybe_refresh_token (Waster     *waster,
                            const char *pin)
{
  const char *refresh_token;
  char *req;
  SoupSession *session;
  SoupMessage *message;
  SoupMessageBody *response_body;
  GError *error = NULL;
  GString *request = g_string_new ("");

  if (!pin && !waster_is_token_expired (waster))
    {
      g_string_free (request, TRUE);
      return;
    }

  g_message ("Refreshing the access token... (PIN '%s')", pin);

  session = soup_session_new ();
  message = soup_message_new ("POST", "https://api.imgur.com/oauth2/token");

  g_string_append (request, "client_id=");
  g_string_append (request, CLIENT_ID);
  g_string_append (request, "&client_secret=");
  g_string_append (request, CLIENT_SECRET);

  /* Without a PIN, this will use the refresh token to get us a new token */
  if (!pin)
    {
      char *refresh_token = g_settings_get_string (waster->settings, "refresh-token");
      g_string_append (request, "&grant_type=refresh_token&refresh_token=");
      g_string_append (request, refresh_token);
      g_free (refresh_token);
    }
  else
    {
      g_string_append (request, "&grant_type=pin&pin=");
      g_string_append (request, pin);
    }


  req = g_string_free (request, FALSE);
  g_message ("Request: %s", req);

  soup_message_set_request (message,
                            "application/x-www-form-urlencoded",
                            SOUP_MEMORY_COPY,
                            req,
                            strlen (req));

  g_free (req);


  soup_session_send_message (session, message);

  g_object_get (G_OBJECT (message), "response_body", &response_body, NULL);

  const char *result = (const char *) response_body->data;

  if (!result)
    {
      g_error ("Could not refresh token");
    }

  JsonParser *parser = json_parser_new ();
  json_parser_load_from_data (parser, result, -1, &error);

  if (error)
    {
      printf ("%s\n", result);
      g_error ("%s", error->message);
    }

  JsonObject *root = json_node_get_object (json_parser_get_root (parser));

  const char *access_token = json_object_get_string_member (root, "access_token");

  refresh_token = json_object_get_string_member (root, "refresh_token");

  waster_set_access_tokens (waster, access_token, refresh_token);

  g_object_unref (parser);
  g_object_unref (message);
  g_object_unref (session);
}


static void
activate (GApplication *app)
{
  WsMainWindow *win = ws_main_window_new (GTK_APPLICATION (app));

  gtk_widget_show (GTK_WIDGET (win));
}

static void
startup (GApplication *app)
{
  GtkSettings *settings;
  const char *accels[] = {NULL, NULL};


  G_APPLICATION_CLASS (ws_parent_class)->startup (app);


  accels[0] = "Right";
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.go-next", accels);

  accels[0] = "Left";
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.go-prev", accels);

  accels[0] = "Down";
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.go-down", accels);

  accels[0] = "Up";
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.go-up", accels);

  accels[0] = "<Control>S";
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.save-current", accels);

  accels[0] = "m";
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.toggle-muted", accels);


  settings = gtk_settings_get_default ();
  g_object_set (G_OBJECT (settings), "gtk-application-prefer-dark-theme", TRUE, NULL);

  /* Custom CSS */
  {
    GtkCssProvider *provider = gtk_css_provider_new ();

    gtk_css_provider_load_from_resource (provider, "/org/baedert/waster/ui/style.css");
    gtk_style_context_add_provider_for_display (gdk_display_get_default (),
                                                GTK_STYLE_PROVIDER (provider),
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);\
  }

}



Waster *
waster_new (void)
{
  return (Waster *) (g_object_new (WS_TYPE_APP,
                     "application_id", "org.baedert.waster",
                     NULL));
}

static void
ws_init (Waster *waster)
{
  waster->settings = g_settings_new ("org.baedert.waster");
  waster->proxy    = imgur_proxy_new (CLIENT_ID);

  if (waster_is_proxy_inited (waster))
    {
      char *access_token = g_settings_get_string (waster->settings, "access-token");
      oauth2_proxy_set_access_token (OAUTH2_PROXY (waster->proxy), access_token);

      waster_maybe_refresh_token (waster, NULL);
      g_free (access_token);
    }
}

static void
ws_class_init (WasterClass *class)
{
  GApplicationClass *application_class = G_APPLICATION_CLASS (class);

  application_class->activate = activate;
  application_class->startup  = startup;
}
