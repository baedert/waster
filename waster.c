#include "waster.h"
#include "waster-main-window.h"

#include <string.h>
#include <gtk/gtk.h>
#include <rest/rest-proxy.h>
#include <rest/oauth2-proxy.h>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>



G_DEFINE_QUARK (ws-error-quark, ws_error);

G_DEFINE_TYPE (Waster, ws, GTK_TYPE_APPLICATION);


gboolean
waster_is_proxy_inited (Waster *waster)
{
  gchar *access_token;
  gchar *refresh_token;
  gboolean inited;

  access_token  = g_settings_get_string (waster->settings, "access-token");
  refresh_token = g_settings_get_string (waster->settings, "refresh-token");

  inited = strlen (access_token) > 0 &&
           strlen (refresh_token) > 0;

  g_free (access_token);
  g_free (refresh_token);

  return inited;
}


gchar *
waster_get_login_url (Waster *waster)
{
  GHashTable *params = g_hash_table_new (g_str_hash, g_str_equal);


  g_hash_table_insert (params, "client_id",     "8774fc09703750c");
  g_hash_table_insert (params, "response_type", "pin");

  gchar *login_url = oauth2_proxy_build_login_url_full (OAUTH2_PROXY (waster->proxy),
                                                        "",
                                                        params);

  g_hash_table_destroy (params);

  return login_url;
}

void
waster_set_access_tokens (Waster       *waster,
                          const gchar  *access_token,
                          const gchar  *refresh_token)
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
waster_maybe_refresh_token (Waster      *waster,
                            const gchar *pin)
{
  const gchar *refresh_token;
  gchar *req;
  SoupSession *session;
  SoupMessage *message;
  SoupMessageBody *response_body;
  GError *error = NULL;
  GString *request = g_string_new ("client_id=8774fc09703750c&client_secret=69c5f69a22dc9baf56c58c174f40de5eaefc3f5e");

  if (!pin && !waster_is_token_expired (waster))
    {
      g_string_free (request, TRUE);
      return;
    }

  g_message ("Refreshing the access token...");


  if (!pin)
    {
      refresh_token = g_settings_get_string (waster->settings, "refresh-token");
      g_string_append (request, "&grant_type=refresh_token&refresh_token=");
      g_string_append (request, refresh_token);
      g_free ((char *)refresh_token);
    }
  else
    {
      g_string_append (request, "?grant_type=pin&pin=");
      g_string_append (request, pin);
    }

  session = soup_session_new ();
  message = soup_message_new ("POST", "https://api.imgur.com/oauth2/token");

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

  gchar *result = (gchar *) response_body->data;
  /*g_message ("%s", result);*/

  JsonParser *parser = json_parser_new ();
  json_parser_load_from_data (parser, result, -1, &error);

  if (error)
    {
      g_error ("%s", error->message);
    }

  JsonObject *root = json_node_get_object (json_parser_get_root (parser));

  const gchar *access_token = json_object_get_string_member (root, "access_token");
  refresh_token = json_object_get_string_member (root, "refresh_token");

  waster_set_access_tokens (waster, access_token, refresh_token);

  g_object_unref (message);
  g_object_unref (session);
}


static void
activate (GApplication *app)
{
  WsMainWindow *win = ws_main_window_new (GTK_APPLICATION (app));

  gtk_widget_show_all (GTK_WIDGET (win));
}

static void
go_next_cb (GSimpleAction *action, GVariant *v, gpointer user_data)
{
  g_message ("next");
}


static void
startup (GApplication *app)
{
  GtkSettings *settings;


  G_APPLICATION_CLASS (ws_parent_class)->startup (app);

  const char *accels[] = {NULL, NULL};

  accels[0] = "Right";
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.go-next", accels);


  settings = gtk_settings_get_default ();
  g_object_set (G_OBJECT (settings), "gtk-application-prefer-dark-theme", TRUE, NULL);
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
  waster->proxy    = oauth2_proxy_new ("8774fc09703750c",
                                       "https://api.imgur.com/oauth2/authorize",
                                       "https://api.imgur.com/3/",
                                       FALSE);

  if (waster_is_proxy_inited (waster))
    {
      gchar *access_token = g_settings_get_string (waster->settings, "access-token");
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
