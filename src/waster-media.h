
#pragma once

#include <gtk/gtk.h>
#include <cairo/cairo.h>
//#include <gst/gst.h>

#include "waster-image-loader.h"

struct _WsImageView
{
  GtkWidget parent_instance;

  GtkWidget *picture;
  GtkWidget *desc;


  guint muted: 1;
};

#define WS_TYPE_IMAGE_VIEW ws_image_view_get_type()

G_DECLARE_FINAL_TYPE (WsImageView, ws_image_view, WS, IMAGE_VIEW, GtkWidget);

GtkWidget *    ws_image_view_new ();
void           ws_image_view_set_contents     (WsImageView  *self,
                                               GdkPaintable *paintable,
                                               const char   *description);
GdkPaintable * ws_image_view_get_contents     (WsImageView  *self);
void           ws_image_view_start            (WsImageView  *self);
void           ws_image_view_stop             (WsImageView  *self);
void           ws_image_view_set_muted        (WsImageView  *self,
                                               gboolean      muted);
