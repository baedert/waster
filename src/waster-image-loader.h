#pragma once

#include <glib-object.h>
#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <cairo-gobject.h>
#include <cairo/cairo.h>
#include "imgur-gallery.h"

struct _WsImageLoader
{
  GObject parent_instance;
};

#define WS_TYPE_IMAGE_LOADER ws_image_loader_get_type ()

G_DECLARE_FINAL_TYPE (WsImageLoader, ws_image_loader, WS, IMAGE_LOADER, GObject);


WsImageLoader *   ws_image_loader_new ();

void              ws_image_loader_load_gallery_async      (WsImageLoader                *loader,
                                                           const ImgurGalleryDefinition *gallery,
                                                           GCancellable                 *cancellable,
                                                           GAsyncReadyCallback           callback,
                                                           gpointer                      user_data);
ImgurGallery *    ws_image_loader_load_gallery_finish     (WsImageLoader                *loader,
                                                           GAsyncResult                 *result,
                                                           GError                      **error);

void              ws_image_loader_load_album_async        (WsImageLoader                *loader,
                                                           ImgurAlbum                   *album,
                                                           GCancellable                 *cancellable,
                                                           GAsyncReadyCallback           callback,
                                                           gpointer                      user_data);
ImgurAlbum *      ws_image_loader_load_album_finish       (WsImageLoader                *loader,
                                                           GAsyncResult                 *result,
                                                           GError                      **error);

void              ws_image_loader_load_image_async        (WsImageLoader                *loader,
                                                           ImgurImage                   *image,
                                                           GCancellable                 *cancellable,
                                                           GAsyncReadyCallback           callback,
                                                           gpointer                      user_data);
ImgurImage *      ws_image_loader_load_image_finish       (WsImageLoader                *loader,
                                                           GAsyncResult                 *result,
                                                           GError                      **error);
