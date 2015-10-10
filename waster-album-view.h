#include <gtk/gtk.h>

struct _WsAlbumView
{
  GtkBox parent_instance;
};

typedef struct _WsAlbumView WsAlbumView;





#define WS_TYPE_ALBUM_VIEW ws_album_view_get_type()

G_DECLARE_FINAL_TYPE (WsAlbumView, ws_album_view, WS, ALBUM_VIEW, GtkBox);


