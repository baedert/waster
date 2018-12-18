#include <math.h>

#include "waster-album-view.h"
#include "waster-media.h"



G_DEFINE_TYPE_WITH_CODE (WsAlbumView, ws_album_view, GTK_TYPE_CONTAINER,
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

static void __empty (GtkContainer *container, GtkWidget *child)
{
  g_critical ("NOPE.");
}

static void
ws_album_view_value_changed_cb (GtkAdjustment *adjustment, gpointer user_data)
{
  WsAlbumView *view = user_data;

  gtk_widget_queue_allocate (GTK_WIDGET (view));
}

static void
get_visible_size (WsAlbumView *view,
                  ImgurImage  *image,
                  int         *visible_width,
                  int         *visible_height,
                  double      *scale)
{
  int widget_width = gtk_widget_get_allocated_width (GTK_WIDGET (view));
  int widget_height = gtk_widget_get_allocated_height (GTK_WIDGET (view));

  double hscale = (double)widget_width / (double)image->width;
  double vscale = (double)widget_height / (double)image->height;

  if (hscale < vscale)
    {
      *visible_width  = widget_width;
      *visible_height = (int)(image->height * hscale);

      if (scale) *scale = hscale;
    }
  else
    {
      *visible_width  = (int)(image->width * vscale);
      *visible_height = widget_height;

      if (scale) *scale = vscale;
    }
}

static void
ws_album_view_update_adjustments (WsAlbumView *view)
{
  int height = 0;
  double value = 0.0;
  double max_value;

  if (view->cur_image->is_album)
    {
      height = gtk_widget_get_allocated_height (GTK_WIDGET (view)) * view->n_images;
    }
  else
    {
      int w, h;
      get_visible_size (view, view->cur_image, &w, &h, NULL);
      height = h;
    }

  gtk_adjustment_set_upper (view->vadjustment, height);
  gtk_adjustment_set_page_size (view->vadjustment, gtk_widget_get_allocated_height (GTK_WIDGET (view)));
  value = gtk_adjustment_get_value (view->vadjustment);
  max_value = gtk_adjustment_get_upper (view->vadjustment) -
              gtk_adjustment_get_page_size (view->vadjustment);


  if (value > max_value)
    gtk_adjustment_set_value (view->vadjustment, max_value);
}

static void
ws_album_view_set (WsAlbumView *view,
                   int          pos,
                   GtkWidget   *widget)
{
  view->images[pos] = widget;

  gtk_widget_set_parent (widget, GTK_WIDGET (view));
}

void
ws_album_view_clear (WsAlbumView *view)
{
  int i;

  for (i = 0; i < view->n_images; i ++)
    gtk_widget_unparent (view->images[i]);

  g_free (view->images);
  view->images = NULL;
  view->n_images = 0;
}

void
ws_album_view_reserve_space (WsAlbumView *view,
                             ImgurImage  *image)
{
  int i;

  g_return_if_fail (WS_IS_ALBUM_VIEW (view));

  ws_album_view_clear (view);
  view->cur_image = image;


  if (image->is_album)
    {
      view->n_images = image->n_subimages;
      view->images = g_malloc (image->n_subimages * sizeof (GtkWidget *));
      for (i = 0; i < image->n_subimages; i ++)
        {
          GtkWidget *content_view;
          ImgurImage *img = image->subimages[i];
          g_assert (img != NULL);

          content_view = ws_image_view_new (image->subimages[i]->width,
                                            image->subimages[i]->height);
          ws_album_view_set (view, i, content_view);
        }
    }
  else
    {
      GtkWidget *content_view;
      view->n_images = 1;
      view->images = g_malloc (1 * sizeof (GtkImage *));

      g_message ("%s: %d×%d", __FILE__, image->width, image->height);
      content_view = ws_image_view_new (image->width, image->height);

      ws_album_view_set (view, 0, content_view);
    }

  ws_album_view_update_adjustments (view);
  gtk_adjustment_set_value (view->vadjustment, 0);
}

void
ws_album_view_show_image (WsAlbumView *view,
                          ImgurImage  *image)
{
  int index;

  if (image->index == -1)
    index = 0;
  else
    index = image->index;

  g_assert (image->paintable);
  ws_image_view_set_contents (WS_IMAGE_VIEW (view->images[index]),
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
  WsAlbumView *view = WS_ALBUM_VIEW (widget);

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      int i, min, nat;

      for (i = 0; i < view->n_images; i ++)
        {
          gtk_widget_measure (view->images[i], GTK_ORIENTATION_HORIZONTAL, -1,
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

      for (i = 0; i < view->n_images; i ++)
        {
          gtk_widget_measure (view->images[i], GTK_ORIENTATION_VERTICAL, -1,
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
  WsAlbumView *view = WS_ALBUM_VIEW (widget);
  GtkAllocation child_allocation;
  int i, y;

  if (!view->cur_image) return;

  ws_album_view_update_adjustments (view);


  y = - (int)gtk_adjustment_get_value (view->vadjustment);
  for (i = 0; i < view->n_images; i ++)
    {
      child_allocation.x = 0;
      child_allocation.y = y;
      child_allocation.width = width;
      child_allocation.height = height;

      gtk_widget_size_allocate (view->images[i], &child_allocation, -1);

      y += child_allocation.height;
    }
}

static void
ws_album_view_finalize (GObject *object)
{
  WsAlbumView *view = WS_ALBUM_VIEW (object);

  g_free (view->images);
}

static void
ws_album_view_init (WsAlbumView *view)
{
  gtk_widget_set_has_surface (GTK_WIDGET (view), FALSE);
  view->images = NULL;
  view->n_images = 0;
}

static void
ws_album_view_class_init (WsAlbumViewClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (class);

  object_class->set_property = ws_album_view_set_property;
  object_class->get_property = ws_album_view_get_property;
  object_class->finalize     = ws_album_view_finalize;

  widget_class->size_allocate = ws_album_view_size_allocate;
  widget_class->measure = ws_album_view_measure;

  container_class->add = __empty;
  container_class->remove = __empty;

  g_object_class_override_property (object_class, PROP_HADJUSTMENT, "hadjustment");
  g_object_class_override_property (object_class, PROP_VADJUSTMENT, "vadjustment");
  g_object_class_override_property (object_class, PROP_HSCROLL_POLICY, "hscroll-policy");
  g_object_class_override_property (object_class, PROP_VSCROLL_POLICY, "vscroll-policy");
}
