#include <math.h>
#include <gtkimageview.h>

#include "waster-media.h"


G_DEFINE_TYPE (WsImageView, ws_image_view, GTK_TYPE_IMAGE_VIEW);


GtkWidget *
ws_image_view_new (int width, int height)
{
  GObject *obj = g_object_new (WS_TYPE_IMAGE_VIEW,
                               "vexpand", TRUE,
                               "fit-allocation", TRUE,
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

  gtk_image_view_set_surface (GTK_IMAGE_VIEW (view), surface);
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
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  widget_class->get_preferred_height = ws_image_view_get_preferred_height;
  widget_class->get_preferred_width  = ws_image_view_get_preferred_width;
}

static void
ws_image_view_init (WsImageView *view)
{
  gtk_widget_set_has_window (GTK_WIDGET (view), FALSE);
}


/* ------------------------------------------------------------------------------------------------- */

G_DEFINE_TYPE (WsVideoView, ws_video_view, GTK_TYPE_FRAME);


static gboolean
bus_cb (GstBus *bus, GstMessage *message, gpointer user_data)
{
  WsVideoView *view = user_data;

  switch (message->type)
    {
      case GST_MESSAGE_EOS:
        gst_element_seek (view->src, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                          GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

        break;
      case GST_MESSAGE_ERROR:
          {
            GError *error = NULL;
            char *debug;
            gst_message_parse_error (message, &error, &debug);
            g_warning ("%s(%d): %s", __FILE__, __LINE__, debug);
            g_warning ("%s", error->message);
            break;
          }
      default:
        return TRUE;
    }

  return TRUE;
}



void
ws_video_view_set_image (WsVideoView *view, ImgurImage *image)
{
  guint flags;
  g_return_if_fail (image != NULL);

  g_assert (image->is_animated);

  /* TODO: Reuse/free old resources */

  view->src  = gst_element_factory_make ("playbin", "video");
  view->sink = gst_element_factory_make ("gtksink", "sink");

  g_object_get (view->sink, "widget", &view->area, NULL);
  g_assert (GTK_IS_DRAWING_AREA (view->area));

  g_object_set (view->src, "video-sink", view->sink, NULL);
  g_object_set (view->src, "uri", image->link, NULL);

  g_object_get (view->src, "flags", &flags, NULL);
  g_object_set (view->src, "flags", flags | (1 << 7)/*GST_PLAY_FLAG_DOWNLOAD*/, NULL);

  gst_bus_add_watch (gst_element_get_bus (view->src), bus_cb, view);


  gtk_container_add (GTK_CONTAINER (view), view->area);
  gtk_widget_show (view->area);

  if (gtk_widget_get_mapped (view->area))
    {
      gst_element_set_state (view->src, GST_STATE_PLAYING);
    }
}

static void
ws_video_view_finalize (GObject *object)
{
  WsVideoView *view = WS_VIDEO_VIEW (object);

  if (view->src != NULL)
    {
      gst_element_set_state (view->src, GST_STATE_NULL);
      g_object_unref (view->src);
    }

  G_OBJECT_CLASS (ws_video_view_parent_class)->finalize (object);
}

GtkWidget *
ws_video_view_new ()
{
  return GTK_WIDGET (g_object_new (WS_TYPE_VIDEO_VIEW, NULL));
}

static void
ws_video_view_class_init (WsVideoViewClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = ws_video_view_finalize;
}

static void
ws_video_view_init (WsVideoView *view)
{

}

