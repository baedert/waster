
#pragma once
#include <gtk/gtk.h>
#include "waster-image-loader.h"
#include "waster-gallery.h"
#include "CbAnimation.h"

struct _WsAlbumView
{
  GtkWidget parent_instance;

  WsImage *cur_image;

  GtkWidget *image;
  GtkWidget *other_image;

  CbAnimation scroll_animation; /* When scrolling from image to image */

  float arrow_down_scale;
  GdkTexture *arrow_down_texture;
  CbAnimation arrow_activate_animation;

  guint muted: 1;
  GtkWidget *muted_image;
};

typedef struct _WsAlbumView WsAlbumView;

#define WS_TYPE_ALBUM_VIEW ws_album_view_get_type()

G_DECLARE_FINAL_TYPE (WsAlbumView, ws_album_view, WS, ALBUM_VIEW, GtkWidget);

void     ws_album_view_show_image        (WsAlbumView  *self,
                                          WsImage      *image);
void     ws_album_view_clear             (WsAlbumView  *self);
void     ws_album_view_scroll_to_next    (WsAlbumView  *self);
void     ws_album_view_scroll_to_prev    (WsAlbumView  *self);
void     ws_album_view_set_muted         (WsAlbumView  *self,
                                          gboolean      muted);
