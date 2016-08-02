#ifndef __ALBUM_VIEW
#define __ALBUM_VIEW

#include <gtk/gtk.h>

#include "waster-image-loader.h"

struct _WsAlbumView
{
  GtkBox parent_instance;

  GtkAdjustment *hadjustment;
  GtkAdjustment *vadjustment;

  ImgurImage *cur_image;
  GtkWidget **images;
  int n_images;

  int cur_visible_image;
};

typedef struct _WsAlbumView WsAlbumView;



/*
 * Plans:
 *
 * (1) Scale each widget/video to either be the real size (never upscale),
 *     or fit BOTH into the width and height of the window/widget.
 *     This way we make sure to always show the "entire image" when scrolling
 *     up/down using the cursor keys.
 *
 * (2) Support Videos! :(
 *
 *
 *
 */


#define WS_TYPE_ALBUM_VIEW ws_album_view_get_type()

G_DECLARE_FINAL_TYPE (WsAlbumView, ws_album_view, WS, ALBUM_VIEW, GtkBox);

void ws_album_view_reserve_space (WsAlbumView *view,
                                  ImgurImage  *image);

void ws_album_view_show_image (WsAlbumView *view,
                               ImgurImage  *image);

void ws_album_view_scroll_to_next (WsAlbumView *view);
void ws_album_view_scroll_to_prev (WsAlbumView *view);


#endif
