#ifndef __MEDIA
#define __MEDIA

#include <gtk/gtk.h>
#include <cairo/cairo.h>

struct _WsImageView
{
  GtkWidget parent_instance;
  cairo_surface_t *surface;
  int surface_width;
  int surface_height;
};

struct _WsVideoView
{
  GtkFrame parent_instance;
};


typedef struct _WsImageView WsImageView;
typedef struct _WsVideoView WsVideoView;



#define WS_TYPE_IMAGE_VIEW ws_image_view_get_type()

G_DECLARE_FINAL_TYPE (WsImageView, ws_image_view, WS, IMAGE_VIEW, GtkWidget);

GtkWidget *ws_image_view_new ();
void ws_image_view_set_surface (WsImageView *image_view, cairo_surface_t *surface);
void ws_image_view_set_surface_size (WsImageView *image_view, int width, int height);




#define WS_TYPE_VIDEO_VIEW ws_video_view_get_type()
G_DECLARE_FINAL_TYPE (WsVideoView, ws_viewo_view, WS, VIDEO_VIEW, GtkFrame);

GtkWidget *ws_video_view_new ();


#endif
