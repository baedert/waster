#include <math.h>

#include "waster-album-view.h"
#include "waster-media.h"



G_DEFINE_TYPE_WITH_CODE (WsAlbumView, ws_album_view, GTK_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_SCROLLABLE, NULL));

enum {
  PROP_0,
  PROP_HADJUSTMENT,
  PROP_VADJUSTMENT,
  PROP_VSCROLL_POLICY,
  PROP_HSCROLL_POLICY
};

static int
current_visible_image (WsAlbumView *view)
{
  int current_visible;
  int widget_height = gtk_widget_get_allocated_height (GTK_WIDGET (view));
  double current_value = gtk_adjustment_get_value (view->vadjustment);

  current_visible = current_value / widget_height;

  return current_visible;
}

static void
ws_album_view_value_changed_cb (GtkAdjustment *adjustment, gpointer user_data)
{
  WsAlbumView *view = user_data;

  gtk_widget_queue_allocate (GTK_WIDGET (view));
}

static void
ws_album_view_update_adjustments (WsAlbumView *self)
{
  int height = 0;
  double value = 0.0;
  double max_value;

  height = gtk_widget_get_height (GTK_WIDGET (self)) * self->n_widgets;

  gtk_adjustment_set_upper (self->vadjustment, height);
  gtk_adjustment_set_page_size (self->vadjustment, gtk_widget_get_allocated_height (GTK_WIDGET (self)));
  value = gtk_adjustment_get_value (self->vadjustment);
  max_value = gtk_adjustment_get_upper (self->vadjustment) -
              gtk_adjustment_get_page_size (self->vadjustment);


  if (value > max_value)
    gtk_adjustment_set_value (self->vadjustment, max_value);
}

void
ws_album_view_clear (WsAlbumView *self)
{
  int i;

  for (i = 0; i < self->n_widgets; i ++)
    gtk_widget_unparent (self->widgets[i]);

  g_free (self->widgets);
  self->widgets = NULL;
  self->n_widgets = 0;
}

void
ws_album_view_reserve_space (WsAlbumView *self,
                             ImgurAlbum  *album)
{
  int i;

  g_return_if_fail (WS_IS_ALBUM_VIEW (self));

  g_message ("%s!!!!!", __FUNCTION__);

  ws_album_view_clear (self);
  self->album = album;

  self->n_widgets = album->n_images;
  self->widgets = g_malloc (album->n_images * sizeof (GtkWidget *));

  for (i = 0; i < album->n_images; i ++)
    {
      GtkWidget *content_view;

      content_view = ws_image_view_new (album->images[i].width,
                                        album->images[i].height);

      self->widgets[i] = content_view;
      gtk_widget_set_parent (content_view, GTK_WIDGET (self));
    }

  ws_album_view_update_adjustments (self);
  gtk_adjustment_set_value (self->vadjustment, 0);
}

void
ws_album_view_show_image (WsAlbumView *self,
                          ImgurImage  *image)
{
  g_assert (image->paintable);
  g_assert (image->index >= 0);

  ws_image_view_set_contents (WS_IMAGE_VIEW (self->widgets[image->index]),
                              image->paintable);
}

void
ws_album_view_scroll_to_next (WsAlbumView *view)
{
  int height = 0;
  int widget_height = gtk_widget_get_allocated_height (GTK_WIDGET (view));
  int current_visible = current_visible_image (view);

  height = (current_visible * widget_height) + widget_height;

  gtk_adjustment_set_value (view->vadjustment, height);
}

void
ws_album_view_scroll_to_prev (WsAlbumView *view)
{
  int widget_height = gtk_widget_get_allocated_height (GTK_WIDGET (view));
  int height = 0;
  int current_visible = current_visible_image (view);//, &height);

  g_assert (current_visible >= 0);

  if (current_visible >= 1)
    {
      int cur_height = current_visible * widget_height;
      height = cur_height - widget_height;
    }

  gtk_adjustment_set_value (view->vadjustment, height);

}



static void
ws_album_view_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  switch (prop_id)
    {
      case PROP_HADJUSTMENT:
        WS_ALBUM_VIEW (object)->hadjustment = g_value_get_object (value);
        break;
      case PROP_VADJUSTMENT:
        WS_ALBUM_VIEW (object)->vadjustment = g_value_get_object (value);
        if (WS_ALBUM_VIEW (object)->vadjustment)
          g_signal_connect (G_OBJECT (WS_ALBUM_VIEW (object)->vadjustment), "value-changed",
                            G_CALLBACK (ws_album_view_value_changed_cb), object);
        break;
      case PROP_HSCROLL_POLICY:
      case PROP_VSCROLL_POLICY:
        break;
    }
}

static void
ws_album_view_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  switch (prop_id)
    {
      case PROP_HADJUSTMENT:
        g_value_set_object (value, WS_ALBUM_VIEW (object)->hadjustment);
        break;
      case PROP_VADJUSTMENT:
        g_value_set_object (value, WS_ALBUM_VIEW (object)->vadjustment);
        break;
      case PROP_HSCROLL_POLICY:
      case PROP_VSCROLL_POLICY:
        break;
    }
}

static void
ws_album_view_measure (GtkWidget      *widget,
                       GtkOrientation  orientation,
                       int             for_size,
                       int            *minimum,
                       int            *natural,
                       int            *minimum_baseline,
                       int            *natural_baseline)
{
  WsAlbumView *self = WS_ALBUM_VIEW (widget);

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      int i, min, nat;

      for (i = 0; i < self->n_widgets; i ++)
        {
          gtk_widget_measure (self->widgets[i], GTK_ORIENTATION_HORIZONTAL, -1,
                              &min, &nat, NULL, NULL);

          *minimum = MAX (min, *minimum);
          *natural = MAX (nat, *natural);
        }
    }
  else
    {
      int i, min, nat;
      *minimum = 0;
      *natural = 0;

      for (i = 0; i < self->n_widgets; i ++)
        {
          gtk_widget_measure (self->widgets[i], GTK_ORIENTATION_VERTICAL, -1,
                              &min, &nat, NULL, NULL);

          *minimum += min;
          *natural += nat;
        }
    }

}

static void
ws_album_view_size_allocate (GtkWidget *widget,
                             int        width,
                             int        height,
                             int        baseline)
{
  WsAlbumView *self= WS_ALBUM_VIEW (widget);
  int i, y;

  /*if (!self->cur_image)*/
      /*return;*/

  ws_album_view_update_adjustments (self);

  y = - (int)gtk_adjustment_get_value (self->vadjustment);
  for (i = 0; i < self->n_widgets; i ++)
    {
      gtk_widget_size_allocate (self->widgets[i],
                                &(GtkAllocation) { 0, y, width, height }, -1);

      y += height;
    }
}

static void
ws_album_view_finalize (GObject *object)
{
  WsAlbumView *self = WS_ALBUM_VIEW (object);

  g_free (self->widgets);
}

static void
ws_album_view_init (WsAlbumView *self)
{
  gtk_widget_set_has_surface (GTK_WIDGET (self), FALSE);
  self->widgets = NULL;
  self->n_widgets = 0;
}

static void
ws_album_view_class_init (WsAlbumViewClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  object_class->set_property = ws_album_view_set_property;
  object_class->get_property = ws_album_view_get_property;
  object_class->finalize     = ws_album_view_finalize;

  widget_class->size_allocate = ws_album_view_size_allocate;
  widget_class->measure = ws_album_view_measure;

  g_object_class_override_property (object_class, PROP_HADJUSTMENT, "hadjustment");
  g_object_class_override_property (object_class, PROP_VADJUSTMENT, "vadjustment");
  g_object_class_override_property (object_class, PROP_HSCROLL_POLICY, "hscroll-policy");
  g_object_class_override_property (object_class, PROP_VSCROLL_POLICY, "vscroll-policy");
}
