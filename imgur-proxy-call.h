#ifndef IMGUR_PROXY_CALL_H
#define IMGUR_PROXY_CALL_H

#include <rest/rest-proxy-call.h>


struct _ImgurProxyCall
{
  RestProxyCall parent_instance;
};

typedef struct _ImgurProxyCall ImgurProxyCall;
#define IMGUR_TYPE_PROXY_CALL imgur_proxy_call_get_type ()

G_DECLARE_FINAL_TYPE (ImgurProxyCall, imgur_proxy_call, IMGUR, PROXY_CALL, RestProxyCall);








#endif
