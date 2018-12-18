#include <math.h>

#include "waster-media.h"


G_DEFINE_TYPE (WsImageView, ws_image_view, GTK_TYPE_WIDGET);


GtkWidget *
ws_image_view_new (int width,
                   int height)
{
  GObject *obj = g_object_new (WS_TYPE_IMAGE_VIEW,
                               NULL);
  WS_IMAGE_VIEW (obj)->surface_width = width;
  WS_IMAGE_VIEW (obj)->surface_height = height;

  return GTK_WIDGET (obj);
}

void
ws_image_view_set_cotent_size (WsImageView *view,
                               int          width,
                               int          height)
{
  view->surface_width = width;
  view->surface_height = height;
}


void
ws_image_view_set_contents (WsImageView  *view,
                            GdkPaintable *paintable)
{
  gtk_picture_set_paintable (GTK_PICTURE (view->picture),
                             paintable);
}

static void
ws_image_view_measure (GtkWidget      *widget,
                       GtkOrientation  orientation,
                       int             for_size,
                       int            *minimum,
                       int            *natural,
                       int            *minimum_baseline,
                       int            *natural_baseline)
{
  WsImageView *view = WS_IMAGE_VIEW (widget);

  gtk_widget_measure (view->picture, orientation, for_size,
                      minimum, natural, minimum_baseline, natural_baseline);
}

static void
ws_image_view_size_allocate (GtkWidget *widget,
                             int        width,
                             int        height,
                             int        baseline)
{
  WsImageView *view = WS_IMAGE_VIEW (widget);

  gtk_widget_size_allocate (view->picture,
                            &(GtkAllocation) { 0, 0, width, height },
                            -1);

}

static void
ws_image_view_class_init (WsImageViewClass *class)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  widget_class->measure = ws_image_view_measure;
  widget_class->size_allocate = ws_image_view_size_allocate;
}

static void
ws_image_view_init (WsImageView *view)
{
  gtk_widget_set_has_surface (GTK_WIDGET (view), FALSE);

  view->picture = gtk_picture_new ();
  gtk_widget_set_parent (view->picture, GTK_WIDGET (view));
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

