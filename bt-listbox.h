
#ifndef __BT_LIST_BOX
#define __BT_LIST_BOX

#include <gtk/gtk.h>

#define BT_TYPE_LIST_BOX (bt_list_box_get_type ())
G_DECLARE_FINAL_TYPE (BtListBox, bt_list_box, BT, LIST_BOX, GtkContainer)




typedef GtkWidget * (*BtListBoxWidgetFillFunc) (GObject   *item,
                                                GtkWidget *old_widget);


GtkWidget * bt_list_box_new ();

void bt_list_box_set_widget_fill_func (BtListBox               *list,
                                       BtListBoxWidgetFillFunc  fill_func);


#endif
