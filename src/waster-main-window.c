#include "imgur-gallery.h"

#include "waster-main-window.h"
#include "waster-image-loader.h"
#include "waster-initial-state.h"
#include "waster-album-view.h"
#include "waster-impostor.h"
#include "waster.h"

#define LOOKAHEAD 1

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
  GtkWidget *impostor;

  ImgurGallery *gallery;

  WsImageLoader *loader;

  GCancellable* cancellables[LOOKAHEAD + 1];

  guint current_album_index;
  int current_image_index;
};

typedef struct _WsMainWindow WsMainWindow;

G_DEFINE_TYPE (WsMainWindow, ws_main_window, GTK_TYPE_APPLICATION_WINDOW);

static void
album_loaded_cb (GObject      *source_object,
                 GAsyncResult *result,
                 gpointer      user_data)
{
  GError *error = NULL;
  ImgurAlbum *album;
  WsImageLoader *loader = WS_IMAGE_LOADER (source_object);

  album = ws_image_loader_load_album_finish (loader, result, &error);

  if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      g_error_free (error);
      g_message ("album lading cancelled");
      return;
    }
  else if (error != NULL)
    {
      g_warning ("%s: %s", __FUNCTION__, error->message);
      g_error_free (error);
      return;
    }
}

static void
image_loaded_cb (GObject      *source_object,
                 GAsyncResult *result,
                 gpointer      user_data)
{
  GError *error = NULL;
  ImgurImage *image;
  WsImageLoader *loader = WS_IMAGE_LOADER (source_object);
  WsMainWindow  *window = user_data;

  image = ws_image_loader_load_image_finish (loader, result, &error);


  if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      g_error_free (error);
      g_message ("image lading cancelled");
      return;
    }
  else if (error != NULL)
    {
      g_warning ("%s: %s", __FUNCTION__, error->message);
      g_error_free (error);
      return;
    }

  ws_album_view_show_image (WS_ALBUM_VIEW (window->album_view), image);
}

static void
show_next_album (WsMainWindow *window)
{
  WsImageLoader *loader = window->loader;
  ImgurAlbum *album;
  char buff[4096];

  gtk_stack_set_transition_type (GTK_STACK (window->image_stack),
                                 GTK_STACK_TRANSITION_TYPE_NONE);
  ws_impostor_clone (WS_IMPOSTOR (window->impostor),
                     gtk_stack_get_visible_child (GTK_STACK (window->image_stack)));
  gtk_stack_set_visible_child_name (GTK_STACK (window->image_stack), "impostor");
  gtk_stack_set_transition_type (GTK_STACK (window->image_stack),
                                 GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);

  ws_album_view_clear (WS_ALBUM_VIEW (window->album_view));

  if (G_IS_CANCELLABLE (window->cancellables[0]))
    {
      g_cancellable_cancel (window->cancellables[0]);
      g_object_unref (window->cancellables[0]);
    }

  window->cancellables[0] = g_cancellable_new ();

  window->current_album_index ++;
  window->current_image_index = 0;

  /* TODO: Remove this and load the next page instead */
  if (window->current_album_index >= window->gallery->n_albums)
    g_error ("New album index too high: %d but only have %d albums",
             window->current_album_index, window->gallery->n_albums);

  album = &window->gallery->albums[window->current_album_index];

  ws_album_view_reserve_space (WS_ALBUM_VIEW (window->album_view),
                               album);

  ws_image_loader_load_album_async (loader,
                                    album,
                                    NULL,
                                    album_loaded_cb,
                                    window);

  ws_image_loader_load_image_async (loader,
                                    &album->images[window->current_image_index],
                                    window->cancellables[0],
                                    image_loaded_cb,
                                    window);

  g_snprintf (buff, sizeof (buff), "%s (%d)", album->title, album->n_images);
  gtk_window_set_title (GTK_WINDOW (window), buff);

  if (window->current_album_index > 0)
    gtk_widget_set_sensitive (window->prev_button, TRUE);

  gtk_stack_set_visible_child_name (GTK_STACK (window->image_stack), "album");
}

static void
show_prev_album (WsMainWindow *window)
{
  ImgurAlbum *album;
  WsImageLoader *loader = window->loader;
  char buff[4096];

  if (window->current_album_index == 0)
    return;

  window->current_album_index --;
  window->current_image_index = 0;

  album = &window->gallery->albums[window->current_album_index];
  ws_album_view_reserve_space (WS_ALBUM_VIEW (window->album_view),
                               album);

  ws_image_loader_load_image_async (loader,
                                    &album->images[0],
                                    NULL,
                                    image_loaded_cb,
                                    window);

  if (window->current_album_index == 0)
    gtk_widget_set_sensitive (window->prev_button, FALSE);

  g_snprintf (buff, sizeof (buff), "%s (%d)", album->title, album->n_images);
  gtk_window_set_title (GTK_WINDOW (window), buff);
}

void
next_button_clicked_cb (GtkButton *button,
                        gpointer   user_data)
{
  WsMainWindow *window = user_data;

  show_next_album (window);
}

void
prev_button_clicked_cb (GtkButton *button,
                        gpointer   user_data)
{
  WsMainWindow *window = user_data;

  show_prev_album (window);
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
gallery_loaded_cb (GObject      *source_object,
                   GAsyncResult *result,
                   gpointer      user_data)
{
  GError *error = NULL;
  WsImageLoader *loader = WS_IMAGE_LOADER (source_object);
  WsMainWindow *window = user_data;
  ImgurGallery *gallery;

  gallery = ws_image_loader_load_gallery_finish (loader, result, &error);

  if (error != NULL)
    {
      g_warning ("%s(%d): %s", __FILE__, __LINE__, error->message);
      return;
    }

  /* -1 so show_next_album will incrase it to 0 and load that */
  window->current_album_index = -1;
  window->current_image_index = -1;
  gtk_stack_set_visible_child_name (GTK_STACK (window->main_stack), "image");
  gtk_stack_set_visible_child_name (GTK_STACK (window->image_stack), "album");

  /* TODO: This leaks the gallery and everything it contains! */
  g_free (window->gallery);
  window->gallery = gallery;

  show_next_album (window);
}

static void
go_next_cb (GSimpleAction *action,
            GVariant      *v,
            gpointer       user_data)
{
  WsMainWindow *window = user_data;

  show_next_album (window);
}


static void
go_prev_cb (GSimpleAction *action,
            GVariant      *v,
            gpointer       user_data)
{
  WsMainWindow *window = user_data;

  show_prev_album (window);
}

static void
go_down_cb (GSimpleAction *action,
            GVariant      *v,
            gpointer       user_data)
{
  WsMainWindow *self = user_data;
  ImgurAlbum *album;

  album = &self->gallery->albums[self->current_album_index];

  if (self->current_image_index < album->n_images - 1)
    {
      ImgurImage *image;

      self->current_image_index ++;
      image = &album->images[self->current_image_index];

      if (image->paintable == NULL)
        ws_image_loader_load_image_async (self->loader,
                                          &album->images[self->current_image_index],
                                          self->cancellables[0],
                                          image_loaded_cb,
                                          self);

      ws_album_view_scroll_to_next (WS_ALBUM_VIEW (self->album_view));
    }
}

static void
go_up_cb (GSimpleAction *action,
          GVariant      *v,
          gpointer       user_data)
{
  WsMainWindow *self = user_data;
  ImgurAlbum *album;

  album = &self->gallery->albums[self->current_album_index];

  if (self->current_image_index > 0)
    {
      self->current_image_index --;

      ws_album_view_scroll_to_prev (WS_ALBUM_VIEW (self->album_view));
    }
}


static GActionEntry win_entries[] = {
  { "go-next", go_next_cb, NULL, NULL, NULL },
  { "go-prev", go_prev_cb, NULL, NULL, NULL },
  { "go-down", go_down_cb, NULL, NULL, NULL },
  { "go-up",   go_up_cb,   NULL, NULL, NULL }
};

void
ws_main_window_init (WsMainWindow *win)
{
  Waster *app = (Waster *)g_application_get_default ();

  gtk_widget_init_template (GTK_WIDGET (win));

  g_action_map_add_action_entries (G_ACTION_MAP (win),
                                   win_entries, G_N_ELEMENTS (win_entries),
                                   win);

  win->loader = ws_image_loader_new ();
  win->cancellables[0] = win->cancellables[1] = NULL;

  if (waster_is_proxy_inited (app))
    {
      gtk_stack_set_visible_child_name (GTK_STACK (win->main_stack), "spinner");
      gtk_spinner_start (GTK_SPINNER (win->spinner));
      ws_image_loader_load_gallery_async (win->loader,
                                          &IMGUR_GALLERIES[0],
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
  gtk_widget_class_bind_template_child (widget_class, WsMainWindow, impostor);

  gtk_widget_class_bind_template_callback (widget_class, next_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, prev_button_clicked_cb);
}
