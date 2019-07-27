
#include "waster-impostor.h"

G_DEFINE_TYPE (WsImpostor, ws_impostor, GTK_TYPE_WIDGET);


void
ws_impostor_clone (WsImpostor *self,
                   GtkWidget  *widget)
{
  GdkPaintable *p;

  g_clear_object (&self->paintable);

  p = gtk_widget_paintable_new (widget);
  self->paintable = gdk_paintable_get_current_image (p);

  g_object_unref (p);
}

static void
ws_impostor_snapshot (GtkWidget   *widget,
                      GtkSnapshot *snapshot)
{
  WsImpostor *self = (WsImpostor *)widget;
  const int width = gtk_widget_get_width (widget);
  const int height = gtk_widget_get_height (widget);

  if (self->paintable)
    {
      gdk_paintable_snapshot (self->paintable,
                              snapshot,
                              width, height);
    }
}

static void
ws_impostor_finalize (GObject *object)
{
  WsImpostor *self = WS_IMPOSTOR (object);

  g_clear_object (&self->paintable);

  G_OBJECT_CLASS (ws_impostor_parent_class)->finalize (object);
}

static void
ws_impostor_class_init (WsImpostorClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  object_class->finalize = ws_impostor_finalize;

  widget_class->snapshot = ws_impostor_snapshot;
}

static void
ws_impostor_init (WsImpostor *impostor)
{
}
