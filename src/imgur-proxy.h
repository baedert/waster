#ifndef IMGUR_PROXY_H
#define IMGUR_PROXY_H

#include <rest/rest-proxy.h>
#include <rest/oauth2-proxy.h>

struct _ImgurProxy
{
  OAuth2Proxy parent_instance;
};

typedef struct _ImgurProxy ImgurProxy;
#define IMGUR_TYPE_PROXY imgur_proxy_get_type ()

G_DECLARE_FINAL_TYPE (ImgurProxy, imgur_proxy, IMGUR, PROXY, OAuth2Proxy);

RestProxy *imgur_proxy_new (const char *client_id);

#endif
