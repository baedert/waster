#include "waster.h"
#include "waster-initial-state.h"
#include "waster-album-view.h"
#include "waster-impostor.h"


int
main (int    argc,
      char **argv)
{
  Waster *app;
  int ret;

  g_type_ensure (WS_TYPE_INITIAL_STATE);
  g_type_ensure (WS_TYPE_ALBUM_VIEW);
  g_type_ensure (WS_TYPE_IMPOSTOR);

  app = waster_new ();

  ret = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);
  return ret;
}
