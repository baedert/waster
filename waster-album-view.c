#include "waster-album-view.h"


G_DEFINE_TYPE_WITH_CODE (WsAlbumView, ws_album_view, GTK_TYPE_BOX,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_SCROLLABLE, NULL));

enum {
  PROP_0,
  PROP_HADJUSTMENT,
  PROP_VADJUSTMENT,
  PROP_VSCROLL_POLICY,
  PROP_HSCROLL_POLICY
};


static void
remove_image (GtkWidget *widget, gpointer data)
{
  gtk_container_remove (data, widget);
}

static void
ws_album_view_value_changed_cb (GtkAdjustment *adjustment, gpointer user_data)
{
  WsAlbumView *view = user_data;

  gtk_widget_queue_draw (user_data);
}

static void
ws_album_view_upate_adjustments (WsAlbumView *view)
{
  int i;
  int height = 0;

  if (view->cur_image->is_album)
    {
      for (i = 0; i < view->cur_image->n_subimages; i ++)
        {
          height += view->cur_image->subimages[i]->height;
        }
    }
  else
    {
      height = view->cur_image->height;
    }

  gtk_adjustment_set_upper (view->vadjustment, height);
}


void
ws_album_view_reserve_space (WsAlbumView *view,
                             ImgurImage  *image)
{
  int i;

  g_return_if_fail (WS_IS_ALBUM_VIEW (view));

  gtk_container_foreach (GTK_CONTAINER (view),
                         remove_image, view);

  g_free (view->images);

  view->cur_image = image;

  if (image->is_album)
    {
      view->n_images = image->n_subimages;
      view->images = g_malloc (image->n_subimages * sizeof (GtkImage *));
      for (i = 0; i < image->n_subimages; i ++)
        {
          ImgurImage *img = image->subimages[i];
          g_assert (img != NULL);

          view->images[i] = gtk_image_new ();
          gtk_widget_show (view->images[i]);
          gtk_widget_set_size_request (GTK_WIDGET (view->images[i]), img->width, img->height);

          gtk_container_add (GTK_CONTAINER (view), view->images[i]);
        }

    }
  else
    {
      view->n_images = 1;
      view->images = g_malloc (1 * sizeof (GtkImage *));
      view->images[0] = gtk_image_new ();
      gtk_widget_show (view->images[0]);
      gtk_widget_set_size_request (GTK_WIDGET (view->images[0]), image->width, image->height);

      gtk_container_add (GTK_CONTAINER (view), view->images[0]);
    }

  ws_album_view_upate_adjustments (view);
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

  g_assert (image->surface);

  gtk_image_set_from_surface (GTK_IMAGE (view->images[index]),
                              image->surface);
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

static gboolean
ws_album_view_draw (GtkWidget *widget, cairo_t *ct)
{
  WsAlbumView *view = WS_ALBUM_VIEW (widget);
  double vvalue = gtk_adjustment_get_value (view->vadjustment);


  cairo_translate (ct, 0, -vvalue);
  GTK_WIDGET_CLASS (ws_album_view_parent_class)->draw (widget, ct);

  return GDK_EVENT_PROPAGATE;
}

static void
ws_album_view_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
  WsAlbumView *view = WS_ALBUM_VIEW (widget);

  gtk_adjustment_set_page_size (view->vadjustment, allocation->height);

  GTK_WIDGET_CLASS (ws_album_view_parent_class)->size_allocate (widget, allocation);
}


static void
ws_album_view_init (WsAlbumView *view)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (view), GTK_ORIENTATION_VERTICAL);
}

static void
ws_album_view_class_init (WsAlbumViewClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  object_class->set_property = ws_album_view_set_property;
  object_class->get_property = ws_album_view_get_property;

  widget_class->draw = ws_album_view_draw;
  widget_class->size_allocate = ws_album_view_size_allocate;

  g_object_class_override_property (object_class, PROP_HADJUSTMENT, "hadjustment");
  g_object_class_override_property (object_class, PROP_VADJUSTMENT, "vadjustment");
  g_object_class_override_property (object_class, PROP_HSCROLL_POLICY, "hscroll-policy");
  g_object_class_override_property (object_class, PROP_VSCROLL_POLICY, "vscroll-policy");
}
