
#pragma once
#include <gtk/gtk.h>
#include "CbAnimation.h"


struct _WsNotification
{
  GtkWidget parent_instance;

  GtkWidget *label;

  guint visible: 1;

  CbAnimation show_animation;
};

typedef struct _WsNotification WsNotification;


#define WS_TYPE_NOTIFICATION ws_notification_get_type ()

G_DECLARE_FINAL_TYPE (WsNotification, ws_impostor, WS, NOTIFICATION, GtkWidget);

GtkWidget *      ws_notification_new         (const char     *message);
const char *     ws_notification_get_message (WsNotification *self);

void             ws_notification_show        (WsNotification *self);
void             ws_notification_hide        (WsNotification *self);
gboolean         ws_notification_get_visible (WsNotification *self);
