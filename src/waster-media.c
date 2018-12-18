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
}

static void
ws_image_view_init (WsImageView *view)
{
  gtk_widget_set_has_surface (GTK_WIDGET (view), FALSE);

  view->picture = gtk_picture_new ();
  gtk_widget_set_parent (view->picture, GTK_WIDGET (view));
}
