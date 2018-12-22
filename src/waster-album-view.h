#ifndef __ALBUM_VIEW
#define __ALBUM_VIEW

#include <gtk/gtk.h>

#include "waster-image-loader.h"

struct _WsAlbumView
{
  GtkBox parent_instance;

  GtkAdjustment *hadjustment;
  GtkAdjustment *vadjustment;

  ImgurAlbum *album;

  ImgurImage *cur_image;

  int n_widgets;
  GtkWidget **widgets;

  int cur_visible_image;
};

typedef struct _WsAlbumView WsAlbumView;

#define WS_TYPE_ALBUM_VIEW ws_album_view_get_type()

G_DECLARE_FINAL_TYPE (WsAlbumView, ws_album_view, WS, ALBUM_VIEW, GtkWidget);

void     ws_album_view_reserve_space     (WsAlbumView  *self,
                                          ImgurAlbum   *album);
void     ws_album_view_show_image        (WsAlbumView  *self,
                                          ImgurImage   *image);
void     ws_album_view_clear             (WsAlbumView  *self);
void     ws_album_view_scroll_to_next    (WsAlbumView  *self);
void     ws_album_view_scroll_to_prev    (WsAlbumView  *self);


#endif