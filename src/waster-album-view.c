#include <math.h>

#include "waster-album-view.h"
#include "waster-media.h"
#include "waster.h"


#define POINT(x, y) (graphene_point_t) {x, y}


G_DEFINE_TYPE (WsAlbumView, ws_album_view, GTK_TYPE_WIDGET);

enum {
  PROP_0,
};

static void
scroll_animate_func (CbAnimation *animation,
                     double       t,
                     gpointer     user_data)
{
  WsAlbumView *self = (WsAlbumView *)animation->owner;

  gtk_widget_queue_allocate (GTK_WIDGET (self));
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
  int m, n;

  gtk_widget_measure (self->image, GTK_ORIENTATION_VERTICAL, -1,
                      minimum, natural, NULL, NULL);

  /* Meh */
  gtk_widget_measure (self->muted_image, GTK_ORIENTATION_VERTICAL, -1,
                      &m, &n, NULL, NULL);
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

      t = gsk_transform_translate (t,
                                   &(graphene_point_t) {
                                     ceil ((width - final_width) / 2.0) + (width * (1 - progress)),
                                     ceil ((height - final_height) / 2.0)
                                   });
      t = gsk_transform_translate (t, &POINT ( final_width / 2.0, final_height / 2.0));
      t = gsk_transform_scale (t, 1.5 - (0.5 * progress), 1.5 - (0.5 * progress));
      t = gsk_transform_translate (t, &POINT (-final_width / 2.0, -final_height / 2.0));

      gtk_widget_allocate (self->image, final_width, final_height, -1, t);
      t = NULL;

      get_image_size (self->other_image, width, height, &final_width, &final_height);

      t = gsk_transform_translate (t,
                                   &(graphene_point_t) {
                                     ceil ((width - final_width) / 2.0) - (width * (progress)),
                                     ceil ((height - final_height) / 2.0)
                                   });
      t = gsk_transform_translate (t, &POINT ( final_width / 2.0, final_height / 2.0));
      t = gsk_transform_scale (t, 1.0 - (0.5 * progress), 1.0 - (0.5 * progress));
      t = gsk_transform_translate (t, &POINT (-final_width / 2.0, -final_height / 2.0));

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

  gtk_widget_measure (self->muted_image, GTK_ORIENTATION_HORIZONTAL, -1,
                      &final_width, NULL, NULL, NULL);
  gtk_widget_measure (self->muted_image, GTK_ORIENTATION_VERTICAL, final_width,
                      &final_height, NULL, NULL, NULL);
  gtk_widget_size_allocate (self->muted_image,
                            &(GtkAllocation) {
                              width - final_width, 0,
                              final_width, final_height
                            }, -1);


}

static void
ws_album_view_snapshot (GtkWidget   *widget,
                        GtkSnapshot *snapshot)
{
  WsAlbumView *self = WS_ALBUM_VIEW (widget);

  gtk_widget_snapshot_child (widget, self->image, snapshot);

  if (cb_animation_is_running (&self->scroll_animation))
    gtk_widget_snapshot_child (widget, self->other_image, snapshot);

  gtk_widget_snapshot_child (widget, self->muted_image, snapshot);
}

static void
ws_album_view_finalize (GObject *object)
{
  WsAlbumView *self = WS_ALBUM_VIEW (object);

  gtk_widget_unparent (self->image);
  gtk_widget_unparent (self->other_image);
  gtk_widget_unparent (self->muted_image);

  g_clear_object (&self->arrow_down_texture);

  G_OBJECT_CLASS (ws_album_view_parent_class)->finalize (object);
}

static void
ws_album_view_init (WsAlbumView *self)
{
  GSettings *settings;

  gtk_widget_set_overflow (GTK_WIDGET (self), GTK_OVERFLOW_HIDDEN);

  self->image = ws_image_view_new ();
  gtk_widget_set_parent (self->image, (GtkWidget *)self);
  self->other_image = ws_image_view_new ();
  gtk_widget_set_parent (self->other_image, (GtkWidget *)self);

  self->arrow_down_texture = gdk_texture_new_from_resource ("/org/baedert/waster/data/arrow-down.png");
  self->arrow_down_scale = 1.0;

  cb_animation_init (&self->scroll_animation, GTK_WIDGET (self), scroll_animate_func);

  self->muted_image = gtk_image_new ();
  gtk_style_context_add_class (gtk_widget_get_style_context (self->muted_image), "muted");
  gtk_widget_set_parent (self->muted_image, (GtkWidget *)self);

  settings = ((Waster *)(g_application_get_default ()))->settings;
  self->muted = g_settings_get_boolean (settings, "muted");
  ws_album_view_set_muted (self, self->muted);
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

void
ws_album_view_clear (WsAlbumView *self)
{
  self->cur_image = NULL;

  ws_image_view_set_contents (WS_IMAGE_VIEW (self->image), NULL, NULL);
  ws_image_view_set_contents (WS_IMAGE_VIEW (self->other_image), NULL, NULL);
}

void
ws_album_view_show_image (WsAlbumView *self,
                          WsImage     *image)
{
  GdkPaintable *cur;

  g_assert (image);
  g_assert (image->paintable);
  g_assert (image->index >= 0);

  cur = ws_image_view_get_contents (WS_IMAGE_VIEW (self->image));
  ws_image_view_set_contents (WS_IMAGE_VIEW (self->other_image), cur, NULL);

  if (cb_animation_is_running (&self->scroll_animation))
    cb_animation_stop (&self->scroll_animation);

  if (cur)
    cb_animation_start (&self->scroll_animation, NULL);

  ws_image_view_set_contents (WS_IMAGE_VIEW (self->image),
                              image->paintable,
                              image->title);

  ws_image_view_start (WS_IMAGE_VIEW (self->image));
  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

void
ws_album_view_set_muted (WsAlbumView  *self,
                         gboolean      muted)
{
  self->muted = muted;

  ws_image_view_set_muted (WS_IMAGE_VIEW (self->image), muted);
  ws_image_view_set_muted (WS_IMAGE_VIEW (self->other_image), muted);

  if (muted)
    gtk_image_set_from_icon_name ((GtkImage *)self->muted_image,
                                  "audio-volume-muted-symbolic");
  else
    gtk_image_set_from_icon_name ((GtkImage *)self->muted_image,
                                  "audio-volume-high-symbolic");

  /* But then... */
  gtk_widget_set_visible (self->muted_image, muted);
}
