#include "waster-image-loader.h"
#include "waster.h"

#include <glib.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>

static void
imgur_image_init_from_json (ImgurImage *self,
                            JsonObject *json_obj)
{
  g_free (self->id);
  self->id = g_strdup (json_object_get_string_member (json_obj, "id"));

  if (json_object_has_member (json_obj, "width"))
    {
      self->width  = json_object_get_int_member (json_obj, "width");
      self->height = json_object_get_int_member (json_obj, "height");
    }

  g_free (self->title);
  if (json_object_get_null_member (json_obj, "title"))
    self->title = NULL;
  else
    self->title = g_strdup (json_object_get_string_member (json_obj, "title"));

  if (json_object_has_member (json_obj, "animated"))
    self->is_animated = json_object_get_boolean_member (json_obj, "animated");
  else
    self->is_animated = FALSE;

  g_free (self->link);
  if (self->is_animated)
    self->link = g_strdup (json_object_get_string_member (json_obj, "mp4"));
  else
    self->link = g_strdup (json_object_get_string_member (json_obj, "link"));

  g_clear_object (&self->paintable);
}

static void
imgur_album_init_from_json (ImgurAlbum *self,
                            JsonObject *album_obj)
{
  int n_images;

  self->title = g_strdup (json_object_get_string_member (album_obj, "title"));

  g_snprintf (self->id, sizeof (self->id), "%s",
              json_object_get_string_member (album_obj, "id"));

  if (json_object_has_member (album_obj, "images_count"))
    n_images = (int) json_object_get_int_member (album_obj, "images_count");
  else if (json_object_has_member (album_obj, "images"))
    n_images = (int) json_array_get_length (json_object_get_array_member (album_obj, "images"));
  else
    n_images = 1;

  self->n_images = n_images;

  /* Some of these json objects have an "images" array, even if the array only contains
   * one element. Some of them don't have it in that case though. Let's map both of those
   * cases to the former one. */
  if (json_object_has_member (album_obj, "images"))
    {
      JsonArray *images_array;
      int i, p;

      images_array = json_object_get_array_member (album_obj, "images");
      self->images = g_malloc0 (self->n_images * sizeof (ImgurImage));

      p = (int) json_array_get_length (images_array);
      for (i = 0; i < p; i ++)
        {
          imgur_image_init_from_json (&self->images[i],
                                      json_array_get_object_element (images_array, i));
          self->images[i].index = i;
        }

      /* If the images array (or the album object itself) already contains all the images
       * of the album, let's mark it as loaded. */
      self->loaded = (self->n_images == p);
    }
  else
    {
      self->images = g_malloc0 (self->n_images * sizeof (ImgurImage));

      imgur_image_init_from_json (&self->images[0], album_obj);
      self->images[0].index = 0;
      self->loaded = TRUE; /* Naturally */
    }
}


G_DEFINE_TYPE (WsImageLoader, ws_image_loader, G_TYPE_OBJECT);

typedef struct _WsImageLoader WsImageLoader;

typedef struct
{
  WsImageLoader *loader;
  void *payload;
  GTask *task;
} LoaderData;

static void
gallery_call_finished_cb (GObject      *source_object,
                          GAsyncResult *result,
                          gpointer      user_data)
{
  RestProxyCall *call = REST_PROXY_CALL (source_object);
  LoaderData *data = user_data;
  GTask *task = data->task;
  WsImageLoader *loader = data->loader;
  GError *error = NULL;
  JsonParser *parser;
  JsonObject *root;
  int i, n_albums;
  JsonArray *data_array;
  ImgurGallery *gallery;

  rest_proxy_call_invoke_finish (call, result, &error);

  parser = json_parser_new ();
  json_parser_load_from_data (parser, rest_proxy_call_get_payload (call), -1, &error);

  if (error)
    {
      g_task_return_error (task, error);
      return;
    }

  root = json_node_get_object (json_parser_get_root (parser));

  if (!json_object_has_member (root, "data") ||
      !JSON_NODE_HOLDS_ARRAY (json_object_get_member (root, "data")))
    {
      g_critical ("%s: root is not an array.", __FUNCTION__);
      printf ("%s\n", rest_proxy_call_get_payload (call));
      return;
    }

  data_array = json_object_get_array_member (root, "data");
  n_albums = json_array_get_length (data_array);

  g_assert (loader);

  gallery = g_malloc0 (sizeof (ImgurGallery));
  gallery->n_albums = n_albums;
  gallery->albums = g_malloc0 (n_albums * sizeof (ImgurAlbum));

  for (i = 0; i < n_albums; i ++)
    {
      JsonObject *json_object = json_array_get_object_element (data_array, i);
      ImgurAlbum *album = &gallery->albums[i];

      imgur_album_init_from_json (album, json_object);
    }

  g_object_unref (parser);
  g_object_unref (call);

  g_task_return_pointer (task, gallery, NULL);
}

void
ws_image_loader_load_gallery_async (WsImageLoader                *loader,
                                    const ImgurGalleryDefinition *gallery,
                                    GCancellable                 *cancellable,
                                    GAsyncReadyCallback           callback,
                                    gpointer                      user_data)
{
  char buff[4096];
  Waster *app = (Waster *)g_application_get_default ();
  GTask *task;
  LoaderData *data;
  RestProxyCall *call;

  g_snprintf (buff, sizeof (buff), gallery->function, 0);

  call = rest_proxy_new_call (app->proxy);
  rest_proxy_call_set_function (call, buff);
  rest_proxy_call_set_method (call, "GET");

  task = g_task_new (loader, cancellable, callback, user_data);

  data = g_new (LoaderData, 1);
  data->loader = loader;
  data->task = task;

  rest_proxy_call_invoke_async (call,
                                cancellable,
                                gallery_call_finished_cb,
                                data);
}

ImgurGallery *
ws_image_loader_load_gallery_finish (WsImageLoader  *loader,
                                     GAsyncResult   *result,
                                     GError        **error)
{
  g_assert (g_task_is_valid (result, loader));

  return g_task_propagate_pointer (G_TASK (result), error);
}


static void
album_call_finished_cb (GObject      *source_object,
                        GAsyncResult *result,
                        gpointer      user_data)
{
  RestProxyCall *call = REST_PROXY_CALL (source_object);
  LoaderData *data = user_data;
  GTask *task = data->task;
  ImgurAlbum *album = data->payload;
  GError *error = NULL;
  JsonParser *parser;
  JsonObject *root;
  int i;
  JsonArray *data_array;

  rest_proxy_call_invoke_finish (call, result, &error);

  if (error)
    {
      g_task_return_error (task, error);
      return;
    }

  parser = json_parser_new ();
  json_parser_load_from_data (parser, rest_proxy_call_get_payload (call), -1, &error);

  if (error)
    {
      g_task_return_error (task, error);
      return;
    }

  root = json_node_get_object (json_parser_get_root (parser));
  data_array = json_object_get_array_member (root, "data");

  g_assert_cmpint (json_array_get_length (data_array), ==, album->n_images);

  /* This will override the few images at the beginning we've already initialized but whatever */
  for (i = 0; i < album->n_images; i ++)
    {
      ImgurImage *image = &album->images[i];

      imgur_image_init_from_json (image, json_array_get_object_element (data_array, i));
      image->index = i;
    }

  g_object_unref (parser);
  g_object_unref (call);

  album->loaded = TRUE;

  g_task_return_pointer (task, album, NULL);
}

void
ws_image_loader_load_album_async (WsImageLoader       *loader,
                                  ImgurAlbum          *album,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
  char buff[4096];
  Waster *app = (Waster *)g_application_get_default ();
  GTask *task;
  LoaderData *data;
  RestProxyCall *call;

  g_assert (album != NULL);

  task = g_task_new (loader, cancellable, callback, user_data);

  if (album->loaded)
    {
      g_task_return_pointer (task, album, NULL);
      g_object_unref (task);
      return;
    }

  g_snprintf (buff, sizeof (buff), "album/%s/images", album->id);

  call = rest_proxy_new_call (app->proxy);
  rest_proxy_call_set_function (call, buff);
  rest_proxy_call_set_method (call, "GET");

  data = g_new (LoaderData, 1);
  data->loader = loader;
  data->task = task;
  data->payload = album;

  rest_proxy_call_invoke_async (call,
                                cancellable,
                                album_call_finished_cb,
                                data);
}

ImgurAlbum *
ws_image_loader_load_album_finish (WsImageLoader  *loader,
                                   GAsyncResult   *result,
                                   GError        **error)
{
  return g_task_propagate_pointer (G_TASK (result), error);
}

void
ws_image_loader_load_image_async (WsImageLoader       *loader,
                                  ImgurImage          *image,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
  GTask *task;

  task = g_task_new (loader, cancellable, callback, user_data);

  g_message ("%s: %s", __FUNCTION__, image->link);

  /* 'Animated' images, aka videos, are a special case and will be streamed
   * when showing them */
  if (image->is_animated)
    {
      image->paintable = GDK_PAINTABLE (gtk_media_file_new_for_filename ("/home/baedert/Peek 2017-09-26 07-23.webm"));
      gtk_media_stream_set_loop (GTK_MEDIA_STREAM (image->paintable), TRUE);
      gtk_media_stream_play (GTK_MEDIA_STREAM (image->paintable));

      g_task_return_pointer (task, image, NULL);
      g_object_unref (task);
      return;
    }

  g_message ("%s: ID: %s, Title: %s, Link: %s",
             __FUNCTION__, image->id, image->title, image->link);

  {
    GInputStream *in_stream;
    GdkPixbufAnimation *animation;
    GdkPixbuf *pixbuf;
    SoupMessage *message = soup_message_new ("GET", image->link);
    SoupSession *session;
    GError *error = NULL;
    GBytes *response;

    session = soup_session_new ();

    g_message ("Loading %s", image->link);

    soup_session_send_message (session, message);

    g_object_get (message, "response-body-data", &response, NULL);

    in_stream = g_memory_input_stream_new_from_bytes (response);

    animation = gdk_pixbuf_animation_new_from_stream (in_stream, NULL, &error);

    if (cancellable != NULL && g_cancellable_is_cancelled (cancellable))
      {
        g_task_return_pointer (task, NULL, NULL);
        return;
      }

    if (error)
      {
        g_message ("%s: %s", __FUNCTION__, error->message);

        g_task_return_error (task, error);

        g_input_stream_close (in_stream, NULL, NULL);
        g_object_unref (in_stream);
        g_bytes_unref (response);
        return;
      }

    pixbuf = gdk_pixbuf_animation_get_static_image (animation);
    image->paintable = GDK_PAINTABLE (gdk_texture_new_for_pixbuf (pixbuf));

    if (cancellable != NULL && g_cancellable_is_cancelled (cancellable))
      {
        g_task_return_pointer (task, NULL, NULL);
        return;
      }

    g_input_stream_close (in_stream, NULL, NULL);
    g_object_unref (in_stream);

    g_object_unref (animation);
    g_object_unref (message);
    g_object_unref (session);
    g_bytes_unref (response);

    g_task_return_pointer (task, image, NULL);
  }
}

ImgurImage *
ws_image_loader_load_image_finish (WsImageLoader  *loader,
                                   GAsyncResult   *result,
                                   GError        **error)
{
  g_return_val_if_fail (g_task_is_valid (result, loader), NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}

/* ---------------------------------------------------------------------------------- */

WsImageLoader *
ws_image_loader_new ()
{
  return g_object_new (WS_TYPE_IMAGE_LOADER, NULL);
}

void
ws_image_loader_init (WsImageLoader *loader)
{
}

void
ws_image_loader_class_init (WsImageLoaderClass *class)
{
}