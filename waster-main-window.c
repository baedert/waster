#include "waster-main-window.h"
#include "waster-image-loader.h"
#include "waster-initial-state.h"
#include "waster-album-view.h"
#include "waster.h"


struct _WsMainWindow
{
  GtkApplicationWindow parent_instance;
  GtkWidget *main_stack;
  GtkWidget *next_button;
  GtkWidget *prev_button;
  GtkWidget *initial_state;
  GtkWidget *spinner;
  GtkWidget *image_stack;
  GtkWidget *album_view;

  WsImageLoader *loader;

  guint current_image_index;
};

typedef struct _WsMainWindow WsMainWindow;

G_DEFINE_TYPE (WsMainWindow, ws_main_window, GTK_TYPE_APPLICATION_WINDOW);

/* Prototypes {{{ */
static void image_loaded_cb (GObject      *source_object,
                             GAsyncResult *result,
                             gpointer      user_data);
/* }}} */

void
next_button_clicked_cb (GtkButton *button,
                        gpointer   user_data)
{
  WsMainWindow *window = user_data;
  WsImageLoader *loader = window->loader;
  g_assert (WS_IS_MAIN_WINDOW (user_data));

  window->current_image_index ++;
  g_message ("Loading image %u", window->current_image_index);
  ws_image_loader_load_image_async (loader,
                                    loader->images[window->current_image_index],
                                    NULL,
                                    image_loaded_cb,
                                    window);

}

void
prev_button_clicked_cb ()
{

}

WsMainWindow *
ws_main_window_new (GtkApplication *app)
{
  return g_object_new (WS_TYPE_MAIN_WINDOW,
                       "show-menubar", FALSE,
                       "application", app,
                       NULL);
}

static void
subimage_loaded_cb (GObject *source_object,
                    GAsyncResult *result,
                    gpointer      user_data)
{
  GError *error = NULL;
  WsMainWindow *window = user_data;
  WsImageLoader *loader = WS_IMAGE_LOADER (source_object);
  ImgurImage *loaded_image;


  g_message ("Subimage loaded!");

  loaded_image = ws_image_loader_load_image_finish (loader, result, &error);

  if (error)
    {
      g_warning ("%s", error->message);
      return;
    }

  ws_album_view_show_image (WS_ALBUM_VIEW (window->album_view),
                            loaded_image);
}

static void
image_loaded_cb (GObject      *source_object,
                 GAsyncResult *result,
                 gpointer      user_data)
{
  GError *error = NULL;
  ImgurImage *img;
  WsImageLoader *loader = WS_IMAGE_LOADER (source_object);
  WsMainWindow  *window = user_data;

  img = ws_image_loader_load_image_finish (loader, result, &error);

  if (error != NULL)
    {
      g_warning (error->message);
      return;
    }

  ws_album_view_reserve_space (WS_ALBUM_VIEW (window->album_view),
                               img);

  if (img->is_album)
    {
      int i;
      g_message ("Loading %d images...", img->n_subimages);

      for (i = 0; i < img->n_subimages; i ++)
        {
          ImgurImage *subimg = img->subimages[i];
          ws_image_loader_load_image_async (loader,
                                            subimg,
                                            NULL,
                                            subimage_loaded_cb,
                                            window);

        }

      gtk_window_set_title (GTK_WINDOW (window), "[Album]");
    }
  else
    {
      gtk_window_set_title (GTK_WINDOW (window), img->title);
      ws_album_view_show_image (WS_ALBUM_VIEW (window->album_view),
                                img);
    }

}

static void
gallery_loaded_cb (GObject      *source_object,
                   GAsyncResult *result,
                   gpointer      user_data)
{
  GError *error = NULL;
  WsImageLoader *loader = WS_IMAGE_LOADER (source_object);
  WsMainWindow  *window = user_data;

  ws_image_loader_load_gallery_finish (loader, result, &error);

  if (error != NULL)
    {
      g_assert (FALSE);
    }

  window->current_image_index = 0;
  gtk_stack_set_visible_child_name (GTK_STACK (window->main_stack), "image");

  /* Gallery is here, we can start loading images */
  ws_image_loader_load_image_async (loader,
                                    loader->images[window->current_image_index],
                                    NULL,
                                    image_loaded_cb,
                                    window);
}

void
ws_main_window_init (WsMainWindow *win)
{
  Waster *app = (Waster *)g_application_get_default ();

  gtk_widget_init_template (GTK_WIDGET (win));
  win->loader = ws_image_loader_new ();

  if (waster_is_proxy_inited (app))
    {
      gtk_stack_set_visible_child_name (GTK_STACK (win->main_stack), "spinner");
      gtk_spinner_start (GTK_SPINNER (win->spinner));
      ws_image_loader_load_gallery_async (win->loader,
                                          NULL,
                                          gallery_loaded_cb,
                                          win);
    }
  else
    {
      gtk_stack_set_visible_child_name (GTK_STACK (win->main_stack), "initial_state");
    }

}

void
ws_main_window_class_init (WsMainWindowClass *class)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/baedert/waster/ui/main-window.ui");

  gtk_widget_class_bind_template_child (widget_class, WsMainWindow, main_stack);
  gtk_widget_class_bind_template_child (widget_class, WsMainWindow, initial_state);
  gtk_widget_class_bind_template_child (widget_class, WsMainWindow, prev_button);
  gtk_widget_class_bind_template_child (widget_class, WsMainWindow, next_button);
  gtk_widget_class_bind_template_child (widget_class, WsMainWindow, image_stack);
  gtk_widget_class_bind_template_child (widget_class, WsMainWindow, spinner);
  gtk_widget_class_bind_template_child (widget_class, WsMainWindow, album_view);

  gtk_widget_class_bind_template_callback (widget_class, next_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, prev_button_clicked_cb);
}
