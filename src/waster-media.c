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
  gtk_picture_set_paintable (GTK_PICTURE (self->picture),
                             paintable);
}

static GtkSizeRequestMode
ws_image_view_get_request_mode (GtkWidget *widget)
{
  WsImageView *self = WS_IMAGE_VIEW (widget);

  return gtk_widget_get_request_mode (self->picture);
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
  WsImageView *self = WS_IMAGE_VIEW (widget);

  gtk_widget_measure (self->picture, orientation, for_size,
                      minimum, natural, minimum_baseline, natural_baseline);
}

static void
ws_image_view_size_allocate (GtkWidget *widget,
                             int        width,
                             int        height,
                             int        baseline)
{
  WsImageView *self = WS_IMAGE_VIEW (widget);

  gtk_widget_size_allocate (self->picture,
                            &(GtkAllocation) { 0, 0, width, height },
                            -1);

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

  widget_class->measure = ws_image_view_measure;
  widget_class->size_allocate = ws_image_view_size_allocate;
  widget_class->get_request_mode = ws_image_view_get_request_mode;

  gtk_widget_class_set_css_name (widget_class, "media");
}

static void
ws_image_view_init (WsImageView *self)
{
  gtk_widget_set_has_surface (GTK_WIDGET (self), FALSE);

  self->picture = gtk_picture_new ();
  gtk_widget_set_parent (self->picture, GTK_WIDGET (self));
}
