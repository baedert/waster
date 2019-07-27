#include <math.h>

#include "waster-album-view.h"
#include "waster-media.h"


#define ARROW_SCALE (0.5)


G_DEFINE_TYPE (WsAlbumView, ws_album_view, GTK_TYPE_WIDGET);

enum {
  PROP_0,
};

static void
image_loaded_cb (ImgurAlbum *album,
                 ImgurImage *image,
                 gpointer    user_data)
{
  WsAlbumView *self = user_data;

  /*g_message ("##################################################################");*/
  g_message ("%s: %d, %d", __FUNCTION__, image->index, self->cur_image_index);

  if (image->index == self->cur_image_index)
    {
      ws_album_view_show_image (self, image);
    }
}

void
ws_album_view_clear (WsAlbumView *self)
{
  self->n_images = 0;
  self->album = NULL;

  self->cur_image = NULL;
  self->cur_image_index = 0;
}

void
ws_album_view_reserve_space (WsAlbumView *self,
                             ImgurAlbum  *album)
{
  g_return_if_fail (WS_IS_ALBUM_VIEW (self));

  ws_album_view_clear (self);

  self->album = album;
  self->n_images = album->n_images;

  imgur_album_set_image_loaded_callback (album, image_loaded_cb, self);
}

void
ws_album_view_show_image (WsAlbumView *self,
                          ImgurImage  *image)
{
  g_assert (image->paintable);
  g_assert (image->index >= 0);

  ws_image_view_set_contents (WS_IMAGE_VIEW (self->image),
                              image->paintable);
}

static void
scroll_animate_func (CbAnimation *animation,
                     double       t,
                     gpointer     user_data)
{
  WsAlbumView *self = (WsAlbumView *)animation->owner;

  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

static void
arrow_activate_func (CbAnimation *animation,
                     double       t,
                     gpointer     user_data)
{
  WsAlbumView *self = (WsAlbumView *)animation->owner;

  self->arrow_down_scale = 1.0 + ((sinf (t * G_PI)) * ARROW_SCALE);
}

void
ws_album_view_scroll_to_next (WsAlbumView *self)
{
  if (self->cur_image_index < self->n_images - 1)
    {
      self->cur_image_index ++;
      g_message ("%s: Image now: %d", __FUNCTION__, self->cur_image_index);
      ws_album_view_show_image (self, &self->album->images[self->cur_image_index]);
      gtk_widget_queue_allocate (GTK_WIDGET (self));
    }
}

void
ws_album_view_scroll_to_prev (WsAlbumView *self)
{

  if (self->cur_image_index > 0)
    {
      self->cur_image_index --;
      g_message ("%s: Image now: %d", __FUNCTION__, self->cur_image_index);
      ws_album_view_show_image (self, &self->album->images[self->cur_image_index]);
      gtk_widget_queue_allocate (GTK_WIDGET (self));
    }
}

static void
ws_album_view_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  switch (prop_id)
    {
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

  if (self->n_images == 0)
    return;

  gtk_widget_measure (self->image, GTK_ORIENTATION_VERTICAL, -1,
                      minimum, natural, NULL, NULL);
}

static void
ws_album_view_size_allocate (GtkWidget *widget,
                             int        width,
                             int        height,
                             int        baseline)
{
  WsAlbumView *self= WS_ALBUM_VIEW (widget);
  const int y = 0;

  /*for (i = 0; i < self->n_images; i ++)*/
    /*{*/
      GtkWidget *image = self->image;
      int nat_width, nat_height;
      int final_width = -1;
      int final_height = -1;

      gtk_widget_measure (image, GTK_ORIENTATION_HORIZONTAL, -1,
                          NULL, &nat_width, NULL, NULL);

      gtk_widget_measure (image, GTK_ORIENTATION_VERTICAL, -1,
                          NULL, &nat_height, NULL, NULL);

      if (nat_width > width)
        {
          final_width = width; /* Force into parent allocation */

          /* Re-negotiate natural height */
          gtk_widget_measure (image, GTK_ORIENTATION_VERTICAL, final_width,
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
          gtk_widget_measure (image, GTK_ORIENTATION_HORIZONTAL, final_height,
                              NULL, &nat_width, NULL, NULL);
          final_width = nat_width;
        }
      else
        {
          final_height = nat_height;
        }

      if (0 && cb_animation_is_running (&self->scroll_animation))
        {
          const float deg = (1 - self->scroll_animation.progress) * (-90.0f);
          GskTransform *t = NULL;

          t = gsk_transform_translate (t,
                                       &(graphene_point_t) {
                                         (width) / 2.0f,
                                         y + (height) / 2.0f,
                                       });

          t = gsk_transform_rotate (t, deg);

          t = gsk_transform_translate (t,
                                       &(graphene_point_t) {
                                         - final_width / 2.0f,
                                         - final_height / 2.0f,
                                       });

          gtk_widget_allocate (image,
                               final_width,
                               final_height,
                               -1,
                               t);
        }
      else
        {
          gtk_widget_size_allocate (image,
                                    &(GtkAllocation) {
                                      ceil ((width - final_width) / 2.0),
                                      ceil ((height - final_height) / 2.0),
                                      final_width,
                                      final_height
                                    }, -1);
        }

      /*y += height;*/
    /*}*/
}

static void
ws_album_view_snapshot (GtkWidget   *widget,
                        GtkSnapshot *snapshot)
{
  WsAlbumView *self = WS_ALBUM_VIEW (widget);
  const int width = gtk_widget_get_width (widget);
  const int height = gtk_widget_get_height (widget);

  if (self->n_images > 0)
    {
      GtkWidget *image = self->image;

      gtk_widget_snapshot_child (widget, image, snapshot);
    }

  if (self->cur_image_index < self->n_images - 1)
    {
      const int twidth = gdk_texture_get_width (self->arrow_down_texture);
      const int theight = gdk_texture_get_height (self->arrow_down_texture);
      GskTransform *transform;

      transform = gsk_transform_translate (NULL, &GRAPHENE_POINT_INIT (twidth / 2.0f, theight / 2.0));
      transform = gsk_transform_scale (transform, self->arrow_down_scale, self->arrow_down_scale);
      transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (- twidth / 2.0f, - theight / 2.0));

      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (snapshot,
                              &(graphene_point_t) {width - twidth, height - theight});//- offset, - offset);
      gtk_snapshot_transform (snapshot, transform);
      gtk_snapshot_push_opacity (snapshot, 0.7);
      gtk_snapshot_append_texture (snapshot, self->arrow_down_texture,
                                   &GRAPHENE_RECT_INIT (
                                     0, 0,
                                     twidth, theight
                                   ));
      gtk_snapshot_pop (snapshot);
      gtk_snapshot_restore (snapshot);

      gsk_transform_unref (transform);
    }
}

static void
ws_album_view_finalize (GObject *object)
{
  WsAlbumView *self = WS_ALBUM_VIEW (object);

  gtk_widget_unparent (self->image);
  gtk_widget_unparent (self->other_image);

  G_OBJECT_CLASS (ws_album_view_parent_class)->finalize (object);
}

static void
ws_album_view_init (WsAlbumView *self)
{
  gtk_widget_set_overflow (GTK_WIDGET (self), GTK_OVERFLOW_HIDDEN);

  self->n_images = 0;

  self->image = ws_image_view_new ();
  gtk_widget_set_parent (self->image, (GtkWidget *)self);
  self->other_image = ws_image_view_new ();
  gtk_widget_set_parent (self->other_image, (GtkWidget *)self);

  self->arrow_down_texture = gdk_texture_new_from_resource ("/org/baedert/waster/data/arrow-down.png");
  self->arrow_down_scale = 1.0;

  cb_animation_init (&self->scroll_animation, GTK_WIDGET (self), scroll_animate_func);
  cb_animation_init (&self->arrow_activate_animation, GTK_WIDGET (self), arrow_activate_func);
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

  gtk_widget_class_set_css_name (widget_class, "album");
}
