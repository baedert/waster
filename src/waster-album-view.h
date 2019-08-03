
#pragma once
#include <gtk/gtk.h>
#include "waster-image-loader.h"
#include "CbAnimation.h"

struct _WsAlbumView
{
  GtkWidget parent_instance;

  ImgurAlbum *album;

  int n_images;
  int cur_image_index;
  ImgurImage *cur_image;

  GtkWidget *image;
  GtkWidget *other_image;

  CbAnimation album_animation;  /* When setting a new album */
  CbAnimation scroll_animation; /* When scrolling through an album */

  float arrow_down_scale;
  GdkTexture *arrow_down_texture;
  CbAnimation arrow_activate_animation;

  guint muted: 1;
  GtkWidget *muted_image;
};

typedef struct _WsAlbumView WsAlbumView;

#define WS_TYPE_ALBUM_VIEW ws_album_view_get_type()

G_DECLARE_FINAL_TYPE (WsAlbumView, ws_album_view, WS, ALBUM_VIEW, GtkWidget);

void     ws_album_view_set_album         (WsAlbumView  *self,
                                          ImgurAlbum   *album);
void     ws_album_view_show_image        (WsAlbumView  *self,
                                          ImgurImage   *image);
void     ws_album_view_clear             (WsAlbumView  *self);
void     ws_album_view_scroll_to_next    (WsAlbumView  *self);
void     ws_album_view_scroll_to_prev    (WsAlbumView  *self);
void     ws_album_view_set_muted         (WsAlbumView  *self,
                                          gboolean      muted);
