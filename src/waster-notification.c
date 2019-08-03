#include "waster-notification.h"

G_DEFINE_TYPE (WsNotification, ws_notification, GTK_TYPE_WIDGET);

static void
ws_notification_dispose (GObject *object)
{
  WsNotification *self = (WsNotification *)object;

  g_clear_pointer (&self->label, gtk_widget_unparent);

  G_OBJECT_CLASS (ws_notification_parent_class)->dispose (object);
}

static void
ws_notification_measure (GtkWidget      *widget,
                         GtkOrientation  orientation,
                         int             for_size,
                         int            *minimum,
                         int            *natural,
                         int            *minimum_baseline,
                         int            *natural_baseline)
{
  WsNotification *self = (WsNotification *)widget;

  gtk_widget_measure (self->label, orientation, for_size,
                      minimum, natural, minimum_baseline, natural_baseline);
}

static void
ws_notification_size_allocate (GtkWidget *widget,
                               int        width,
                               int        height,
                               int        baseline)
{
  WsNotification *self = (WsNotification *)widget;

  if (cb_animation_is_running (&self->show_animation))
    {
      const double progress = self->show_animation.progress;
      GskTransform *t = NULL;

      t = gsk_transform_translate (t,
                                   &(graphene_point_t) {
                                     0,
                                     - height * (1 - progress)
                                   });
      t = gsk_transform_translate (t, &(graphene_point_t) { width / 2, height / 2});
      t = gsk_transform_scale (t, progress, progress);
      t = gsk_transform_rotate (t, 180 * (1 - progress));
      t = gsk_transform_translate (t, &(graphene_point_t) { -width / 2, -height / 2});

      gtk_widget_allocate (self->label, width, height, -1, t);
    }
  else
    {
      gtk_widget_size_allocate (self->label,
                                &(GtkAllocation) { 0, 0, width, height }, -1);
    }
}

#if 0
static void
ws_notification_snapshot (GtkWidget   *widget,
                          GtkSnapshot *snapshot)
{
  const int width = gtk_widget_get_width (widget);
  const int height = gtk_widget_get_height (widget);

  gtk_snapshot_push_clip (snapshot, &GRAPHENE_RECT_INIT (0, 0, width, height));
  GTK_WIDGET_CLASS (ws_notification_parent_class)->snapshot (widget, snapshot);
  gtk_snapshot_pop (snapshot);
}
#endif

static void
show_animate_func (CbAnimation *animation,
                   double       t,
                   gpointer     user_data)
{
  gtk_widget_set_opacity (animation->owner, t);
  gtk_widget_queue_allocate (animation->owner);
}

static void
ws_notification_class_init (WsNotificationClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = (GtkWidgetClass *)klass;

  object_class->dispose = ws_notification_dispose;

  widget_class->measure = ws_notification_measure;
  widget_class->size_allocate = ws_notification_size_allocate;
  /*widget_class->snapshot = ws_notification_snapshot;*/

  gtk_widget_class_set_css_name (widget_class, "notification");
}

static void
ws_notification_init (WsNotification *self)
{
  self->label = gtk_label_new ("");
  gtk_widget_set_parent (self->label, (GtkWidget *)self);

  cb_animation_init (&self->show_animation, (GtkWidget *)self, show_animate_func);
}

GtkWidget *
ws_notification_new (const char *message)
{
  WsNotification *self = (WsNotification *)g_object_new (WS_TYPE_NOTIFICATION, NULL);

  gtk_label_set_label ((GtkLabel *)self->label, message);

  return (GtkWidget *)self;
}

const char *
ws_notification_get_message (WsNotification *self)
{
  g_return_val_if_fail (WS_IS_NOTIFICATION (self), NULL);

  return gtk_label_get_label ((GtkLabel *)self->label);
}

void
ws_notification_show (WsNotification *self)
{
  gtk_widget_show (self->label);
  self->visible = TRUE;
  cb_animation_start (&self->show_animation, NULL);
}

void
ws_notification_hide (WsNotification *self)
{
  self->visible = FALSE;
}

gboolean
ws_notification_get_visible (WsNotification *self)
{
  return self->visible;
}

