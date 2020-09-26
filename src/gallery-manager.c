
#include "gallery-manager.h"
#include "waster.h"
#include "waster-gallery.h"
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>

typedef struct
{
  WsImageLoader *loader;
  void *payload;
  GTask *task;
} LoaderData;

G_DEFINE_TYPE (WsGalleryManager, ws_gallery_manager, G_TYPE_OBJECT);

static void
ws_gallery_manager_init (WsGalleryManager *self)
{
}

static void
ws_gallery_manager_class_init (WsGalleryManagerClass *class)
{
}


WsGalleryManager *
ws_gallery_manager_new (void)
{
  return (WsGalleryManager *)g_object_new (WS_TYPE_GALLERY_MANAGER, NULL);
}

static void
gallery_call_finished_cb (GObject      *source_object,
                          GAsyncResult *result,
                          gpointer      user_data)
{
  RestProxyCall *call = REST_PROXY_CALL (source_object);
  LoaderData *data = user_data;
  GTask *task = data->task;
  GError *error = NULL;
  JsonParser *parser;
  JsonObject *root;
  int i, n_albums;
  JsonArray *data_array;
  WsGallery *gallery;

  rest_proxy_call_invoke_finish (call, result, &error);

  if (error)
    {
      g_task_return_error (task, error);
      goto out;
    }

  parser = json_parser_new ();
  json_parser_load_from_data (parser, rest_proxy_call_get_payload (call), -1, &error);

  if (error)
    {
      g_task_return_error (task, error);
      goto out;
    }

  root = json_node_get_object (json_parser_get_root (parser));

  if (!json_object_has_member (root, "data") ||
      !JSON_NODE_HOLDS_ARRAY (json_object_get_member (root, "data")))
    {
      g_critical ("%s: root is not an array.", __FUNCTION__);
      printf ("%s\n", rest_proxy_call_get_payload (call));
      goto out;
    }

  data_array = json_object_get_array_member (root, "data");
  n_albums = json_array_get_length (data_array);

  gallery = ws_gallery_new ();

  for (i = 0; i < n_albums; i ++)
    {
      JsonObject *json_object = json_array_get_object_element (data_array, i);

      if (json_object_has_member (json_object, "images"))
        {
          JsonArray *images_array;
          guint j, p;

          images_array = json_object_get_array_member (json_object, "images");

          p = json_array_get_length (images_array);
          for (j = 0; j < p; j ++)
            {
              WsImage *img = ws_image_new_from_json (json_array_get_object_element (images_array, j));

              ws_gallery_append_image (gallery, img);
            }
        }
      else
        {
          WsImage *img = ws_image_new_from_json (json_object);

          ws_gallery_append_image (gallery, img);
        }
    }

  g_object_unref (parser);
  g_object_unref (call);

  g_task_return_pointer (task, gallery, NULL);
  g_object_unref (task);

out:
  g_free (data);
}

void
ws_gallery_manager_load_gallery_async (WsGalleryManager             *self,
                                       const ImgurGalleryDefinition *def,
                                       GCancellable                 *cancellable,
                                       GAsyncReadyCallback           callback,
                                       gpointer                      user_data)
{
  char buff[4096];
  Waster *app = (Waster *)g_application_get_default ();
  GTask *task;
  LoaderData *data;
  RestProxyCall *call;

  g_snprintf (buff, sizeof (buff), def->function, 0);

  call = rest_proxy_new_call (app->proxy);
  rest_proxy_call_set_function (call, buff);
  rest_proxy_call_set_method (call, "GET");

  task = g_task_new (self, cancellable, callback, user_data);

  data = g_new (LoaderData, 1);
  data->task = task;

  rest_proxy_call_invoke_async (call,
                                cancellable,
                                gallery_call_finished_cb,
                                data);
}

WsGallery *
ws_gallery_manager_load_gallery_finish (WsGalleryManager  *self,
                                        GAsyncResult      *result,
                                        GError           **error)
{
  g_assert (g_task_is_valid (result, self));

  return g_task_propagate_pointer (G_TASK (result), error);
}

typedef struct
{
  GTask *task;
  SoupSession *session;
  WsImage *image;
} ImageLoadingData;


static void
soup_message_cb (SoupSession *session,
                 SoupMessage *message,
                 gpointer     user_data)
{
  ImageLoadingData *data = user_data;
  GTask *task = data->task;
  GCancellable *cancellable = g_task_get_cancellable (task);
  WsImage *image = data->image;
  GBytes *response;
  GInputStream *in_stream;
  GdkPixbuf *pixbuf;
  GError *error = NULL;
  GdkPaintable *paintable;

  g_object_get (message, "response-body-data", &response, NULL);
  in_stream = g_memory_input_stream_new_from_bytes (response);

  pixbuf = gdk_pixbuf_new_from_stream (in_stream, NULL, &error);

  if (cancellable != NULL && g_cancellable_is_cancelled (cancellable))
    {
      g_task_return_pointer (task, NULL, NULL);
      g_object_unref (task);
      return;
    }

  if (error)
    {
      g_task_return_error (task, error);
      g_object_unref (task);

      g_input_stream_close (in_stream, NULL, NULL);
      g_object_unref (in_stream);
      g_bytes_unref (response);
      return;
    }

  paintable = GDK_PAINTABLE (gdk_texture_new_for_pixbuf (pixbuf));

  if (cancellable != NULL && g_cancellable_is_cancelled (cancellable))
    {
      g_task_return_pointer (task, NULL, NULL);
      g_object_unref (task);
      g_object_unref (paintable);
      return;
    }


  g_clear_object (&image->paintable);
  image->paintable = paintable;
  image->loaded = TRUE;
  /*imgur_album_notify_image_loaded (image->album, image->index);*/

  g_input_stream_close (in_stream, NULL, NULL);
  g_object_unref (in_stream);

  g_object_unref (pixbuf);
  g_bytes_unref (response);

  g_task_return_pointer (task, image, NULL);
  g_object_unref (task);

  g_free (data);
}

void
ws_gallery_manager_load_image_async (WsGalleryManager    *self,
                                     WsGallery           *gallery,
                                     guint                image_index,
                                     GCancellable        *cancellable,
                                     GAsyncReadyCallback  callback,
                                     gpointer             user_data)
{
  GTask *task;
  WsImage *image;
  ImageLoadingData *data;
  SoupMessage *message;

  g_assert (self);
  g_assert (gallery);

  image = ws_gallery_get_image (gallery, image_index);
  task = g_task_new (self, cancellable, callback, user_data);

  if (image->loaded)
    {
      g_task_return_pointer (task, image, NULL);
      g_object_unref (task);
      return;
    }

  /* 'Animated' images, aka videos, are a special case and will be streamed
   * when showing them */
  if (image->is_animated)
    {
      GFile *file = g_file_new_for_uri (image->link);

      g_clear_object (&image->paintable);
      image->paintable = GDK_PAINTABLE (gtk_media_file_new_for_file (file));
      image->loaded = TRUE;
      /*imgur_album_notify_image_loaded (image->album, image->index);*/

      g_task_return_pointer (task, image, NULL);
      g_object_unref (task);
      g_object_unref (file);

      return;
    }

  data = g_malloc0 (sizeof (ImageLoadingData));
  data->task = task;
  data->session = soup_session_new ();
  data->image = image;

  message = soup_message_new ("GET", image->link);

  soup_session_queue_message (data->session,
                              message,
                              soup_message_cb,
                              data);
}

WsImage *
ws_gallery_manager_load_image_finish (WsGalleryManager  *self,
                                      GAsyncResult       *result,
                                      GError            **error)
{
  g_assert (g_task_is_valid (result, self));

  return g_task_propagate_pointer (G_TASK (result), error);
}


