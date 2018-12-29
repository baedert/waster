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
  const int widget_height = gtk_widget_get_height (GTK_WIDGET (view));
  const double current_value = gtk_adjustment_get_value (view->vadjustment);
  int current_visible;

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
  const int widget_height = gtk_widget_get_height ((GtkWidget *)self);
  double value = 0.0;
  double max_value;

  gtk_adjustment_set_upper (self->vadjustment, widget_height * self->n_widgets);
  gtk_adjustment_set_page_size (self->vadjustment, widget_height);
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

      if (album->images[i].paintable != NULL)
        ws_image_view_set_contents (WS_IMAGE_VIEW (self->widgets[i]),
                                    album->images[i].paintable);
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

static void
scroll_animate_func (CbAnimation *animation,
                     double       t,
                     gpointer     user_data)
{
  WsAlbumView *self = (WsAlbumView *)animation->owner;

  gtk_adjustment_set_value (self->vadjustment,
                            self->scroll_start_value + t * (self->scroll_end_value - self->scroll_start_value));

  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

void
ws_album_view_scroll_to_next (WsAlbumView *self)
{
  const int widget_height = gtk_widget_get_height (GTK_WIDGET (self));
  const int current_visible = current_visible_image (self);
  int height = 0;

  height = (current_visible * widget_height) + widget_height;


  self->scroll_start_value = gtk_adjustment_get_value (self->vadjustment);
  self->scroll_end_value = height;

  /* TODO: Check for animated animations and whatever */
  if (cb_animation_is_running (&self->scroll_animation))
    cb_animation_stop (&self->scroll_animation);

  cb_animation_start (&self->scroll_animation, NULL);

  /* Kick off */
  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

void
ws_album_view_scroll_to_prev (WsAlbumView *view)
{
  int widget_height = gtk_widget_get_height (GTK_WIDGET (view));
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

  ws_album_view_update_adjustments (self);

  y = - (int)gtk_adjustment_get_value (self->vadjustment);

  for (i = 0; i < self->n_widgets; i ++)
    {
      int nat_width, nat_height;
      int final_width = -1;
      int final_height = -1;

      gtk_widget_measure (self->widgets[i], GTK_ORIENTATION_HORIZONTAL, -1,
                          NULL, &nat_width, NULL, NULL);

      gtk_widget_measure (self->widgets[i], GTK_ORIENTATION_VERTICAL, -1,
                          NULL, &nat_height, NULL, NULL);

      if (nat_width > width)
        {
          final_width = width; /* Force into parent allocation */

          /* Re-negotiate natural height */
          gtk_widget_measure (self->widgets[i], GTK_ORIENTATION_VERTICAL, final_width,
                              NULL, &nat_height, NULL, NULL);
        }
      else
        {
          final_width = nat_width;
        }

      /* Same vertically */
      if (nat_height > height)
        {
          final_height = height;
          /* Re-negotiate natural height */
          gtk_widget_measure (self->widgets[i], GTK_ORIENTATION_HORIZONTAL, final_height,
                              NULL, &nat_width, NULL, NULL);
          final_width = nat_width;
        }
      else
        {
          final_height = nat_height;
        }

      gtk_widget_size_allocate (self->widgets[i],
                                &(GtkAllocation) {
                                  ceil ((width - final_width) / 2.0),
                                  ceil (y + (height - final_height) / 2.0),
                                  final_width,
                                  final_height
                                }, -1);

      y += height;
    }
}

static void
ws_album_view_snapshot (GtkWidget   *widget,
                        GtkSnapshot *snapshot)
{
  WsAlbumView *self = WS_ALBUM_VIEW (widget);
  GtkAdjustment *adjustment = self->vadjustment;
  const int width = gtk_widget_get_width (widget);
  const int height = gtk_widget_get_height (widget);

  gtk_snapshot_push_clip (snapshot,
                          &GRAPHENE_RECT_INIT (0, 0, width, height));

  GTK_WIDGET_CLASS (ws_album_view_parent_class)->snapshot (widget, snapshot);
  gtk_snapshot_pop (snapshot);

  if (gtk_adjustment_get_value (adjustment) <
      gtk_adjustment_get_upper (adjustment) - gtk_adjustment_get_page_size (adjustment))
    {
      const int twidth = gdk_texture_get_width (self->arrow_down_texture);
      const int theight = gdk_texture_get_height (self->arrow_down_texture);

      gtk_snapshot_append_texture (snapshot, self->arrow_down_texture,
                                   &GRAPHENE_RECT_INIT (
                                     width - twidth,
                                     height - theight,
                                     twidth, theight
                                   ));
    }
}

static void
ws_album_view_finalize (GObject *object)
{
  WsAlbumView *self = WS_ALBUM_VIEW (object);
  int i;

  for (i = 0; i < self->n_widgets; i ++)
    gtk_widget_unparent (self->widgets[i]);

  g_free (self->widgets);

  G_OBJECT_CLASS (ws_album_view_parent_class)->finalize (object);
}

static void
ws_album_view_init (WsAlbumView *self)
{
  gtk_widget_set_has_surface (GTK_WIDGET (self), FALSE);
  self->widgets = NULL;
  self->n_widgets = 0;

  self->arrow_down_texture = gdk_texture_new_from_resource ("/org/baedert/waster/data/arrow-down.png");

  cb_animation_init (&self->scroll_animation, GTK_WIDGET (self), scroll_animate_func);
}

static void
ws_album_view_class_init (WsAlbumViewClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  object_class->set_property = ws_album_view_set_property;
  object_class->get_property = ws_album_view_get_property;
  object_class->finalize     = ws_album_view_finalize;

  widget_class->measure = ws_album_view_measure;
  widget_class->size_allocate = ws_album_view_size_allocate;
  widget_class->snapshot = ws_album_view_snapshot;

  g_object_class_override_property (object_class, PROP_HADJUSTMENT, "hadjustment");
  g_object_class_override_property (object_class, PROP_VADJUSTMENT, "vadjustment");
  g_object_class_override_property (object_class, PROP_HSCROLL_POLICY, "hscroll-policy");
  g_object_class_override_property (object_class, PROP_VSCROLL_POLICY, "vscroll-policy");

  gtk_widget_class_set_css_name (widget_class, "album");
}
