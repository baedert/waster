#include "waster-image-loader.h"
#include "waster.h"

#include <glib.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>



static void
print_imgur_image (ImgurImage *img)
{
  g_message ("Image -- ID: %s , link: %s , album: %d , title: '%s'",
             img->id, img->link, img->is_album, img->title);
}


struct _WsImageLoader
{
  GObject parent_instance;
  ImgurImage **images;
  guint n_images;
  guint current;
};

G_DEFINE_TYPE (WsImageLoader, ws_image_loader, G_TYPE_OBJECT);

typedef struct _WsImageLoader WsImageLoader;

/*enum {*/
  /*GALLERY_LOADED,*/
  /*IMAGE_LOADED,*/

  /*N_SIGNALS*/
/*};*/

/*static guint loader_signals[N_SIGNALS] = { 0, };*/


void
ws_image_loader_next (WsImageLoader *loader)
{
  g_assert (0);

  loader->current ++;
}


void ws_image_loader_preload (WsImageLoader *loader)
{
  /*
   * 1) Use libsoup to load the entire file.
   * 2) Use a MemoryInputStream to pipe it into GdkPixbuf
   * 3) Render that pixbuf to a cairo_surface_t
   * 4) Render that surface.
   *
   * TODO:
   *
   * 1) Actual GIFs. Do they even exist on imgur or are they just encoding all the gifs as videos?
   * 2) Videos. gifv/mp4/webm available!
   *
   */
}

static void
ws_image_loader_load_gallery_threaded (GTask         *task,
                                       gpointer       source_object,
                                       gpointer       task_data,
                                       GCancellable *cancellable)
{
  WsImageLoader *loader = source_object;
  Waster *app = (Waster *)g_application_get_default ();
  int i;

  RestProxyCall *call = rest_proxy_new_call (app->proxy);
  rest_proxy_call_set_function (call, "/gallery/hot/viral/0.json");
  rest_proxy_call_set_method (call, "GET");

  rest_proxy_call_sync (call, NULL);

  JsonParser *parser = json_parser_new ();
  json_parser_load_from_data (parser, rest_proxy_call_get_payload (call), -1, NULL);

  JsonObject *root = json_node_get_object (json_parser_get_root (parser));
  JsonArray  *data_array = json_object_get_array_member (root, "data");
  int n_images = json_array_get_length (data_array);

  g_assert (loader);
  loader->images = (ImgurImage **) g_malloc (n_images * sizeof (ImgurImage *));

  for (i = 0; i < n_images; i ++)
    {
      JsonObject *image_object = json_array_get_object_element (data_array, i);
      loader->images[i] = g_malloc (sizeof (ImgurImage));
      ImgurImage *img = loader->images[i];

      img->id       = g_strdup (json_object_get_string_member (image_object, "id"));
      img->title    = g_strdup (json_object_get_string_member (image_object, "title"));
      img->link     = g_strdup (json_object_get_string_member (image_object, "link"));
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
  GTask *task = g_task_new (loader, cancellable, callback, user_data);

  g_task_run_in_thread (task, ws_image_loader_load_gallery_threaded);
  g_object_unref (task);
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
  GError *error = NULL;
  WsImageLoader *loader = source_object;
  guint image_index = GPOINTER_TO_UINT (task_data);

  ImgurImage *current_image = loader->images[image_index];



  SoupSession *session = soup_session_new ();
  SoupMessage *message = soup_message_new ("GET", current_image->link);

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

  current_image->surface = surface;

  g_input_stream_close (in_stream, NULL, NULL);
  g_object_unref (in_stream);

  g_object_unref (animation);
  g_object_unref (message);
  g_object_unref (session);

  g_task_return_pointer (task, current_image, NULL);
}

void
ws_image_loader_load_image_async (WsImageLoader       *loader,
                                  guint                image_index,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
  GTask *task = g_task_new (loader, cancellable, callback, user_data);

  if (image_index >= loader->n_images)
    {
      g_task_return_new_error (task, WS_ERROR, WS_GENERIC_ERROR,
                               "%u >= %u", image_index, loader->n_images);
      g_object_unref (task);
      return;
    }

  g_task_set_task_data (task, GUINT_TO_POINTER (image_index), NULL);
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


  /*loader_signals[IMAGE_LOADED] =*/
      /*g_signal_newv ("gallery-loaded",*/
                    /*G_TYPE_FROM_CLASS (class),*/
                    /*G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,*/
                    /*NULL,*/
                    /*NULL,*/
                    /*NULL,*/
                    /*NULL,*/
                    /*G_TYPE_NONE, [> Return type <]*/
                    /*0,*/
                    /*NULL);*/


   /*TODO: Make this a signal where the callback gets 2 parameters -- the image id and the subimage id (for albums */
  /*loader_signals[IMAGE_LOADED] =*/
      /*g_signal_new ("image-loaded",*/
                    /*G_TYPE_FROM_CLASS (object_class),*/
                    /*G_SIGNAL_RUN_LAST,*/
                    /*0, [> class callback offset <]*/
                    /*NULL,*/
                    /*NULL,*/
                    /*NULL,*/
                    /*G_TYPE_NONE, [> return type <]*/
                    /*1, CAIRO_GOBJECT_TYPE_SURFACE);*/





                    /*G_STRUCT_OFFSET (WsImageLoaderClass, load_iamge*/
      /*g_signal_newv ("image-loaded",*/
                    /*G_TYPE_FROM_CLASS (class),*/
                    /*G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,*/
                    /*NULL,*/
                    /*NULL,*/
                    /*NULL,*/
                    /*NULL,*/
                    /*G_TYPE_NONE, [> Return type <]*/
                    /*0,*/
                    /*NULL);*/
}
