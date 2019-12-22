#include <gtk/gtk.h>
#include <rest/rest-proxy.h>
#include <rest/rest-proxy-call.h>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>
#include <string.h>

#include "waster-initial-state.h"
#include "waster.h"


#define NUM_CIRCLES 60
#define CIRCLE_SIZE 40

typedef struct
{
  GskRoundedRect outline;
  GdkRGBA color;
} Circle;

struct _WsInitialState
{
  GtkWidget parent_instance;

  GtkWidget *pin_entry;
  GtkWidget *request_pin_button;
  GtkWidget *submit_button;

  guint circles_inited : 1;
  Circle circles[NUM_CIRCLES];
};

typedef struct _WsInitialState WsInitialState;

G_DEFINE_TYPE (WsInitialState, ws_initial_state, GTK_TYPE_WIDGET);

GtkWidget *
ws_initial_state_new (void)
{
  return GTK_WIDGET (g_object_new (WS_TYPE_INITIAL_STATE, NULL));
}


static void
request_pin_button_clicked_cb ()
{
  Waster *app = (Waster *)g_application_get_default ();
  char *login_url = waster_get_login_url (app);

  g_app_info_launch_default_for_uri (login_url, NULL, NULL);
  g_free (login_url);
}

static void
submit_button_clicked_cb (GtkWidget      *source,
                          WsInitialState *initial_state)
{
  Waster *app = (Waster *) (g_application_get_default ());
  const char *pin = gtk_editable_get_text (GTK_EDITABLE (initial_state->pin_entry));

  waster_maybe_refresh_token (app, pin);
}

static void
snapshot_circle (GtkSnapshot  *snapshot,
                 const Circle *circle)
{
  gtk_snapshot_push_rounded_clip (snapshot, &circle->outline);
  gtk_snapshot_append_color (snapshot,
                             &circle->color,
                             &circle->outline.bounds);
  gtk_snapshot_pop (snapshot);
}

static void
ws_initial_state_snapshot (GtkWidget   *widget,
                           GtkSnapshot *snapshot)
{
  WsInitialState *self = (WsInitialState *)widget;
  const int width = gtk_widget_get_width (widget);
  const int height = gtk_widget_get_height (widget);
  int i;

  if (G_UNLIKELY (!self->circles_inited))
    {
      GdkRGBA colors[4];

      gdk_rgba_parse (&colors[0], "#4baea0");
      gdk_rgba_parse (&colors[1], "#b6e6bd");
      gdk_rgba_parse (&colors[2], "#f1f0cf");
      gdk_rgba_parse (&colors[3], "#f0c9c9");

      for (i = 0; i < NUM_CIRCLES; i ++)
        {
          const float size = g_random_double_range (CIRCLE_SIZE * 0.5, CIRCLE_SIZE * 2.5);
          const GdkRGBA *color = &colors[(int)(g_random_double () * G_N_ELEMENTS (colors))];
          const graphene_rect_t bounds = GRAPHENE_RECT_INIT (
                                           g_random_double () * width,
                                           g_random_double () * height,
                                           size, size
                                         );
          int p;
          gboolean keep = TRUE;

          /* This new circle should not collide with any already exsisting one... */
          for (p = 0; p < i; p ++)
            {
              if (graphene_rect_intersection (&bounds, &self->circles[p].outline.bounds, NULL))
                {
                  keep = FALSE;
                  break;
                }
            }

          if (keep)
            {
              self->circles[i].color = *color;
              gsk_rounded_rect_init_from_rect (&self->circles[i].outline, &bounds, size);
            }
          else
            {
             i--;
             continue;
            }
        }
      self->circles_inited = TRUE;
    }

  gtk_snapshot_push_blur (snapshot, 5.0);
  for (i = 0; i < NUM_CIRCLES; i ++)
    {
      snapshot_circle (snapshot, &self->circles[i]);
    }
  gtk_snapshot_pop (snapshot);

  GTK_WIDGET_CLASS (ws_initial_state_parent_class)->snapshot (widget, snapshot);
}

static void
ws_initial_state_dispose (GObject *object)
{
  GtkWidget *w;

  w = gtk_widget_get_first_child (GTK_WIDGET (object));
  while (w)
    {
      GtkWidget *next = gtk_widget_get_next_sibling (w);

      gtk_widget_unparent (w);

      w = next;
    }

  G_OBJECT_CLASS (ws_initial_state_parent_class)->dispose (object);
}

void
ws_initial_state_init (WsInitialState *state)
{
  gtk_widget_init_template (GTK_WIDGET (state));

  gtk_widget_set_overflow (GTK_WIDGET (state), GTK_OVERFLOW_HIDDEN);
}

void
ws_initial_state_class_init (WsInitialStateClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  object_class->dispose = ws_initial_state_dispose;

  widget_class->snapshot = ws_initial_state_snapshot;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/baedert/waster/ui/initial-state.ui");

  gtk_widget_class_bind_template_child (widget_class, WsInitialState, pin_entry);
  gtk_widget_class_bind_template_child (widget_class, WsInitialState, request_pin_button);
  gtk_widget_class_bind_template_child (widget_class, WsInitialState, submit_button);

  gtk_widget_class_bind_template_callback (widget_class, request_pin_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, submit_button_clicked_cb);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BOX_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, "initialstate");
}
