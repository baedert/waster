
#ifndef __BT_LIST_BOX
#define __BT_LIST_BOX

#include <gtk/gtk.h>

#define BT_TYPE_LIST_BOX (bt_list_box_get_type ())
G_DECLARE_FINAL_TYPE (BtListBox, bt_list_box, BT, LIST_BOX, GtkContainer)

struct _BtListBox
{
  GtkContainer parent_instance;
};

typedef struct _BtListBox BtListBox;


typedef GtkWidget * (*BtListBoxWidgetFillFunc) (GObject   *item,
                                                GtkWidget *old_widget);

typedef gboolean (*BtListBoxFilterFunc) (GObject *item);


GType       bt_list_box_get_type (void) G_GNUC_CONST;

GtkWidget * bt_list_box_new ();

void        bt_list_box_set_model (BtListBox *list_box, GListModel *model);


void bt_list_box_set_widget_fill_func (BtListBox               *list_box,
                                       BtListBoxWidgetFillFunc  fill_func);

void bt_list_box_set_filter_func (BtListBox            *list_box,
                                  BtListBoxFilterFunc  filter_func);


#endif
