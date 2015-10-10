#ifndef WASTER__
#define WASTER__

#include <glib-object.h>
#include <gtk/gtk.h>
#include <rest/rest-proxy.h>
#include <rest/oauth2-proxy.h>

struct _Waster
{
  GtkApplication parent_instance;
  GSettings      *settings;
  RestProxy      *proxy;
};

typedef struct _Waster Waster;

#define WS_TYPE_APP ws_get_type ()

#define WS_ERROR ws_error_quark()
GQuark ws_error_quark (void);


enum {
  WS_GENERIC_ERROR
} WsErrors;

G_DECLARE_FINAL_TYPE (Waster, waster, ws, WS, GtkApplication)

Waster * waster_new (void);
gboolean waster_is_proxy_inited (Waster *waster);
gchar *  waster_get_login_url (Waster *waster);
void     waster_set_access_tokens (Waster *waster, const gchar *access_token, const gchar *refresh_token);
void     waster_maybe_refresh_token (Waster *waster, const gchar *pin);

#endif
