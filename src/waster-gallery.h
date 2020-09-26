#pragma once

#include <glib.h>
#include <json-glib/json-glib.h>
#include <gtk/gtk.h>


typedef struct _WsImage WsImage;

typedef struct
{
  GPtrArray *images;
} WsGallery;

WsGallery *                 ws_gallery_new              (void);
void                        ws_gallery_append_image     (WsGallery *self,
                                                         WsImage   *image);
guint                       ws_gallery_get_n_images     (WsGallery *self);
WsImage *                   ws_gallery_get_image        (WsGallery *self,
                                                         guint      image_index);


struct _WsImage
{
  char *id;
  char *title;
  char *link;
  int width;
  int height;
  guint index;
  //ImgurAlbum *album;
  GdkPaintable *paintable; /* NULL if not loaded! */

  guint is_animated : 1;
  guint loaded : 1;
};

WsImage *                 ws_image_new_from_json          (JsonObject *json);
