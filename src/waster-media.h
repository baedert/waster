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

  int surface_width;
  int surface_height;
};

struct _WsVideoView
{
  GtkFrame parent_instance;

  GtkWidget *area;
  GstElement *src;
  GstElement *sink;
};


typedef struct _WsImageView WsImageView;
typedef struct _WsVideoView WsVideoView;



#define WS_TYPE_IMAGE_VIEW ws_image_view_get_type()

G_DECLARE_FINAL_TYPE (WsImageView, ws_image_view, WS, IMAGE_VIEW, GtkWidget);

GtkWidget *ws_image_view_new ();
void       ws_image_view_set_contents     (WsImageView  *image_view,
                                           GdkPaintable *paintable);
void       ws_image_view_set_content_size (WsImageView  *image_view,
                                           int           width,
                                           int           height);




#define WS_TYPE_VIDEO_VIEW ws_video_view_get_type()
G_DECLARE_FINAL_TYPE (WsVideoView, ws_video_view, WS, VIDEO_VIEW, GtkFrame);

GtkWidget *ws_video_view_new ();
void       ws_video_view_set_image (WsVideoView *view, ImgurImage *image);


#endif
