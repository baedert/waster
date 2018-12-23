
#include "waster-impostor.h"

G_DEFINE_TYPE (WsImpostor, ws_impostor, GTK_TYPE_WIDGET);


void
ws_impostor_clone (WsImpostor *self,
                   GtkWidget  *widget)
{
  g_clear_object (&self->paintable);

  self->paintable = gtk_widget_paintable_new (widget);
}

static void
ws_impostor_snapshot (GtkWidget   *widget,
                      GtkSnapshot *snapshot)
{
  WsImpostor *self = (WsImpostor *)widget;

  if (self->paintable)
    {
      gdk_paintable_snapshot (self->paintable,
                              snapshot,
                              gtk_widget_get_width (widget),
                              gtk_widget_get_height (widget));
    }

  g_message ("%s, %p", __FUNCTION__, self->paintable);
}

static void
ws_impostor_class_init (WsImpostorClass *class)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  widget_class->snapshot = ws_impostor_snapshot;
}

static void
ws_impostor_init (WsImpostor *impostor)
{
  gtk_widget_set_has_surface (GTK_WIDGET (impostor), FALSE);
}
