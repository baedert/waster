#include <math.h>

#include "waster-album-view.h"
#include "waster-media.h"


#define ARROW_SCALE (0.5)
#define POINT(x, y) (graphene_point_t) {x, y}


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

  if (image->index == self->cur_image_index)
    ws_album_view_show_image (self, image);
}

void
ws_album_view_clear (WsAlbumView *self)
{
  self->n_images = 0;
  self->album = NULL;

  self->cur_image = NULL;
  self->cur_image_index = 0;

  ws_image_view_set_contents (WS_IMAGE_VIEW (self->image), NULL);
  ws_image_view_set_contents (WS_IMAGE_VIEW (self->other_image), NULL);
}

void
ws_album_view_set_album (WsAlbumView *self,
                         ImgurAlbum  *album)
{
  GdkPaintable *cur;
  gboolean use_animation;

  g_return_if_fail (WS_IS_ALBUM_VIEW (self));
  g_return_if_fail (album != NULL);

  use_animation = (self->album != NULL);
  cur = ws_image_view_get_contents (WS_IMAGE_VIEW (self->image));
  ws_album_view_clear (self);
  ws_image_view_set_contents (WS_IMAGE_VIEW (self->other_image), cur);

  self->album = album;
  self->n_images = album->n_images;

  imgur_album_set_image_loaded_callback (album, image_loaded_cb, self);

  if (use_animation)
    cb_animation_start (&self->album_animation, NULL);
}

void
ws_album_view_show_image (WsAlbumView *self,
                          ImgurImage  *image)
{
  g_assert (image->paintable);
  g_assert (image->index >= 0);

  ws_image_view_stop (WS_IMAGE_VIEW (self->image));

  ws_image_view_set_contents (WS_IMAGE_VIEW (self->image),
                              image->paintable);

  ws_image_view_start (WS_IMAGE_VIEW (self->image));
}


static void
album_animate_func (CbAnimation *animation,
                    double       t,
                    gpointer     user_data)
{
  WsAlbumView *self = (WsAlbumView *)animation->owner;

  gtk_widget_queue_allocate (GTK_WIDGET (self));
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
      GdkPaintable *cur;
      self->cur_image_index ++;

      cur = ws_image_view_get_contents (WS_IMAGE_VIEW (self->image));
      ws_image_view_set_contents (WS_IMAGE_VIEW (self->other_image), cur);

      ws_album_view_show_image (self, &self->album->images[self->cur_image_index]);

      if (cb_animation_is_running (&self->scroll_animation))
        cb_animation_stop (&self->scroll_animation);

      cb_animation_start (&self->scroll_animation, NULL);
      cb_animation_start (&self->arrow_activate_animation, NULL);

      gtk_widget_queue_allocate (GTK_WIDGET (self));
    }
}

void
ws_album_view_scroll_to_prev (WsAlbumView *self)
{

  if (self->cur_image_index > 0)
    {
      self->cur_image_index --;
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
get_image_size (GtkWidget *widget,
                int        viewport_width,
                int        viewport_height,
                int       *out_width,
                int       *out_height)
{
  int nat_width, nat_height;
  int final_width, final_height;

  gtk_widget_measure (widget, GTK_ORIENTATION_HORIZONTAL, -1,
                      NULL, &nat_width, NULL, NULL);

  gtk_widget_measure (widget, GTK_ORIENTATION_VERTICAL, -1,
                      NULL, &nat_height, NULL, NULL);

  if (nat_width > viewport_width)
    {
      final_width = viewport_width; /* Force into parent allocation */

      /* Re-negotiate natural height */
      gtk_widget_measure (widget, GTK_ORIENTATION_VERTICAL, final_width,
                          NULL, &nat_height, NULL, NULL);
    }
  else
    {
      final_width = nat_width;
    }

  /* Same vertically */
  if (nat_height > viewport_height)
    {
      final_height = viewport_height;
      /* Re-negotiate natural height */
      gtk_widget_measure (widget, GTK_ORIENTATION_HORIZONTAL, final_height,
                          NULL, &nat_width, NULL, NULL);
      final_width = nat_width;
    }
  else
    {
      final_height = nat_height;
    }

  *out_width = final_width;
  *out_height = final_height;
}

GskTransform *
get_image_transform (int           image_width,
                     int           image_height,
                     int           viewport_width,
                     int           viewport_height,
                     double        animation_progress,
                     GskTransform *start)
{
  const float deg = (1 - animation_progress) * (- 90.0f);
  GskTransform *t = start;

  t = gsk_transform_translate (t,
                               &(graphene_point_t) {
                                 viewport_width / 2.0f,
                                 viewport_height / 2.0f,
                               });

  t = gsk_transform_rotate (t, deg);

  t = gsk_transform_translate (t,
                               &(graphene_point_t) {
                                 - image_width / 2.0f,
                                 - image_height / 2.0f,
                               });

  return t;
}

static void
ws_album_view_size_allocate (GtkWidget *widget,
                             int        width,
                             int        height,
                             int        baseline)
{
  WsAlbumView *self= WS_ALBUM_VIEW (widget);
  int final_width = -1;
  int final_height = -1;

  get_image_size (self->image, width, height, &final_width, &final_height);

  if (cb_animation_is_running (&self->scroll_animation))
    {
      const double progress = self->scroll_animation.progress;
      GskTransform *t = NULL;
      int other_width, other_height;

      t = gsk_transform_translate (t,
                                   &(graphene_point_t) {
                                     0,
                                     height * (1 - progress)
                                   });

      t = get_image_transform (final_width, final_height,
                               width, height,
                               progress,
                               t);
      gtk_widget_allocate (self->image,
                           final_width,
                           final_height,
                           -1,
                           t);
      t = NULL;

      /* Same for the other image */
      get_image_size (self->other_image, width, height, &other_width, &other_height);

      t = gsk_transform_translate (t,
                                   &(graphene_point_t) {
                                     0,
                                     - width * progress
                                   });

      t = get_image_transform (other_width, other_height,
                               width, height,
                               1 - progress,
                               t);
      gtk_widget_allocate (self->other_image,
                           other_width,
                           other_height,
                           -1,
                           t);
    }
  else if (cb_animation_is_running (&self->album_animation))
    {
      const double progress = self->album_animation.progress;
      GskTransform *t = NULL;

      t = gsk_transform_translate (t, &POINT ( final_width / 2.0, final_height / 2.0));
      t = gsk_transform_scale (t, 1.2 - (0.2 * progress), 1.2 - (0.2 * progress));
      t = gsk_transform_translate (t, &POINT (-final_width / 2.0, -final_height / 2.0));
      t = gsk_transform_translate (t,
                                   &(graphene_point_t) {
                                     ceil ((width - final_width) / 2.0) + (width * (1 - progress)),
                                     ceil ((height - final_height) / 2.0)
                                   });

      gtk_widget_allocate (self->image, final_width, final_height, -1, t);
      t = NULL;

      get_image_size (self->other_image, width, height, &final_width, &final_height);

      t = gsk_transform_translate (t, &POINT ( final_width / 2.0, final_height / 2.0));
      t = gsk_transform_scale (t, 1.0 - (0.2 * progress), 1.0 - (0.2 * progress));
      t = gsk_transform_translate (t, &POINT (-final_width / 2.0, -final_height / 2.0));
      t = gsk_transform_translate (t,
                                   &(graphene_point_t) {
                                     ceil ((width - final_width) / 2.0) - (width * (progress)),
                                     ceil ((height - final_height) / 2.0)
                                   });

      gtk_widget_allocate (self->other_image, final_width, final_height, -1, t);
    }
  else
    {
      gtk_widget_size_allocate (self->image,
                                &(GtkAllocation) {
                                  ceil ((width - final_width) / 2.0),
                                  ceil ((height - final_height) / 2.0),
                                  final_width,
                                  final_height
                                }, -1);
    }

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
      gtk_widget_snapshot_child (widget, self->image, snapshot);

      if (cb_animation_is_running (&self->scroll_animation) ||
          cb_animation_is_running (&self->album_animation))
        gtk_widget_snapshot_child (widget, self->other_image, snapshot);
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
                              &(graphene_point_t) {width - twidth, height - theight});
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

  g_clear_object (&self->arrow_down_texture);

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

  cb_animation_init (&self->album_animation, GTK_WIDGET (self), album_animate_func);
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
