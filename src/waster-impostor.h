
#pragma once
#include <gtk/gtk.h>
#include <cairo/cairo.h>


struct _WsImpostor
{
  GtkWidget parent_instance;

  GdkPaintable *paintable;
};

typedef struct _WsImpostor WsImpostor;


#define WS_TYPE_IMPOSTOR ws_impostor_get_type ()

G_DECLARE_FINAL_TYPE (WsImpostor, ws_impostor, WS, IMPOSTOR, GtkWidget);

void ws_impostor_clone (WsImpostor *impostor, GtkWidget *widget);
