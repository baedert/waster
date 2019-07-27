#ifndef __IMGUR_GALLERY_H__
#define __IMGUR_GALLERY_H__

#include <glib.h>
#include <gtk/gtk.h>


typedef struct _ImgurAlbum ImgurAlbum;

struct _ImgurImage
{
  char *id;
  char *title;
  char *link;
  int width;
  int height;
  guint index;
  ImgurAlbum *album;
  GdkPaintable *paintable; /* NULL if not loaded! */

  guint is_animated : 1;
  guint loaded : 1;
};

typedef struct _ImgurImage ImgurImage;


typedef void (*ImgurAlbumImageLoadedFunc) (ImgurAlbum *album,
                                           ImgurImage *image,
                                           gpointer    user_data);

struct _ImgurAlbum {
  int n_images;
  ImgurImage *images;

  char id[64];
  char *title;

  guint loaded : 1;

  ImgurAlbumImageLoadedFunc image_loaded_callback;
  gpointer image_loaded_user_data;
};

void imgur_album_set_image_loaded_callback (ImgurAlbum                *self,
                                            ImgurAlbumImageLoadedFunc  callback,
                                            gpointer                   user_data);
void imgur_album_notify_image_loaded       (ImgurAlbum                *self,
                                            int                        image_index);


typedef struct
{
  const char *name;
  const char *function;
} ImgurGalleryDefinition;

static const ImgurGalleryDefinition IMGUR_GALLERIES[] = {
  { "Hot", "gallery/hot/viral/%d.json" }
};


typedef struct
{
  const ImgurGalleryDefinition *definition;

  int n_albums;
  ImgurAlbum *albums;
} ImgurGallery;



#endif
