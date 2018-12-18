#ifndef __MEDIA
#define __MEDIA

#include <gtk/gtk.h>
#include <cairo/cairo.h>
#include <gst/gst.h>

#include "waster-image-loader.h"

struct _WsImageView
{
  GtkWidget parent_instance;

  GtkWidget *picture;
};

#define WS_TYPE_IMAGE_VIEW ws_image_view_get_type()

G_DECLARE_FINAL_TYPE (WsImageView, ws_image_view, WS, IMAGE_VIEW, GtkWidget);

GtkWidget *ws_image_view_new ();
void       ws_image_view_set_contents     (WsImageView  *image_view,
                                           GdkPaintable *paintable);
#endif
