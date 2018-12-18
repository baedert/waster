#ifndef WASTER_IMAGE_LOADER__
#define WASTER_IMAGE_LOADER__

#include <glib-object.h>
#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <cairo-gobject.h>
#include <cairo/cairo.h>


struct _ImgurImage
{
  gchar *id;
  gchar *title;
  gchar *link;
  int width;
  int height;
  guint is_animated : 1;
  guint is_album : 1;
  GdkTexture *texture;

  int n_subimages;
  struct _ImgurImage **subimages;
  int index; /* Index of this image in the array of its parent */
};

typedef struct _ImgurImage ImgurImage;



struct _WsImageLoader
{
  GObject parent_instance;
  ImgurImage **images;
  guint n_images;
  guint current;
};




#define WS_TYPE_IMAGE_LOADER ws_image_loader_get_type ()

G_DECLARE_FINAL_TYPE (WsImageLoader, ws_image_loader, WS, IMAGE_LOADER, GObject);


WsImageLoader *ws_image_loader_new ();

void ws_image_loader_load_gallery_async (WsImageLoader       *loader,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             user_data);
void ws_image_loader_load_gallery_finish (WsImageLoader  *loader,
                                          GAsyncResult   *result,
                                          GError        **error);


void ws_image_loader_load_image_async (WsImageLoader       *loader,
                                       ImgurImage          *image,
                                       GCancellable        *cancellable,
                                       GAsyncReadyCallback  callback,
                                       gpointer             user_data);
ImgurImage * ws_image_loader_load_image_finish (WsImageLoader  *loader,
                                               GAsyncResult    *result,
                                               GError         **error);
#endif
