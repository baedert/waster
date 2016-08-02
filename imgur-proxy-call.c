#include "imgur-proxy-call.h"
#include <rest/rest-proxy.h>
#include <rest/oauth2-proxy.h>


G_DEFINE_TYPE (ImgurProxyCall, imgur_proxy_call, REST_TYPE_PROXY_CALL);



static gboolean
imgur_proxy_call_prepare (RestProxyCall *call, GError **error)
{
  OAuth2Proxy *proxy = NULL;
  char *header;

  g_object_get (call, "proxy", &proxy, NULL);

  if (oauth2_proxy_get_access_token (proxy) == NULL)
    {
      g_error ("%s(%d): ImgurProxyCall without access_token", __FILE__, __LINE__);
    }

  /* OAuth2ProxyCall adds a access_token parameter to the call here, but imgur
   * wants an Authorization header instead */
  header = g_strdup_printf ("Bearer %s", oauth2_proxy_get_access_token (proxy));
  rest_proxy_call_add_header (call, "Authorization", header);

  g_free (header);
  return TRUE;
}

static void
imgur_proxy_call_class_init (ImgurProxyCallClass *class)
{
  RestProxyCallClass *proxy_call_class = REST_PROXY_CALL_CLASS (class);

  proxy_call_class->prepare = imgur_proxy_call_prepare;
}

static void
imgur_proxy_call_init (ImgurProxyCall *call)
{

}
