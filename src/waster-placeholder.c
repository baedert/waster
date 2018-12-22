
#include "waster-placeholder.h"

#define WS_TYPE_PLACEHOLDER (ws_placeholder_get_type ())
static
G_DECLARE_FINAL_TYPE(WsPlaceholder, ws_placeholder, WS, PLACEHOLDER, GObject)

/* This is a lot like GdkEmptyPaintable but allows us to show some sort
 * of animation while loading if we ever want to do that. */
struct _WsPlaceholder
{
  GObject parent_instance;

  int width;
  int height;
};

struct _WsPlaceholderClass
{
  GObjectClass parent_class;
};

static void
ws_placeholder_snapshot (GdkPaintable *paintable,
                         GdkSnapshot  *snapshot,
                         double        width,
                         double        height)
{
  g_assert (WS_IS_PLACEHOLDER (paintable));

  gtk_snapshot_append_color (snapshot,
                             &(GdkRGBA) { 1, 0, 0, 1 },
                             &GRAPHENE_RECT_INIT (0, 0, width, height));
}

static GdkPaintableFlags
ws_placeholder_get_flags (GdkPaintable *paintable)
{
  return GDK_PAINTABLE_STATIC_SIZE
       | GDK_PAINTABLE_STATIC_CONTENTS;
}

static int
ws_placeholder_get_intrinsic_width (GdkPaintable *paintable)
{
  WsPlaceholder *self = WS_PLACEHOLDER (paintable);

  return self->width;
}

static int
ws_placeholder_get_intrinsic_height (GdkPaintable *paintable)
{
  WsPlaceholder *self = WS_PLACEHOLDER (paintable);

  return self->height;
}

static void
ws_placeholder_paintable_init (GdkPaintableInterface *iface)
{
  iface->snapshot = ws_placeholder_snapshot;
  iface->get_flags = ws_placeholder_get_flags;
  iface->get_intrinsic_width = ws_placeholder_get_intrinsic_width;
  iface->get_intrinsic_height = ws_placeholder_get_intrinsic_height;
}

G_DEFINE_TYPE_WITH_CODE (WsPlaceholder, ws_placeholder, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE,
                                                ws_placeholder_paintable_init))

static void
ws_placeholder_init (WsPlaceholder *self)
{
}

static void
ws_placeholder_class_init (WsPlaceholderClass *klass)
{
}

GdkPaintable *
ws_placeholder_new (int width,
                    int height)
{
  WsPlaceholder *self = (WsPlaceholder *)g_object_new (WS_TYPE_PLACEHOLDER, NULL);

  self->width = width;
  self->height = height;

  return (GdkPaintable *)self;
}
