#include "imgur-gallery.h"


void
imgur_album_set_image_loaded_callback (ImgurAlbum               *self,
                                       ImgurAlbumImageLoadedFunc callback,
                                       gpointer                  user_data)
{
  self->image_loaded_callback = callback;
  self->image_loaded_user_data = user_data;
}

void
imgur_album_notify_image_loaded (ImgurAlbum *self,
                                 int         image_index)
{
  if (!self->image_loaded_callback)
    return;

  self->image_loaded_callback (self,
                               &self->images[image_index],
                               self->image_loaded_user_data);
}
