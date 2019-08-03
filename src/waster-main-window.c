#include "imgur-gallery.h"

#include "waster-main-window.h"
#include "waster-image-loader.h"
#include "waster-initial-state.h"
#include "waster-album-view.h"
#include "waster-impostor.h"
#include "waster-notification.h"
#include "waster.h"

#define LOOKAHEAD 1


static void image_loaded_cb (GObject      *source_object,
                             GAsyncResult *result,
                             gpointer      user_data);


struct _WsMainWindow
{
  GtkApplicationWindow parent_instance;
  GtkWidget *main_stack;
  GtkWidget *next_button;
  GtkWidget *prev_button;
  GtkWidget *initial_state;
  GtkWidget *spinner;
  GtkWidget *album_stack;
  GtkWidget *album_view;
  GtkWidget *impostor;

  ImgurGallery *gallery;

  WsImageLoader *loader;

  int current_album_index;
  int current_image_index;

  GCancellable *image_cancellables[1 + LOOKAHEAD];

  GtkWidget *notification; /* NULL if no notification is being shown */
  guint notification_id;
};

typedef struct _WsMainWindow WsMainWindow;

G_DEFINE_TYPE (WsMainWindow, ws_main_window, GTK_TYPE_APPLICATION_WINDOW);

static void
refresh_cancellable (GCancellable **c)
{
  g_assert (c);

  if (*c != NULL)
    {
      g_cancellable_cancel (*c);
      g_object_unref (*c);
    }

  *c = g_cancellable_new ();
}

static void
ws_main_window_show_image (WsMainWindow *self,
                           int           image_index)
{
  const ImgurAlbum *album;
  int i;

  self->current_image_index = image_index;

  album = &self->gallery->albums[self->current_album_index];

  /* Load the current image */
  refresh_cancellable (&self->image_cancellables[0]);
  ws_image_loader_load_image_async (self->loader,
                                    &album->images[image_index],
                                    self->image_cancellables[0],
                                    image_loaded_cb,
                                    self);

  /* Show first image */
  ws_album_view_show_image (WS_ALBUM_VIEW (self->album_view), &album->images[image_index]);

  for (i = 0; i < MIN (LOOKAHEAD, album->n_images - image_index - 1); i ++)
    {
      refresh_cancellable (&self->image_cancellables[1 + i]);
      ws_image_loader_load_image_async (self->loader,
                                        &album->images[image_index + 1 + i],
                                        self->image_cancellables[1 + i],
                                        image_loaded_cb,
                                        self);
    }

  if (self->current_album_index < self->gallery->n_albums - 1)
    {
      const ImgurAlbum *next_album = &self->gallery->albums[self->current_album_index + 1];
      ws_image_loader_load_image_async (self->loader,
                                        &next_album->images[0],
                                        NULL,
                                        image_loaded_cb,
                                        self);
    }
}

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
}

static void
show_next_album (WsMainWindow *window)
{
  WsImageLoader *loader = window->loader;
  ImgurAlbum *album;
  char buff[4096];

  window->current_album_index ++;
  window->current_image_index = 0;

  /* TODO: Remove this and load the next page instead */
  if (window->current_album_index >= window->gallery->n_albums)
    g_error ("New album index too high: %d but only have %d albums",
             window->current_album_index, window->gallery->n_albums);

  album = &window->gallery->albums[window->current_album_index];

  ws_album_view_set_album (WS_ALBUM_VIEW (window->album_view), album);

  ws_image_loader_load_album_async (loader,
                                    album,
                                    NULL,
                                    album_loaded_cb,
                                    window);

  g_snprintf (buff, sizeof (buff), "%s (%d)", album->title, album->n_images);
  gtk_window_set_title (GTK_WINDOW (window), buff);

  if (window->current_album_index > 0)
    gtk_widget_set_sensitive (window->prev_button, TRUE);

  gtk_stack_set_visible_child_name (GTK_STACK (window->album_stack), "album");

  ws_main_window_show_image (window, 0);
}

static void
show_prev_album (WsMainWindow *self)
{
  ImgurAlbum *album;
  WsImageLoader *loader = self->loader;
  char buff[4096];

  if (self->current_album_index == 0)
    return;

  self->current_album_index --;
  self->current_image_index = 0;

  album = &self->gallery->albums[self->current_album_index];
  ws_album_view_set_album (WS_ALBUM_VIEW (self->album_view), album);

  ws_image_loader_load_image_async (loader,
                                    &album->images[0],
                                    NULL,
                                    image_loaded_cb,
                                    self);

  if (self->current_album_index == 0)
    gtk_widget_set_sensitive (self->prev_button, FALSE);

  g_snprintf (buff, sizeof (buff), "%s (%d)", album->title, album->n_images);
  gtk_window_set_title (GTK_WINDOW (self), buff);
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

  {
    GtkStackTransitionType old = gtk_stack_get_transition_type (GTK_STACK (window->main_stack));

    gtk_stack_set_transition_type (GTK_STACK (window->main_stack), GTK_STACK_TRANSITION_TYPE_NONE);
    gtk_stack_set_visible_child_name (GTK_STACK (window->main_stack), "image");
    gtk_stack_set_transition_type (GTK_STACK (window->main_stack), old);
  }

  {
    GtkStackTransitionType old = gtk_stack_get_transition_type (GTK_STACK (window->album_stack));

    gtk_stack_set_transition_type (GTK_STACK (window->album_stack), GTK_STACK_TRANSITION_TYPE_NONE);
    gtk_stack_set_visible_child_name (GTK_STACK (window->album_stack), "album");
    gtk_stack_set_transition_type (GTK_STACK (window->album_stack), old);
  }

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

  ws_album_view_scroll_to_next (WS_ALBUM_VIEW (self->album_view));

  ws_main_window_show_image (self, WS_ALBUM_VIEW (self->album_view)->cur_image_index);
}

static void
go_up_cb (GSimpleAction *action,
          GVariant      *v,
          gpointer       user_data)
{
  WsMainWindow *self = user_data;
  ImgurAlbum *album;

  album = &self->gallery->albums[self->current_album_index];

  ws_album_view_scroll_to_prev (WS_ALBUM_VIEW (self->album_view));
}

static void
save_current_cb (GSimpleAction *action,
                 GVariant      *v,
                 gpointer       user_data)
{
  WsMainWindow *self = user_data;
  const ImgurAlbum *album;
  const ImgurImage *image;
  char buff[512];

  album = &self->gallery->albums[self->current_album_index];
  image = &album->images[self->current_image_index];

  if (GDK_IS_TEXTURE (image->paintable))
    {
      char *filename = g_strdup_printf ("./meme.png");

      gdk_texture_save_to_png (GDK_TEXTURE (image->paintable), filename);

      g_snprintf (buff, sizeof (buff), "Saved to '%s'", filename);
      ws_main_window_show_notification (self, buff);

      g_free (filename);
    }
  else
    {
      ws_main_window_show_notification (self, "Can't save videos :(");
    }
}

static GActionEntry win_entries[] = {
  { "go-next",      go_next_cb, NULL, NULL, NULL },
  { "go-prev",      go_prev_cb, NULL, NULL, NULL },
  { "go-down",      go_down_cb, NULL, NULL, NULL },
  { "go-up",        go_up_cb,   NULL, NULL, NULL },
  { "save-current", save_current_cb, NULL, NULL, NULL },
};

void
ws_main_window_init (WsMainWindow *self)
{
  Waster *app = (Waster *)g_application_get_default ();

  gtk_widget_init_template (GTK_WIDGET (self));

  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   win_entries, G_N_ELEMENTS (win_entries),
                                   self);

  self->loader = ws_image_loader_new ();

  if (waster_is_proxy_inited (app))
    {
      gtk_stack_set_visible_child_name (GTK_STACK (self->main_stack), "spinner");
      gtk_spinner_start (GTK_SPINNER (self->spinner));
      ws_image_loader_load_gallery_async (self->loader,
                                          &IMGUR_GALLERIES[0],
                                          NULL,
                                          gallery_loaded_cb,
                                          self);
    }
  else
    {
      gtk_stack_set_visible_child_name (GTK_STACK (self->main_stack), "initial_state");
    }

  gtk_window_set_default_size (GTK_WINDOW (self), 1024, 768);

}

static void
ws_main_window_dispose (GObject *object)
{
  WsMainWindow *self = (WsMainWindow *)object;
  int i;

  for (i = 0; i < G_N_ELEMENTS (self->image_cancellables); i ++)
    g_clear_object (&self->image_cancellables[i]);

  if (self->gallery)
    {
      for (i = 0; i < self->gallery->n_albums; i ++)
        {
          ImgurAlbum *album = &self->gallery->albums[i];
          int k;

          g_clear_pointer (&album->title, g_free);

          for (k = 0; k < album->n_images; k ++)
            {
              ImgurImage *image = &album->images[k];

              g_clear_pointer (&image->id, g_free);
              g_clear_pointer (&image->title, g_free);
              g_clear_pointer (&image->link, g_free);
              g_clear_object (&image->paintable);
            }
          g_clear_pointer (&album->images, g_free);
        }
      g_clear_pointer (&self->gallery->albums, g_free);
      g_clear_pointer (&self->gallery, g_free);
    }

  g_clear_object (&self->loader);
  g_clear_pointer (&self->notification, gtk_widget_unparent);

  G_OBJECT_CLASS (ws_main_window_parent_class)->dispose (object);
}

static void
ws_main_window_size_allocate (GtkWidget *widget,
                              int        width,
                              int        height,
                              int        baseline)
{
  WsMainWindow *self = (WsMainWindow *)widget;

  GTK_WIDGET_CLASS (ws_main_window_parent_class)->size_allocate (widget, width, height, baseline);

  if (self->notification)
    {
      #define FUCK_ME 80
      int w, h;

      gtk_widget_measure (self->notification, GTK_ORIENTATION_HORIZONTAL, -1,
                          NULL, &w, NULL, NULL);
      gtk_widget_measure (self->notification, GTK_ORIENTATION_VERTICAL, w,
                          NULL, &h, NULL, NULL);

      gtk_widget_size_allocate (self->notification,
                                &(GtkAllocation) {
                                  (width - w) / 2, FUCK_ME,
                                  w, h
                                }, -1);

    }

}

void
ws_main_window_class_init (WsMainWindowClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  object_class->dispose = ws_main_window_dispose;

  widget_class->size_allocate = ws_main_window_size_allocate;


  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/baedert/waster/ui/main-window.ui");

  gtk_widget_class_bind_template_child (widget_class, WsMainWindow, main_stack);
  gtk_widget_class_bind_template_child (widget_class, WsMainWindow, initial_state);
  gtk_widget_class_bind_template_child (widget_class, WsMainWindow, prev_button);
  gtk_widget_class_bind_template_child (widget_class, WsMainWindow, next_button);
  gtk_widget_class_bind_template_child (widget_class, WsMainWindow, album_stack);
  gtk_widget_class_bind_template_child (widget_class, WsMainWindow, spinner);
  gtk_widget_class_bind_template_child (widget_class, WsMainWindow, album_view);
  gtk_widget_class_bind_template_child (widget_class, WsMainWindow, impostor);

  gtk_widget_class_bind_template_callback (widget_class, next_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, prev_button_clicked_cb);
}

static gboolean
notification_timeout_cb (gpointer user_data)
{
  WsMainWindow *self = user_data;

  g_clear_pointer (&self->notification, gtk_widget_unparent);
  self->notification_id = 0;

  return G_SOURCE_REMOVE;
}

void
ws_main_window_show_notification (WsMainWindow *self,
                                  const char   *message)
{
  g_clear_pointer (&self->notification, gtk_widget_unparent);

  if (self->notification_id != 0)
    g_source_remove (self->notification_id);

  self->notification = ws_notification_new (message);
  gtk_widget_set_parent (self->notification, GTK_WIDGET (self));

  self->notification_id = g_timeout_add_seconds (3, notification_timeout_cb, self);
  ws_notification_show ((WsNotification *)self->notification);
}
