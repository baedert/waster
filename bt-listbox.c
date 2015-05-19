#include "bt-listbox.h"

#include <gtk/gtk.h>


typedef struct _BtListBoxPrivate BtListBoxPrivate;

struct _BtListBox
{
  GtkContainer parent_instance;

  BtListBoxPrivate *priv;
};

struct _BtListBoxPrivate
{
  GtkAdjustment *vadjustment;
  GtkAdjustment *hadjustment;
  GdkWindow     *bin_window;
  int            bin_y_diff;
  GPtrArray     *widgets;
  GList         *old_widgets;


  BtListBoxWidgetFillFunc fill_func;
};

enum
{
  PROP_0,
  PROP_HADJUSTMENT,
  PROP_VADJUSTMENT,
  PROP_HSCROLL_POLICY,
  PROP_VSCROLL_POLICY
};


G_DEFINE_TYPE_WITH_CODE (BtListBox, bt_list_box,
                         GTK_TYPE_CONTAINER,
                         G_ADD_PRIVATE (BtListBox)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_SCROLLABLE, NULL))


static void
bt_list_box_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  BtListBoxPrivate *priv = bt_list_box_get_instance_private (BT_LIST_BOX (object));

  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      gtk_scrollable_set_hadjustment (GTK_SCROLLABLE (object), g_value_get_object (value));
      break;
    case PROP_VADJUSTMENT:
      gtk_scrollable_set_vadjustment (GTK_SCROLLABLE (object), g_value_get_object (value));
      break;
    /*case PROP_HSCROLL_POLICY:*/
    /*case PROP_VSCROLL_POLICY:*/
    }

}

static void
bt_list_box_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  BtListBoxPrivate *priv = bt_list_box_get_instance_private (BT_LIST_BOX (object));

  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      g_value_set_object (value, priv->hadjustment);
      break;
    case PROP_VADJUSTMENT:
      g_value_set_object (value, priv->vadjustment);
      break;
    /*case PROP_HSCROLL_POLICY:*/
    /*case PROP_VSCROLL_POLICY:*/
    }
}


static inline int
get_widget_height (GtkWidget *instance,
                   GtkWidget *widget)
{
  int min, nat;
  gtk_widget_get_preferred_height_for_width (widget,
                                             gtk_widget_get_allocated_width (instance),
                                             &min, &nat);

  g_assert (min >= 1);

  return min;
}

static void
ensure_visible_widgets ()
{

}

static void
position_children ()
{

}


static void
update_bin_window ()
{

}

/* GtkContainer API {{{ */
static void
bt_list_box_forall (GtkContainer *container,
                    gboolean      include_internals,
                    GtkCallback   callback,
                    gpointer      callback_data)
{

}

static GType
bt_list_box_child_type (GtkContainer *container)
{
  return GTK_TYPE_WIDGET;
}

static void
dont_call_this ()
{
  g_error ("Don't call this.");
}


/* }}} */


/* GtkWidget API {{{ */

static gboolean
bt_list_box_draw (GtkWidget *widget,
                  cairo_t   *ct)
{
  BtListBoxPrivate *priv = bt_list_box_get_instance_private (BT_LIST_BOX (widget));

  if (gtk_cairo_should_draw_window (ct, priv->bin_window))
    {
      // XXX propagate_draw all children
    }

  return FALSE;
}

static void
bt_list_box_size_allocate (GtkWidget     *widget,
                           GtkAllocation *allocation)
{
  gtk_widget_set_allocation (widget, allocation);
  position_children ();

  if (gtk_widget_get_realized (widget))
    {
      GdkWindow *win = gtk_widget_get_window (widget);
      gdk_window_move_resize (win,
                              allocation->x,
                              allocation->y,
                              allocation->width,
                              allocation->height);
      update_bin_window ();
    }

  ensure_visible_widgets ();
}

static void
bt_list_box_realize (GtkWidget *widget)
{
  BtListBoxPrivate *priv = bt_list_box_get_instance_private (BT_LIST_BOX (widget));
  BtListBox *list = BT_LIST_BOX (widget);
  GtkAllocation  allocation;
  GdkWindow     *window;
  GdkWindowAttr  attributes;
  int            attributes_mask;


  gtk_widget_set_realized (widget, TRUE);
  gtk_widget_get_allocation (widget, &allocation);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.height = allocation.height;
  attributes.width = allocation.width;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = GDK_ALL_EVENTS_MASK;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

  window = gdk_window_new (gtk_widget_get_parent_window (widget),
                           &attributes,
                           attributes_mask);
  gtk_widget_set_window (widget, window);
  gtk_widget_register_window (widget, window);



  priv->bin_window = gdk_window_new (window,
                                     &attributes,
                                     attributes_mask);
  gtk_widget_register_window (widget, priv->bin_window);
  gdk_window_show (priv->bin_window);


  // XXX Set the parent window of all existing child widgets

}

/* }}} */

static void
bt_list_box_finalize (GObject *object)
{
  BtListBoxPrivate *priv = bt_list_box_get_instance_private (BT_LIST_BOX (object));

  if (priv->hadjustment)
    g_object_unref (priv->hadjustment);
}

static void
bt_list_box_class_init (BtListBoxClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (class);

  object_class->set_property = bt_list_box_set_property;
  object_class->get_property = bt_list_box_get_property;

  widget_class->realize = bt_list_box_realize;
  widget_class->size_allocate = bt_list_box_size_allocate;
  widget_class->draw = bt_list_box_draw;

  container_class->add = dont_call_this;
  container_class->remove = dont_call_this;
  container_class->forall = bt_list_box_forall;
  container_class->child_type = bt_list_box_child_type;

  g_object_class_override_property (object_class, PROP_HADJUSTMENT, "hadjustment");
  g_object_class_override_property (object_class, PROP_VADJUSTMENT, "vadjustment");
  g_object_class_override_property (object_class, PROP_HSCROLL_POLICY, "hscroll-policy");
  g_object_class_override_property (object_class, PROP_VSCROLL_POLICY, "vscroll-policy");
}

static void
bt_list_box_init (BtListBox *list)
{
  /* foobar */
}



GtkWidget *
bt_list_box_new ()
{
  return g_object_new (BT_TYPE_LIST_BOX, NULL);
}


/* Setter/Getter {{{ */
void
bt_list_box_set_widget_fill_func (BtListBox               *list,
                                  BtListBoxWidgetFillFunc  fill_func)
{
  BtListBoxPrivate *priv = bt_list_box_get_instance_private (list);

  priv->fill_func = fill_func;
}
/* }}} */
