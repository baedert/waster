#include "imgur-gallery.h"

#include "waster-main-window.h"
#include "waster-image-loader.h"
#include "waster-initial-state.h"
#include "waster-album-view.h"
#include "waster-impostor.h"
#include "waster-notification.h"
#include "gallery-manager.h"
#include "waster-gallery.h"
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
  GtkWidget *save_button;

  WsGalleryManager *gallery_manager;
  WsGallery *gallery;

  WsImageLoader *loader;

  int current_image_index;

  GCancellable *image_cancellables[1 + LOOKAHEAD];

  GtkWidget *notification; /* NULL if no notification is being shown */
  guint notification_id;
};

typedef struct _WsMainWindow WsMainWindow;

G_DEFINE_TYPE (WsMainWindow, ws_main_window, GTK_TYPE_APPLICATION_WINDOW);

static void
image_loaded_cb (GObject      *source_object,
                 GAsyncResult *result,
                 gpointer      user_data)
{
  WsMainWindow *self = user_data;
  GError *error = NULL;
  WsImage *image;

  image = ws_gallery_manager_load_image_finish (self->gallery_manager, result, &error);
  g_assert (image);

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

  ws_album_view_show_image (WS_ALBUM_VIEW (self->album_view), image);
}

static void
image_loaded_cb_ignore (GObject      *source_object,
                        GAsyncResult *result,
                        gpointer      user_data)
{
  WsMainWindow *self = user_data;
  GError *error = NULL;
  WsImage *image;

  image = ws_gallery_manager_load_image_finish (self->gallery_manager, result, &error);
  g_assert (image);

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
show_next_image (WsMainWindow *self)
{
  WsImage *image;

  if (!self->gallery)
    return;

  self->current_image_index ++;

  // TODO: What to do once we've reached the end of the gallery?

  ws_gallery_manager_load_image_async (self->gallery_manager,
                                       self->gallery,
                                       self->current_image_index,
                                       NULL,
                                       image_loaded_cb,
                                       self);

  /* Preload next image */
  ws_gallery_manager_load_image_async (self->gallery_manager,
                                       self->gallery,
                                       self->current_image_index + 1,
                                       NULL,
                                       image_loaded_cb_ignore,
                                       self);


  image = ws_gallery_get_image (self->gallery, self->current_image_index);
  if (image->title)
    gtk_window_set_title (GTK_WINDOW (self), image->title);
  else
    gtk_window_set_title (GTK_WINDOW (self), "Waster");

  if (self->current_image_index > 0)
    gtk_widget_set_sensitive (self->prev_button, TRUE);

  gtk_stack_set_visible_child_name (GTK_STACK (self->album_stack), "album");
}

void
next_button_clicked_cb (GtkButton *button,
                        gpointer   user_data)
{
  WsMainWindow *window = user_data;

  show_next_image (window);
}

void
prev_button_clicked_cb (GtkButton *button,
                        gpointer   user_data)
{
  WsMainWindow *window = user_data;
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
  WsGalleryManager *manager = (WsGalleryManager *)source_object;
  GError *error = NULL;
  WsMainWindow *window = user_data;
  WsGallery *gallery;

  gallery = ws_gallery_manager_load_gallery_finish (manager, result, &error);

  if (error != NULL)
    {
      g_warning ("%s(%d): %s", __FILE__, __LINE__, error->message);
      return;
    }

  /* -1 so show_next_image will incrase it to 0 and load that */
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

  show_next_image (window);
}

static void
go_next_cb (GSimpleAction *action,
            GVariant      *v,
            gpointer       user_data)
{
  WsMainWindow *window = user_data;

  show_next_image (window);
}

static void
go_prev_cb (GSimpleAction *action,
            GVariant      *v,
            gpointer       user_data)
{
  WsMainWindow *window = user_data;
}

static void
go_down_cb (GSimpleAction *action,
            GVariant      *v,
            gpointer       user_data)
{
  WsMainWindow *self = user_data;
}

static void
go_up_cb (GSimpleAction *action,
          GVariant      *v,
          gpointer       user_data)
{
  WsMainWindow *self = user_data;
}

static void
save_current_cb (GSimpleAction *action,
                 GVariant      *v,
                 gpointer       user_data)
{
  WsMainWindow *self = user_data;
#if 0
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
#endif
}

static void
toggle_muted_cb (GSimpleAction *action,
                 GVariant      *v,
                 gpointer       user_data)
{
  WsMainWindow *self = user_data;
  GSettings *settings;
  gboolean value;

  settings = ((Waster *)(g_application_get_default ()))->settings;
  value = g_settings_get_boolean (settings, "muted");

  value = !value;
  g_settings_set_boolean (settings, "muted", value);

  ws_album_view_set_muted (WS_ALBUM_VIEW (self->album_view), value);
}

static GActionEntry win_entries[] = {
  { "go-next",      go_next_cb, NULL, NULL, NULL },
  { "go-prev",      go_prev_cb, NULL, NULL, NULL },
  { "go-down",      go_down_cb, NULL, NULL, NULL },
  { "go-up",        go_up_cb,   NULL, NULL, NULL },
  { "save-current", save_current_cb, NULL, NULL, NULL },
  { "toggle-muted",  toggle_muted_cb, NULL, NULL, NULL },
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
  self->gallery_manager = ws_gallery_manager_new ();

  if (waster_is_proxy_inited (app))
    {
      gtk_stack_set_visible_child_name (GTK_STACK (self->main_stack), "spinner");
      gtk_spinner_start (GTK_SPINNER (self->spinner));

      /* TODO: Save selected gallery and load that one here */
     ws_gallery_manager_load_gallery_async (self->gallery_manager,
                                            &IMGUR_GALLERIES[0],
                                            NULL,
                                            gallery_loaded_cb,
                                            self);
    }
  else
    {
      gtk_stack_set_visible_child_name (GTK_STACK (self->main_stack), "initial_state");
      gtk_widget_hide (self->save_button);
      gtk_widget_hide (self->next_button);
      gtk_widget_hide (self->prev_button);
    }

  gtk_window_set_default_size (GTK_WINDOW (self), 1024, 768);

}

static void
ws_main_window_dispose (GObject *object)
{
  WsMainWindow *self = (WsMainWindow *)object;
  guint i;

  for (i = 0; i < G_N_ELEMENTS (self->image_cancellables); i ++)
    g_clear_object (&self->image_cancellables[i]);

  // TODO: gallery manager

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
  gtk_widget_class_bind_template_child (widget_class, WsMainWindow, save_button);

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
