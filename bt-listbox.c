#include "bt-listbox.h"

#include <gtk/gtk.h>


typedef struct _BtListBoxPrivate BtListBoxPrivate;

struct _BtListBoxPrivate
{
  GtkAdjustment *vadjustment;
  GtkAdjustment *hadjustment;
  GdkWindow     *bin_window;

  int            bin_y_diff;
  GPtrArray     *widgets;
  GList         *old_widgets;

  int            model_from;
  int            model_to;

  int           next_visible_item;
  int           prev_visible_item;


  GListModel    *model;


  BtListBoxWidgetFillFunc fill_func;
  BtListBoxFilterFunc     filter_func;
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

/* GObject API {{{ */

static void
bt_list_box_set_vadjustment (BtListBox *list_box, GtkAdjustment *vadjustment)
{
  BtListBoxPrivate *priv = bt_list_box_get_instance_private (list_box);

  priv->vadjustment = vadjustment;
}

static void
bt_list_box_set_hadjustment (BtListBox *list_box, GtkAdjustment *hadjustment)
{
  BtListBoxPrivate *priv = bt_list_box_get_instance_private (list_box);

  priv->hadjustment = hadjustment;
}

static void
bt_list_box_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  BtListBoxPrivate *priv = bt_list_box_get_instance_private ((BtListBox *)object);

  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      bt_list_box_set_hadjustment ((BtListBox *)object, g_value_get_object ((value)));
      break;
    case PROP_VADJUSTMENT:
      bt_list_box_set_vadjustment ((BtListBox *)object, g_value_get_object ((value)));
      break;
    /*case PROP_HSCROLL_POLICY:*/
    /*case PROP_VSCROLL_POLICY:*/
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
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
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
bt_list_box_finalize (GObject *object)
{
  BtListBoxPrivate *priv = bt_list_box_get_instance_private ((BtListBox *)object);

  g_clear_object (&priv->hadjustment);
  g_clear_object (&priv->vadjustment);
}



/* }}} */

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
ensure_visible_widgets (BtListBox *list_box)
{
  BtListBoxPrivate *priv = bt_list_box_get_instance_private (list_box);
}

static void
position_children (BtListBox *list_box)
{
  BtListBoxPrivate *priv = bt_list_box_get_instance_private (list_box);
  int i;
  int len = priv->widgets->len;
  int widget_width = gtk_widget_get_allocated_width ((GtkWidget *)list_box);
  GtkAllocation child_allocation = { 0 };

  child_allocation.x = 0;
  child_allocation.y = 0;
  child_allocation.width = widget_width;


  for (i = 0; i < len; ++i)
    {
      GtkWidget *widget = g_ptr_array_index (priv->widgets, i);
      int minimum, natural;
      gtk_widget_get_preferred_height_for_width (widget, widget_width,
                                                 &minimum,
                                                 &natural);
      child_allocation.height = minimum;

      gtk_widget_size_allocate (widget, &child_allocation);

      child_allocation.y += minimum;
    }
}


static void
update_bin_window (BtListBox *list_box)
{

  BtListBoxPrivate *priv = bt_list_box_get_instance_private (list_box);
}


static void
items_changed_cb (GListModel *model,
                  guint       position,
                  guint       removed,
                  guint       added,
                  gpointer    user_data)
{
  BtListBox *list_box = user_data;

  g_message ("%s: %u, %u, %u", __FUNCTION__, position, removed, added);
}


static void
configure_adjustment (BtListBox *list_box)
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
  return GTK_TYPE_LIST_BOX_ROW;
}

static void
dont_call_this ()
{
  g_error ("Don't call %s", __FUNCTION__);
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
  BtListBox *list_box = (BtListBox *)widget;
  gtk_widget_set_allocation (widget, allocation);
  position_children (list_box);

  if (gtk_widget_get_realized (widget))
    {
      GdkWindow *win = gtk_widget_get_window (widget);
      gdk_window_move_resize (win,
                              allocation->x,
                              allocation->y,
                              allocation->width,
                              allocation->height);
      update_bin_window (list_box);
    }

  configure_adjustment (list_box);
  /*ensure_visible_widgets (list_box);*/
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
bt_list_box_class_init (BtListBoxClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (class);

  object_class->set_property = bt_list_box_set_property;
  object_class->get_property = bt_list_box_get_property;

  widget_class->realize       = bt_list_box_realize;
  widget_class->size_allocate = bt_list_box_size_allocate;
  widget_class->draw          = bt_list_box_draw;

  container_class->add        = dont_call_this;
  container_class->remove     = dont_call_this;
  container_class->forall     = bt_list_box_forall;
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
  GtkStyleContext *ct = gtk_widget_get_style_context ((GtkWidget *)list);


  gtk_style_context_add_class (ct, "list");
}



GtkWidget *
bt_list_box_new ()
{
  return g_object_new (BT_TYPE_LIST_BOX, NULL);
}


/* Setter/Getter {{{ */
void
bt_list_box_set_widget_fill_func (BtListBox               *list_box,
                                  BtListBoxWidgetFillFunc  fill_func)
{
  BtListBoxPrivate *priv = bt_list_box_get_instance_private (list_box);

  priv->fill_func = fill_func;
}

void bt_list_box_set_filter_func (BtListBox           *list_box,
                                  BtListBoxFilterFunc  filter_func)
{
  BtListBoxPrivate *priv = bt_list_box_get_instance_private (list_box);

  priv->filter_func = filter_func;
}


void
bt_list_box_set_model (BtListBox  *list_box,
                       GListModel *model)
{
  BtListBoxPrivate *priv = bt_list_box_get_instance_private (list_box);

  g_assert (priv->model == NULL);

  g_object_ref ((GObject *)model);
  priv->model = model;

  g_signal_connect ((GObject *)model, "items-changed", (GCallback)items_changed_cb, list_box);
}


/* }}} */
