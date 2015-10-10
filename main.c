#include "waster.h"
#include "waster-initial-state.h"


int
main (int    argc,
      char **argv)
{
  Waster *app;
  int ret;

  g_type_ensure (WS_TYPE_INITIAL_STATE);

  app = waster_new ();

  ret = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);
  return ret;
}
