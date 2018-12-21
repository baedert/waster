#ifndef __IMGUR_GALLERY_H__
#define __IMGUR_GALLERY_H__

#include <glib.h>
#include <gtk/gtk.h>



struct _ImgurImage
{
  char *id;
  char *title;
  char *link;
  int width;
  int height;
  guint is_animated : 1;
  guint is_album : 1;        /* TODO: REMOVE */
  guint index;
  GdkPaintable *paintable;
};

typedef struct _ImgurImage ImgurImage;



typedef struct {
  int n_images;
  ImgurImage *images;

  char id[64];
  char *title;

} ImgurAlbum;

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
