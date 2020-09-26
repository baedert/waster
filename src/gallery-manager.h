
#pragma once

#include "waster-image-loader.h"
#include "waster-gallery.h"

struct _WsGalleryManager
{
  GObject parent_instance;
  ImgurGallery *gallery; /* Current gallery */
  WsImageLoader *loader;
};

#define WS_TYPE_GALLERY_MANAGER (ws_gallery_manager_get_type ())

G_DECLARE_FINAL_TYPE (WsGalleryManager, ws_gallery_manager, WS, GALLERY_MANAGER, GObject);


WsImageLoader *   ws_image_loader_new ();




WsGalleryManager * ws_gallery_manager_new                    (void);
void               ws_gallery_manager_load_gallery_async     (WsGalleryManager             *self,
                                                              const ImgurGalleryDefinition *def,
                                                              GCancellable                 *cancellable,
                                                              GAsyncReadyCallback           callback,
                                                              gpointer                      user_data);
WsGallery *        ws_gallery_manager_load_gallery_finish    (WsGalleryManager             *self,
                                                              GAsyncResult                 *result,
                                                              GError                      **error);

void               ws_gallery_manager_load_image_async       (WsGalleryManager             *self,
                                                              WsGallery                    *gallery,
                                                              guint                         image_index,
                                                              GCancellable                 *cancellable,
                                                              GAsyncReadyCallback           callback,
                                                              gpointer                      user_data);
WsImage *          ws_gallery_manager_load_image_finish      (WsGalleryManager             *self,
                                                              GAsyncResult                 *result,
                                                              GError                      **error);


