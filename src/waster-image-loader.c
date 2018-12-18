#include "waster-image-loader.h"
#include "waster.h"

#include <glib.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>

#if 0
static void
print_imgur_image (ImgurImage *img)
{
  g_message ("Image -- ID: %s , link: %s , album: %d , title: '%s'",
             img->id, img->link, img->is_album, img->title);
}
#endif

static void
imgur_image_init_from_json (ImgurImage *img,
                            JsonObject *json_obj)
{
  img->id     = g_strdup (json_object_get_string_member (json_obj, "id"));

  if (json_object_has_member (json_obj, "width"))
    {
      img->width  = json_object_get_int_member (json_obj, "width");
      img->height = json_object_get_int_member (json_obj, "height");
    }

  if (json_object_get_null_member (json_obj, "title"))
    img->title = NULL;
  else
    img->title = g_strdup (json_object_get_string_member (json_obj, "title"));

  if (json_object_has_member (json_obj, "animated"))
    img->is_animated = json_object_get_boolean_member (json_obj, "animated");
  else
    img->is_animated = FALSE;

  if (img->is_animated)
    img->link = g_strdup (json_object_get_string_member (json_obj, "mp4"));
  else
    img->link = g_strdup (json_object_get_string_member (json_obj, "link"));


  img->index = -1;
  img->paintable = NULL;
}

G_DEFINE_TYPE (WsImageLoader, ws_image_loader, G_TYPE_OBJECT);

typedef struct _WsImageLoader WsImageLoader;

typedef struct
{
  WsImageLoader *loader;
  ImgurImage *image;
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
  int i, n_images;
  g_message (__FUNCTION__);

  rest_proxy_call_invoke_finish (call, result, &error);
  g_assert_no_error (error);

  parser = json_parser_new ();
  json_parser_load_from_data (parser, rest_proxy_call_get_payload (call), -1, &error);

  root = json_node_get_object (json_parser_get_root (parser));

  if (error)
    {
      g_error ("%s", error->message);
    }

  if (!json_object_has_member (root, "data") ||
      !JSON_NODE_HOLDS_ARRAY (json_object_get_member (root, "data")))
    {
      g_critical ("%s: root is not an array.", __FUNCTION__);
      printf ("%s\n", rest_proxy_call_get_payload (call));
      return;
    }

  JsonArray *data_array = json_object_get_array_member (root, "data");
  n_images = json_array_get_length (data_array);

  g_assert (loader);
  loader->images = (ImgurImage **) g_malloc (n_images * sizeof (ImgurImage *));

  for (i = 0; i < n_images; i ++)
    {
      JsonObject *image_object = json_array_get_object_element (data_array, i);
      loader->images[i] = g_malloc (sizeof (ImgurImage));
      ImgurImage *img = loader->images[i];

      imgur_image_init_from_json (img, image_object);
      img->is_album = json_object_get_boolean_member (image_object, "is_album");
    }

  g_object_unref (parser);
  g_object_unref (call);

  g_task_return_boolean (task, TRUE);

  loader->n_images = n_images;
}

void
ws_image_loader_load_gallery_async (WsImageLoader       *loader,
                                    GCancellable        *cancellable,
                                    GAsyncReadyCallback  callback,
                                    gpointer             user_data)
{
  GTask *task;
  Waster *app = (Waster *)g_application_get_default ();
  LoaderData *data;
  RestProxyCall *call = rest_proxy_new_call (app->proxy);

  rest_proxy_call_set_function (call, "gallery/hot/viral/0.json");
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

void
ws_image_loader_load_gallery_finish (WsImageLoader  *loader,
                                     GAsyncResult   *result,
                                     GError        **error)
{
  g_return_if_fail (g_task_is_valid (result, loader));
}

static void
image_call_finished_cb (GObject      *source_object,
                        GAsyncResult *result,
                        gpointer      user_data)
{
  RestProxyCall *call = REST_PROXY_CALL (source_object);
  GError *error = NULL;
  LoaderData *data = user_data;
  GTask *task = data->task;
  ImgurImage *image = data->image;
  JsonParser *parser = json_parser_new ();
  JsonObject *root;
  JsonArray *data_array;
  int i;

  rest_proxy_call_invoke_finish (call, result, &error);
  if (error)
    {
      g_task_return_error (task, error);
      return;
    }

  json_parser_load_from_data (parser, rest_proxy_call_get_payload (call), -1, NULL);
  root = json_node_get_object (json_parser_get_root (parser));
  data_array = json_object_get_array_member (root, "data");

  image->n_subimages = (int)json_array_get_length (data_array);
  image->subimages = g_malloc (image->n_subimages * sizeof (ImgurImage *));
  for (i = 0; i < image->n_subimages; i ++)
    {
      JsonObject *img_json = json_array_get_object_element (data_array, i);
      image->subimages[i] = g_malloc (sizeof (ImgurImage));
      imgur_image_init_from_json (image->subimages[i], img_json);
      image->subimages[i]->index = i;
      image->subimages[i]->is_album = FALSE;
    }

  g_object_unref (parser);

  g_task_return_pointer (task, image, NULL);
}

void
ws_image_loader_load_image_async (WsImageLoader       *loader,
                                  ImgurImage          *image,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
  GTask *task;
  Waster *app = (Waster *)g_application_get_default ();

  task = g_task_new (loader, cancellable, callback, user_data);

  /* 'Animated' images, aka videos, are a special case and will be streamed
   * when showing them */
  if (image->is_animated)
    {
      image->paintable = GDK_PAINTABLE (gtk_media_file_new_for_resource ("/home/baedert/Peek 2017-09-26 07-23.webm"));
      gtk_media_stream_set_loop (GTK_MEDIA_STREAM (image->paintable), TRUE);
      gtk_media_stream_play (GTK_MEDIA_STREAM (image->paintable));

      g_task_return_pointer (task, image, NULL);
      g_object_unref (task);
      return;
    }

  if (image->is_album)
    {
      LoaderData *data;
      char *function;
      RestProxyCall *call = rest_proxy_new_call (app->proxy);

      function = g_strdup_printf ("album/%s/images", image->id);

      rest_proxy_call_take_function (call, g_strdup_printf ("album/%s/images", image->id));
      rest_proxy_call_set_method (call, "GET");

      data = g_new (LoaderData, 1);
      data->loader = loader;
      data->image = image;
      data->task = task;

      rest_proxy_call_invoke_async (call,
                                    cancellable,
                                    image_call_finished_cb,
                                    data);
    }
  else
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
ws_image_loader_load_image_finish (WsImageLoader *loader,
                                   GAsyncResult  *result,
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
  loader->current = 0;
}

void
ws_image_loader_class_init (WsImageLoaderClass *class)
{
}
