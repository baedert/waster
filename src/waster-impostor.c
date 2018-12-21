
#include "waster-impostor.h"

G_DEFINE_TYPE (WsImpostor, ws_impostor, GTK_TYPE_WIDGET);




void
ws_impostor_clone (WsImpostor *impostor, GtkWidget *widget)
{
  cairo_t *context;

  /*if (!widget)*/

  /*g_warning ("TODO: Fix %s", __FUNCTION__);*/
  return;

#if 0
  if (impostor->surface)
    cairo_surface_destroy (impostor->surface);

  impostor->w = gtk_widget_get_allocated_width (widget);
  impostor->h = gtk_widget_get_allocated_height (widget);

  impostor->surface = gdk_window_create_similar_surface (gtk_widget_get_window (widget),
                                                         CAIRO_CONTENT_COLOR_ALPHA,
                                                         impostor->w, impostor->h);

  g_assert (impostor->surface != NULL);

  context = cairo_create (impostor->surface);

  g_assert (context != NULL);

  gtk_widget_draw (widget, context);

  cairo_destroy (context);
#endif
}


/*static void*/
/*get_preferred_width (GtkWidget *widget,*/
                     /*int       *min,*/
                     /*int       *nat)*/
/*{*/
  /*WsImpostor *imp = WS_IMPOSTOR (widget);*/

  /*if (!imp->surface)*/
    /*{*/
      /**min = 1;*/
      /**nat = 1;*/
    /*}*/
  /*else*/
    /*{*/
      /**min = imp->w;*/
      /**nat = imp->w;*/
    /*}*/
/*}*/


/*static void*/
/*get_preferred_height (GtkWidget *widget,*/
                      /*int       *min,*/
                      /*int       *nat)*/
/*{*/
  /*WsImpostor *imp = WS_IMPOSTOR (widget);*/

  /*if (!imp->surface)*/
    /*{*/
      /**min = 1;*/
      /**nat = 1;*/
    /*}*/
  /*else*/
    /*{*/
      /**min = imp->h;*/
      /**nat = imp->h;*/
    /*}*/
/*}*/


/*static gboolean*/
/*draw (GtkWidget *widget,*/
      /*cairo_t   *ct)*/
/*{*/
  /*WsImpostor *imp = WS_IMPOSTOR (widget);*/

  /*if (imp->surface)*/
    /*{*/
      /*cairo_rectangle (ct, 0, 0, gtk_widget_get_allocated_width (widget),*/
                       /*gtk_widget_get_allocated_height (widget));*/
      /*cairo_set_source_surface (ct, imp->surface, 0, 0);*/
      /*cairo_fill (ct);*/
    /*}*/

  /*return GDK_EVENT_PROPAGATE;*/
/*}*/

static void
ws_impostor_class_init (WsImpostorClass *class)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  /*widget_class->get_preferred_width  = get_preferred_width;*/
  /*widget_class->get_preferred_height = get_preferred_height;*/
  /*widget_class->draw                 = draw;*/
}

static void
ws_impostor_init (WsImpostor *impostor)
{
  gtk_widget_set_has_surface (GTK_WIDGET (impostor), FALSE);
}
