#include <math.h>

#include "waster-media.h"


G_DEFINE_TYPE (WsImageView, ws_image_view, GTK_TYPE_WIDGET);


GtkWidget *
ws_image_view_new (int width,
                   int height)
{
  GObject *obj = g_object_new (WS_TYPE_IMAGE_VIEW,
                               NULL);

  return GTK_WIDGET (obj);
}

void
ws_image_view_set_contents (WsImageView  *self,
                            GdkPaintable *paintable)
{
  ws_image_view_stop (self);

  gtk_picture_set_paintable (GTK_PICTURE (self->picture),
                             paintable);

  ws_image_view_set_muted (self, self->muted);
}

GdkPaintable *
ws_image_view_get_contents (WsImageView *self)
{
  return gtk_picture_get_paintable (GTK_PICTURE (self->picture));
}

static void
ws_image_view_finalize (GObject *object)
{
  WsImageView *self = WS_IMAGE_VIEW (object);

  g_clear_pointer (&self->picture, gtk_widget_unparent);

  G_OBJECT_CLASS (ws_image_view_parent_class)->finalize (object);
}

static void
ws_image_view_class_init (WsImageViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ws_image_view_finalize;

  gtk_widget_class_set_css_name (widget_class, "media");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}

static void
ws_image_view_init (WsImageView *self)
{
  gtk_widget_set_overflow (GTK_WIDGET (self), GTK_OVERFLOW_HIDDEN);

  self->picture = gtk_picture_new ();
  gtk_widget_set_parent (self->picture, GTK_WIDGET (self));
}

void
ws_image_view_start (WsImageView *self)
{
  GdkPaintable *p = gtk_picture_get_paintable (GTK_PICTURE (self->picture));

  if (p && GTK_IS_MEDIA_STREAM (p))
    {
      gtk_media_stream_set_loop (GTK_MEDIA_STREAM (p), TRUE);
      gtk_media_stream_play (GTK_MEDIA_STREAM (p));
    }
}

void
ws_image_view_stop (WsImageView  *self)
{
  GdkPaintable *p = gtk_picture_get_paintable (GTK_PICTURE (self->picture));

  if (p && GTK_IS_MEDIA_STREAM (p))
    {
      gtk_media_stream_pause (GTK_MEDIA_STREAM (p));
    }
}

void
ws_image_view_set_muted (WsImageView *self,
                         gboolean     muted)
{
  GdkPaintable *p = gtk_picture_get_paintable (GTK_PICTURE (self->picture));

  if (p && GTK_IS_MEDIA_STREAM (p))
    {
      gtk_media_stream_set_muted (GTK_MEDIA_STREAM (p), muted);
    }

  self->muted = muted;
}
