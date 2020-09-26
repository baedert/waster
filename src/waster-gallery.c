#include "waster-gallery.h"
#include "waster-placeholder.h"


WsGallery *
ws_gallery_new (void)
{
  WsGallery *self = g_malloc0 (sizeof (WsGallery));

  self->images = g_ptr_array_new ();

  return self;
}

void
ws_gallery_append_image (WsGallery *self,
                         WsImage   *image)
{
  g_assert (self);
  g_assert (image);

  g_ptr_array_add (self->images, image);
}

guint
ws_gallery_get_n_images (WsGallery *self)
{
  g_assert (self);

  return self->images->len;
}

WsImage *
ws_gallery_get_image (WsGallery *self,
                      guint      image_index)
{
  g_assert (image_index < ws_gallery_get_n_images (self));

  return g_ptr_array_index (self->images, image_index);
}


/////////////////////////////////////////////////////////////////


WsImage *
ws_image_new_from_json (JsonObject *json)
{
  WsImage *self;

  g_assert (json);

  self = g_malloc0 (sizeof (WsImage));

  self->id = g_strdup (json_object_get_string_member (json, "id"));

  if (json_object_has_member (json, "width"))
    {
      self->width  = json_object_get_int_member (json, "width");
      self->height = json_object_get_int_member (json, "height");
      self->paintable = ws_placeholder_new (self->width, self->height);
    }
  else
    {
      self->paintable = ws_placeholder_new (200, 200);
    }

  g_assert (self->paintable);

  if (json_object_get_null_member (json, "title"))
    self->title = NULL;
  else
    self->title = g_strdup (json_object_get_string_member (json, "title"));

  if (json_object_has_member (json, "animated"))
    self->is_animated = json_object_get_boolean_member (json, "animated");
  else
    self->is_animated = FALSE;

  if (self->is_animated)
    self->link = g_strdup (json_object_get_string_member (json, "mp4"));
  else
    self->link = g_strdup (json_object_get_string_member (json, "link"));

  return self;
}
