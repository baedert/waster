#include <math.h>

#include "waster-media.h"


G_DEFINE_TYPE (WsImageView, ws_image_view, GTK_TYPE_WIDGET);


GtkWidget *
ws_image_view_new (int width, int height)
{
  GObject *obj = g_object_new (WS_TYPE_IMAGE_VIEW,
                               "vexpand", TRUE,
                               NULL);
  WS_IMAGE_VIEW (obj)->surface_width = width;
  WS_IMAGE_VIEW (obj)->surface_height = height;

  return GTK_WIDGET (obj);
}

void
ws_image_view_set_surface_size (WsImageView *view,
                                int          width,
                                int          height)
{
  view->surface_width = width;
  view->surface_height = height;
}


void
ws_image_view_set_surface (WsImageView     *view,
                           cairo_surface_t *surface)
{
  view->surface = surface;

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

static double
get_scale (WsImageView *view)
{
  int widget_width = gtk_widget_get_allocated_width (GTK_WIDGET (view));
  int widget_height = gtk_widget_get_allocated_height (GTK_WIDGET (view));
  int image_width = view->surface_width;
  int image_height = view->surface_height;

  double hscale = (double)widget_width / (double)image_width;
  double vscale = (double)widget_height / (double)image_height;

  return MIN (hscale, MIN (vscale, 1));
}

static gboolean
ws_image_view_draw (GtkWidget *widget, cairo_t *ct)
{
  WsImageView *view = WS_IMAGE_VIEW (widget);
  int x = 0;
  int y = 0;
  int widget_width = gtk_widget_get_allocated_width (widget);
  int widget_height = gtk_widget_get_allocated_height (widget);

  double scale = get_scale (view);
  x = (widget_width - (view->surface_width * scale)) / 2;
  y = (widget_height - (view->surface_height * scale)) / 2;


  if (view->surface)
    {
      cairo_rectangle (ct, 0, 0, widget_width, widget_height);
      cairo_scale (ct, scale, scale);
      cairo_set_source_surface (ct, view->surface, x / scale, y / scale);
      cairo_fill (ct);
    }


  return GDK_EVENT_PROPAGATE;
}

static void
ws_image_view_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
  gtk_widget_set_allocation (widget, allocation);
}

static void
ws_image_view_get_preferred_height (GtkWidget *widget,
                                    int *min,
                                    int *nat)
{
  WsImageView *view = WS_IMAGE_VIEW (widget);
  double scale = get_scale (WS_IMAGE_VIEW (widget));

  *nat = view->surface_height * scale;
  *min = 0;
}

static void
ws_image_view_get_preferred_width (GtkWidget *widget,
                                    int *min,
                                    int *nat)
{
  WsImageView *view = WS_IMAGE_VIEW (widget);
  double scale = get_scale (WS_IMAGE_VIEW (widget));

  *nat  = view->surface_width * scale;
  *min = 0;
}

static void
ws_image_view_class_init (WsImageViewClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  widget_class->size_allocate = ws_image_view_size_allocate;
  widget_class->draw = ws_image_view_draw;
  widget_class->get_preferred_height = ws_image_view_get_preferred_height;
  widget_class->get_preferred_width  = ws_image_view_get_preferred_width;
}

static void
ws_image_view_init (WsImageView *view)
{
  gtk_widget_set_has_window (GTK_WIDGET (view), FALSE);
}




G_DEFINE_TYPE (WsVideoView, ws_video_view, GTK_TYPE_FRAME);

GtkWidget *
ws_video_view_new ()
{
  return GTK_WIDGET (g_object_new (WS_TYPE_VIDEO_VIEW, NULL));
}

static void
ws_video_view_class_init (WsVideoViewClass *class)
{

}

static void
ws_video_view_init (WsVideoView *view)
{

}

