#include "waster-image-loader.h"
#include "waster.h"

#include <glib.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>



static void
print_call (RestProxyCall *call)
{
  RestParams *params;
  GHashTable *ht;
  GList *keys, *l;
  GString *str = g_string_new (rest_proxy_call_get_method (call));
  g_string_append (str, " ");
  g_string_append (str, rest_proxy_call_get_function (call));

  params = rest_proxy_call_get_params (call);
  ht = rest_params_as_string_hash_table (params);

  g_string_append_c (str, '?');

  keys = g_hash_table_get_keys (ht);
  for (l = keys; l; l = l->next)
    {
      char *k = l->data;
      char *v = g_hash_table_lookup (ht, k);
      g_string_append (str, k);
      g_string_append (str, "=");
      g_string_append (str, v);
      g_string_append (str, "&");
    }

  g_message ("%s", str->str);
}


static void
print_imgur_image (ImgurImage *img)
{
  g_message ("Image -- ID: %s , link: %s , album: %d , title: '%s'",
             img->id, img->link, img->is_album, img->title);
}

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
  img->surface = NULL;
}

G_DEFINE_TYPE (WsImageLoader, ws_image_loader, G_TYPE_OBJECT);

typedef struct _WsImageLoader WsImageLoader;


  /*
   * TODO:
   *
   * 1) Actual GIFs. Do they even exist on imgur or are they just encoding all the gifs as videos?
   * 2) Videos. gifv/mp4/webm available!
   *
   */

static void
ws_image_loader_load_gallery_threaded (GTask         *task,
                                       gpointer       source_object,
                                       gpointer       task_data,
                                       GCancellable *cancellable)
{
  GError *error = NULL;
  WsImageLoader *loader = source_object;
  Waster *app = (Waster *)g_application_get_default ();
  int i;

  RestProxyCall *call = rest_proxy_new_call (app->proxy);
  /*rest_proxy_call_set_function (call, "/gallery/hot/viral/0.json");*/
  rest_proxy_call_set_function (call, "gallery/hot/viral/0.json");
  rest_proxy_call_set_method (call, "GET");
  /*char *auth = g_strdup_printf ("Bearer %s", oauth2_proxy_get_access_token (app->proxy));*/
  /*rest_proxy_call_add_header (call, "Authorization", auth);*/

  print_call (call);

  rest_proxy_call_sync (call, NULL);

  JsonParser *parser = json_parser_new ();
  json_parser_load_from_data (parser, rest_proxy_call_get_payload (call), -1, &error);

  JsonObject *root = json_node_get_object (json_parser_get_root (parser));

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

  JsonArray  *data_array = json_object_get_array_member (root, "data");
  int n_images = json_array_get_length (data_array);

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


  loader->n_images = n_images;
}

void
ws_image_loader_load_gallery_async (WsImageLoader       *loader,
                                    GCancellable        *cancellable,
                                    GAsyncReadyCallback  callback,
                                    gpointer             user_data)
{
  /*GTask *task = g_task_new (loader, cancellable, callback, user_data);*/

  ws_image_loader_load_gallery_threaded (NULL, loader, NULL, NULL);
  /*g_task_run_in_thread (task, ws_image_loader_load_gallery_threaded);*/
  /*g_object_unref (task);*/
}

void
ws_image_loader_load_gallery_finish (WsImageLoader  *loader,
                                     GAsyncResult   *result,
                                     GError        **error)
{
  g_return_if_fail (g_task_is_valid (result, loader));
}



void
ws_image_loader_load_image_threaded (GTask         *task,
                                     gpointer       source_object,
                                     gpointer       task_data,
                                     GCancellable *cancellable)

{
  SoupSession *session;
  SoupMessage *msg;
  GError *error = NULL;
  WsImageLoader *loader = source_object;
  ImgurImage *image = task_data;

  session = soup_session_new ();

  if (image->is_album)
    {
      Waster *waster = (Waster *) g_application_get_default ();
      char *function = g_strdup_printf ("album/%s/images", image->id);

      RestProxyCall *call = rest_proxy_new_call (waster->proxy);
      rest_proxy_call_set_function (call, function);
      rest_proxy_call_set_method (call, "GET");

      rest_proxy_call_sync (call, NULL);

      {
        JsonParser *parser = json_parser_new ();
        JsonObject *root;
        JsonArray  *data_array;
        int         n_subimages, i;
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
      }


      g_free (function);

      g_task_return_pointer (task, image, NULL);
      return;
    }


  SoupMessage *message = soup_message_new ("GET", image->link);

  soup_session_send_message (session, message);

  GBytes *response;
  g_object_get (message, "response-body-data", &response, NULL);
  g_assert (response);

  GInputStream *in_stream = g_memory_input_stream_new_from_bytes (response);

  GdkPixbufAnimation *animation = gdk_pixbuf_animation_new_from_stream (in_stream,
                                                                        NULL, &error);

  if (error)
    {
      g_task_return_error (task, error);

      g_input_stream_close (in_stream, NULL, NULL);
      g_object_unref (in_stream);
      return;
    }

  GdkPixbuf *pixbuf = gdk_pixbuf_animation_get_static_image (animation);

  cairo_surface_t *surface = gdk_cairo_surface_create_from_pixbuf (pixbuf,
                                                                   1,
                                                                   NULL);

  image->surface = surface;

  g_input_stream_close (in_stream, NULL, NULL);
  g_object_unref (in_stream);

  g_object_unref (animation);
  g_object_unref (message);
  g_object_unref (session);

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

  g_return_if_fail (WS_IS_IMAGE_LOADER (loader));

  task = g_task_new (loader, cancellable, callback, user_data);
  if (image->surface != NULL)
    {
      g_task_return_pointer (task, image, NULL);
      g_object_unref (task);
      return;
    }

  g_task_set_task_data (task, image, NULL);
  g_task_run_in_thread (task, ws_image_loader_load_image_threaded);

  g_object_unref (task);
}

ImgurImage *
ws_image_loader_load_image_finish (WsImageLoader *loader,
                                   GAsyncResult  *result,
                                   GError        **error)
{
  g_return_val_if_fail (g_task_is_valid (result, loader), NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}



void
ws_image_loader_load_album_threaded (GTask         *task,
                                     gpointer       source_object,
                                     gpointer       task_data,
                                     GCancellable *cancellable)
{
  WsImageLoader *loader = source_object;
  ImgurImage *album = task_data;
  ImgurImage *image;

  g_assert (album->is_album);




}

void
ws_image_loader_load_album_async (WsImageLoader       *loader,
                                  ImgurImage          *album,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
  GTask *task;

  g_return_if_fail (album != NULL);
  g_return_if_fail (album->is_album);

  task = g_task_new (loader, cancellable, callback, user_data);

  g_task_set_task_data (task, album, NULL);
  g_task_run_in_thread (task, ws_image_loader_load_album_threaded);

  g_object_unref (task);
}

ImgurImage *
ws_image_loader_load_album_finish (WsImageLoader  *loader,
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
  loader->current = 0;
}

void
ws_image_loader_class_init (WsImageLoaderClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
}
