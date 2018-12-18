#include <rest/rest-proxy-call.h>
#include <rest/rest-proxy.h>
#include <rest/oauth2-proxy.h>

#include "imgur-proxy.h"
#include "imgur-proxy-call.h"


G_DEFINE_TYPE (ImgurProxy, imgur_proxy, OAUTH2_TYPE_PROXY);



static RestProxyCall *
imgur_proxy_new_call (RestProxy *proxy)
{
  ImgurProxyCall *call = g_object_new (IMGUR_TYPE_PROXY_CALL,
                                       "proxy", proxy,
                                       NULL);

  return REST_PROXY_CALL (call);
}

RestProxy *
imgur_proxy_new (const char *client_id)
{
  return REST_PROXY (g_object_new (IMGUR_TYPE_PROXY,
                                   "client-id", client_id,
                                   "auth-endpoint", "https://api.imgur.com/oauth2/authorize",
                                   "url-format", "https://api.imgur.com/3/",
                                   "binding-required", FALSE,
                                   NULL));
}

static void
imgur_proxy_class_init (ImgurProxyClass *class)
{
  RestProxyClass *proxy_class = REST_PROXY_CLASS (class);

  proxy_class->new_call = imgur_proxy_new_call;
}

static void
imgur_proxy_init (ImgurProxy *proxy)
{

}
