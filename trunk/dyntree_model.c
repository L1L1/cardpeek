#include "config.h"
#include "dyntree_model.h"
#include "misc.h"
#include "a_string.h"
#include <glib/gstdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#ifndef HAVE_GSTATBUF
#include <unistd.h>
typedef struct stat GStatBuf;
#endif

/*
 * NOTE: Integration of the data model in GTK+ is based on the GtkTreeView
 * tutorial by Tim-Philipp Müller (thanks!).
 *
 */

/* internal stuff */

static void dyntree_model_init (DyntreeModel *pkg_tree);

static void dyntree_model_class_init (DyntreeModelClass *klass);

static void dyntree_model_tree_model_init (GtkTreeModelIface *iface);

static void dyntree_model_finalize (GObject *object);

static GtkTreeModelFlags dyntree_model_get_flags (GtkTreeModel *tree_model);

static GType dyntree_model_get_column_type (GtkTreeModel *tree_model,
        gint c_index);

static gboolean dyntree_model_get_iter (GtkTreeModel *tree_model,
                                        GtkTreeIter *iter,
                                        GtkTreePath *path);

static GtkTreePath *dyntree_model_get_path (GtkTreeModel *tree_model,
        GtkTreeIter *iter);

static void dyntree_model_get_value (GtkTreeModel *tree_model,
                                     GtkTreeIter *iter,
                                     gint column,
                                     GValue *value);

static gboolean dyntree_model_iter_next (GtkTreeModel *tree_model,
        GtkTreeIter *iter);

static gboolean dyntree_model_iter_children (GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        GtkTreeIter *parent);

static gboolean dyntree_model_iter_has_child (GtkTreeModel *tree_model,
        GtkTreeIter *iter);

static gint dyntree_model_iter_n_children (GtkTreeModel *tree_model,
        GtkTreeIter *iter);

static gboolean dyntree_model_iter_nth_child (GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        GtkTreeIter *parent,
        gint n);

static gboolean dyntree_model_iter_parent (GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        GtkTreeIter *child);

static GObjectClass *parent_class = NULL;

static gboolean internal_dyntree_model_iter_remove(DyntreeModel *ctm,
                                                   GtkTreeIter *iter,
				                   gboolean emit_signal);

/* implementation */

GType dyntree_model_get_type (void)
{
    static GType dyntree_model_type = 0;

    if (dyntree_model_type)
        return dyntree_model_type;

    if (1)
    {
        static const GTypeInfo dyntree_model_info =
        {
            sizeof (DyntreeModelClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) dyntree_model_class_init,
            NULL, /* class finalize */
            NULL, /* class_data */
            sizeof (DyntreeModel),
            0, /* n_preallocs */
            (GInstanceInitFunc) dyntree_model_init,
            NULL,
        };
        dyntree_model_type = g_type_register_static (G_TYPE_OBJECT, "DyntreeModel",
                             &dyntree_model_info, (GTypeFlags)0);
    }

    if (1)
    {
        static const GInterfaceInfo tree_model_info =
        {
            (GInterfaceInitFunc) dyntree_model_tree_model_init,
            NULL,
            NULL
        };
        g_type_add_interface_static (dyntree_model_type, GTK_TYPE_TREE_MODEL, &tree_model_info);
    }
    return dyntree_model_type;
}

static void dyntree_model_class_init(DyntreeModelClass *klass)
{
    GObjectClass *object_class;
    parent_class = (GObjectClass*) g_type_class_peek_parent (klass);
    object_class = (GObjectClass*) klass;
    object_class->finalize = dyntree_model_finalize;
}

static void dyntree_model_tree_model_init (GtkTreeModelIface *iface)
{
    iface->get_flags = dyntree_model_get_flags;
    iface->get_n_columns = dyntree_model_get_n_columns;
    iface->get_column_type = dyntree_model_get_column_type;
    iface->get_iter = dyntree_model_get_iter;
    iface->get_path = dyntree_model_get_path;
    iface->get_value = dyntree_model_get_value;
    iface->iter_next = dyntree_model_iter_next;
    iface->iter_children = dyntree_model_iter_children;
    iface->iter_has_child = dyntree_model_iter_has_child;
    iface->iter_n_children = dyntree_model_iter_n_children;
    iface->iter_nth_child = dyntree_model_iter_nth_child;
    iface->iter_parent = dyntree_model_iter_parent;
}

int dyntree_model_column_name_to_index(DyntreeModel *ctm, const char *column_name)
{
    /* the value in the table is c+1 to differentiate the NULL from N=0, so we substract 1 */
    return GPOINTER_TO_INT(g_hash_table_lookup(ctm->columns_by_name,(gpointer)column_name))-1;
}

const char *dyntree_model_column_index_to_name(DyntreeModel *ctm, int c_index)
{
    if (c_index<0 || c_index>=ctm->n_columns)
        return NULL;
    return ctm->columns_by_index[c_index].name;
}

int dyntree_model_column_register(DyntreeModel *ctm,
                                  const char *column_name)
{
    int col;

    if ((col = dyntree_model_column_name_to_index(ctm,column_name))<0)
    {
        col = ctm->n_columns++;
        if (col==ctm->max_columns)
        {
            ctm->max_columns<<=1;
            ctm->columns_by_index = g_realloc(ctm->columns_by_index,
                                              sizeof(DyntreeModelAttributeDescriptor)*ctm->max_columns);
        }
        ctm->columns_by_index[col].name       = g_strdup(column_name);
        /* we add 1 to col so that col==0 is stored as 1 and not NULL */
        g_hash_table_insert(ctm->columns_by_name,(gpointer)ctm->columns_by_index[col].name,GINT_TO_POINTER(col+1));
    }
    return col;
}

static void dyntree_model_init (DyntreeModel *ctm)
{
    ctm->root = NULL;

    ctm->columns_by_name = g_hash_table_new(cstring_hash,cstring_equal);
    g_assert(ctm->columns_by_name != NULL);

    ctm->columns_by_index = g_try_malloc(sizeof(char *)*16);
    g_assert(ctm->columns_by_index != NULL);

    ctm->max_columns = 16;
    ctm->n_columns = 0;
    ctm->stamp = g_random_int(); /* Random int to check whether an iter belongs to our model */
}

static void dyntree_model_finalize (GObject *object)
{
    DyntreeModel *ctm = DYNTREE_MODEL(object);
    int i;

    internal_dyntree_model_iter_remove(ctm,NULL,FALSE);

    for (i=0; i<ctm->n_columns; i++)
    {
        if (ctm->columns_by_index[i].name)
            g_free(ctm->columns_by_index[i].name);
    }
    g_free(ctm->columns_by_index);
    g_hash_table_destroy(ctm->columns_by_name);

    /* must chain up - finalize parent */
    (* parent_class->finalize) (object);
}

static GtkTreeModelFlags dyntree_model_get_flags (GtkTreeModel *tree_model)
{
    g_return_val_if_fail (DYNTREE_IS_MODEL(tree_model), (GtkTreeModelFlags)0);
    return (GTK_TREE_MODEL_ITERS_PERSIST);
}

/*****************************************************************************
*
* dyntree_model_get_n_columns: tells the rest of the world how many data
* columns we export via the tree model interface
*
*****************************************************************************/
gint dyntree_model_get_n_columns (GtkTreeModel *tree_model)
{
    g_return_val_if_fail (DYNTREE_IS_MODEL(tree_model), 0);
    return DYNTREE_MODEL(tree_model)->n_columns;
}
/*****************************************************************************
*
* dyntree_model_get_column_type: tells the rest of the world which type of
* data an exported model column contains
*
*****************************************************************************/
static GType dyntree_model_get_column_type (GtkTreeModel *tree_model,
        gint c_index)
{
    g_return_val_if_fail (DYNTREE_IS_MODEL(tree_model), G_TYPE_INVALID);
    g_return_val_if_fail (c_index < DYNTREE_MODEL(tree_model)->n_columns && c_index >= 0, G_TYPE_INVALID);
    return G_TYPE_STRING;
}

/*****************************************************************************
*
* dyntree_model_get_iter: converts a tree path (physical position) into a
* tree iter structure
*
*****************************************************************************/

static gboolean dyntree_model_get_iter (GtkTreeModel *tree_model,
                                        GtkTreeIter *iter,
                                        GtkTreePath *path)
{
    /* DyntreeModel *dyntree_model; */
    GtkTreeIter parent;
    gint *indices, n, depth;

    g_assert(DYNTREE_IS_MODEL(tree_model));

    g_assert(path!=NULL);

    /* dyntree_model = DYNTREE_MODEL(tree_model); */

    indices = gtk_tree_path_get_indices(path);
    depth = gtk_tree_path_get_depth(path);

    g_assert (depth>0);

    if (dyntree_model_iter_nth_child(tree_model,iter,NULL,indices[0])==FALSE)
        return FALSE;

    for (n=1; n<depth; n++)
    {
        parent = *iter;
        if (dyntree_model_iter_nth_child(tree_model,iter,&parent,indices[n])==FALSE)
            return FALSE;
    }
    return TRUE;
}

/*****************************************************************************
*
* dyntree_model_get_path: converts a tree iter into a tree path.
*
*****************************************************************************/
static GtkTreePath *dyntree_model_get_path (GtkTreeModel *tree_model,
        GtkTreeIter *iter)
{
    GtkTreePath *path;
    DyntreeModelNode *node;

    g_return_val_if_fail (DYNTREE_IS_MODEL(tree_model), NULL);
    g_return_val_if_fail (iter != NULL, NULL);
    g_return_val_if_fail (iter->user_data != NULL, NULL);

    node = (DyntreeModelNode *)iter->user_data;
    path = gtk_tree_path_new();
    while (node)
    {
        gtk_tree_path_prepend_index(path, node->sibling_index);
        node = node -> parent;
    }
    return path;
}
/*****************************************************************************
*
* dyntree_model_get_value: Returns a row's exported data columns
* (_get_value is what gtk_tree_model_get uses)
*
*****************************************************************************/
static void dyntree_model_get_value (GtkTreeModel *tree_model,
                                     GtkTreeIter *iter,
                                     gint column,
                                     GValue *value)
{
    DyntreeModelNode *node;
    /* DyntreeModel *ctm; */

    g_return_if_fail (DYNTREE_IS_MODEL (tree_model));
    g_return_if_fail (iter != NULL);
    g_return_if_fail (column < DYNTREE_MODEL(tree_model)->n_columns);

    /* ctm = DYNTREE_MODEL(tree_model); */

    node = (DyntreeModelNode*) iter->user_data;

    g_return_if_fail ( node != NULL );

    g_value_init (value, G_TYPE_STRING);


    if (column<node->max_attributes && node->attributes[column].value)
        g_value_set_string(value, node->attributes[column].value);
}

/*****************************************************************************
*
* dyntree_model_iter_next: Takes an iter structure and sets it to point
* to the next row.
*
*****************************************************************************/
static gboolean dyntree_model_iter_next (GtkTreeModel *tree_model,
        GtkTreeIter *iter)
{
    DyntreeModelNode *node, *nextnode;
    DyntreeModel *ctm;

    g_return_val_if_fail (DYNTREE_IS_MODEL (tree_model), FALSE);
    g_return_val_if_fail (iter != NULL, FALSE);
    g_return_val_if_fail (iter->user_data != NULL, FALSE);

    ctm = DYNTREE_MODEL(tree_model);

    node = (DyntreeModelNode *) iter->user_data;

    if (node->next == NULL)
        return FALSE;

    nextnode = node->next;

    iter->stamp = ctm->stamp;
    iter->user_data = nextnode;

    return TRUE;
}
/*****************************************************************************
*
* dyntree_model_iter_children: Returns TRUE or FALSE depending on whether
* the row specified by 'parent' has any children.
* If it has children, then 'iter' is set to
* point to the first child. Special case: if
* 'parent' is NULL, then the first top-level
* row should be returned if it exists.
*
*****************************************************************************/
static gboolean dyntree_model_iter_children (GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        GtkTreeIter *parent)
{
    DyntreeModel *ctm;
    DyntreeModelNode *node;

    g_return_val_if_fail (DYNTREE_IS_MODEL (tree_model), FALSE);

    ctm = DYNTREE_MODEL(tree_model);

    if (parent==NULL)
    {
        if (ctm->root == NULL)
            return FALSE;
        iter->user_data = ctm->root;
        iter->stamp = ctm->stamp;
        return TRUE;
    }
    node = (DyntreeModelNode *)parent->user_data;
    if (node->children == NULL)
        return FALSE;
    iter->user_data = node->children;
    iter->stamp = ctm->stamp;
    return TRUE;
}

/*****************************************************************************
*
* dyntree_model_iter_has_child: Returns TRUE or FALSE depending on whether
* the row specified by 'iter' has any children.
*
*****************************************************************************/
static gboolean dyntree_model_iter_has_child (GtkTreeModel *tree_model,
        GtkTreeIter *iter)
{
    DyntreeModelNode *node;

    g_return_val_if_fail (DYNTREE_IS_MODEL (tree_model), FALSE);
    g_return_val_if_fail (iter != NULL, FALSE);

    node = (DyntreeModelNode *)iter->user_data;

    if (node->children == NULL)
        return FALSE;
    return TRUE;
}

/*****************************************************************************
*
* dyntree_model_iter_n_children: Returns the number of children the row
* specified by 'iter' has.
*
*****************************************************************************/

static gint dyntree_model_iter_n_children (GtkTreeModel *tree_model,
        GtkTreeIter *iter)
{
    DyntreeModel *ctm;
    DyntreeModelNode *node;
    gint count_children;

    g_return_val_if_fail (DYNTREE_IS_MODEL (tree_model), -1);

    ctm = DYNTREE_MODEL(tree_model);

    /* if we are at the root of ctm, we can't rely on node->n_children so
       we manually count them */
    if (iter==NULL)
    {
        node = ctm->root;
        count_children = 0;
        while (node)
        {
            node = node->next;
            count_children++;
        }
        return count_children;
    }

    node = (DyntreeModelNode *)iter->user_data;
    if (node==NULL) return 0;
    return node->n_children;
}

/*****************************************************************************
*
* dyntree_model_iter_nth_child: If the row specified by 'parent' has any
* children, set 'iter' to the n-th child and
* return TRUE if it exists, otherwise FALSE.
* A special case is when 'parent' is NULL, in
* which case we need to set 'iter' to the n-th
* row if it exists.
*
*****************************************************************************/
static gboolean dyntree_model_iter_nth_child (GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        GtkTreeIter *parent,
        gint n)
{
    DyntreeModelNode *node;
    DyntreeModel *ctm;

    g_return_val_if_fail (DYNTREE_IS_MODEL (tree_model), FALSE);

    ctm = DYNTREE_MODEL(tree_model);

    if (parent==NULL)
    {
        node = ctm->root;
    }
    else
    {
        node = (DyntreeModelNode *)parent->user_data;
        if (node==NULL) return FALSE;
        node = node->children;
    }

    while (node)
    {
        if (n==0)
        {
            iter->stamp 	= ctm->stamp;
            iter->user_data = node;
            return TRUE;
        }
        node = node -> next;
        n--;
    }
    return FALSE;
}
/*****************************************************************************
*
* dyntree_model_iter_parent: Point 'iter' to the parent node of 'child'. As
* we have a list and thus no children and no
* parents of children, we can just return FALSE.
*
*****************************************************************************/
static gboolean dyntree_model_iter_parent (GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        GtkTreeIter *child)
{
    DyntreeModel *ctm;
    DyntreeModelNode *node;

    g_return_val_if_fail (DYNTREE_IS_MODEL (tree_model), -1);

    ctm = DYNTREE_MODEL(tree_model);

    node = (DyntreeModelNode *)child->user_data;
    if (node->parent==NULL)
        return FALSE;
    iter->stamp = ctm->stamp;
    iter->user_data = node->parent;
    return TRUE;
}
/*****************************************************************************
*
* dyntree_model_new: This is what you use in your own code to create a
* new custom list tree model for you to use.
*
*****************************************************************************/
DyntreeModel *dyntree_model_new (void)
{
    DyntreeModel *newctm;
    newctm = (DyntreeModel *) g_object_new (DYNTREE_MODEL_TYPE, NULL);
    g_assert( newctm != NULL );

    dyntree_model_column_register(newctm,"classname");
    dyntree_model_column_register(newctm,"label");
    dyntree_model_column_register(newctm,"id");
    dyntree_model_column_register(newctm,"size");
    dyntree_model_column_register(newctm,"val");
    dyntree_model_column_register(newctm,"alt");
    dyntree_model_column_register(newctm,"mime-type");
    g_assert(newctm->n_columns == CC_INITIAL_COUNT);

    return newctm;
}

/*****************************************************************************
*
* Implement user level functions
*
*****************************************************************************/

gboolean dyntree_model_iter_attribute_set(DyntreeModel *m,
        GtkTreeIter *iter,
        int c_index,
        const char *str)
{
    int i,pos;
    DyntreeModelNode *node;
    GtkTreePath *path;

    g_return_val_if_fail ( iter!=NULL , FALSE );

    g_return_val_if_fail ( c_index < m->n_columns && c_index >= 0, FALSE);

    node = (DyntreeModelNode *)iter->user_data;

    if (c_index>=node->max_attributes)
    {
        pos = node->max_attributes;
        node->max_attributes = m->max_columns;

        node->attributes = g_realloc(node->attributes,
                                     sizeof(DyntreeModelAttributeValue)*m->max_columns);

        for (i=pos; i<node->max_attributes; i++)
        {
            node->attributes[i].value = NULL;
        }
    }

    if (node->attributes[c_index].value)
        g_free(node->attributes[c_index].value);

    if (str==NULL)
        node->attributes[c_index].value = NULL;
    else
        node->attributes[c_index].value = g_strdup(str);


    path = dyntree_model_get_path(GTK_TREE_MODEL(m), iter);

    gtk_tree_model_row_changed(GTK_TREE_MODEL(m), path, iter);

    gtk_tree_path_free(path);

    return TRUE;
}

gboolean dyntree_model_iter_attribute_set_by_name(DyntreeModel *m,
        GtkTreeIter *iter,
        const char *attr_name,
        const char *str)
{
    int col = dyntree_model_column_register(m,attr_name);
    if (col>=0)
        return dyntree_model_iter_attribute_set(m,iter,col,str);
    return FALSE;
}

static int dyntree_model_iter_n_attributes(DyntreeModel *m, GtkTreeIter *iter)
{
    int attr_index;
    int attr_count = 0;
    DyntreeModelNode *node = iter->user_data;

    for (attr_index=0; attr_index<m->n_columns && attr_index<node->max_attributes; attr_index++)
    {
        if (node->attributes[attr_index].value) attr_count++;
    }
    return attr_count;
}

gboolean dyntree_model_iter_attribute_get(DyntreeModel *m,
        GtkTreeIter *iter,
        int c_index,
        const char **str)
{
    DyntreeModelNode *node;

    g_return_val_if_fail ( iter!=NULL, FALSE);

    g_return_val_if_fail ( str!=NULL , FALSE);

    node = (DyntreeModelNode *)iter->user_data;

    g_return_val_if_fail (c_index < m->n_columns && c_index >= 0, FALSE);

    if (c_index < node->max_attributes)
        *str = node->attributes[c_index].value;
    else
        *str = NULL;

    return TRUE;
}

gboolean dyntree_model_iter_attribute_get_by_name(DyntreeModel *m,
        GtkTreeIter *iter,
        const char *attr_name,
        const char **str)
{
    int col = dyntree_model_column_name_to_index(m,attr_name);
    if (col>=0)
        return dyntree_model_iter_attribute_get(m,iter,col,str);
    return FALSE;
}

static DyntreeModelNode* dyntree_model_node_new(DyntreeModel *ctm)
{
    DyntreeModelNode* node = g_new0(DyntreeModelNode,1);

    UNUSED(ctm);

    node->max_attributes = 8;
    node->attributes = g_new0(DyntreeModelAttributeValue,node->max_attributes);

    return node;
}

static void dyntree_model_node_reindex_from_parent(DyntreeModel *ct, DyntreeModelNode *parent)
{
    int n = 0;
    DyntreeModelNode *cur_node;

    if (parent==NULL)
        cur_node = ct->root;
    else
        cur_node = parent->children;

    while (cur_node)
    {
        cur_node->sibling_index = n++;
        cur_node = cur_node->next;
    }

    if (parent)
        parent->n_children = n;
}

static gboolean internal_dyntree_model_iter_remove(DyntreeModel *ctm,
                                                   GtkTreeIter *iter,
				                   gboolean emit_signal)
{
    DyntreeModelNode *node;
    int i;
    GtkTreeIter other;
    GtkTreePath *path;

    if (iter == NULL)
    {
        while (ctm->root)
        {
            other.user_data = ctm->root;
            internal_dyntree_model_iter_remove(ctm,&other,emit_signal);
        }
        return TRUE;
    }

    g_return_val_if_fail(iter->user_data != NULL, FALSE);

    node = iter->user_data;

    while (node->children)
    {
        other.stamp = ctm->stamp;
        other.user_data = node->children;
        internal_dyntree_model_iter_remove(ctm,&other,emit_signal);
    }

    if (emit_signal)
	    path = dyntree_model_get_path(GTK_TREE_MODEL(ctm),iter);

    if (node->prev==NULL)
    {
        if (node->parent)
            (node->parent)->children = node->next;
        else
            ctm->root = node->next;
    }

    if (node->prev) (node->prev)->next = node->next;

    if (node->next) (node->next)->prev = node->prev;

    dyntree_model_node_reindex_from_parent(ctm,node->parent);

    for (i=0; i<node->max_attributes; i++)
    {
        if (node->attributes[i].value)
            g_free(node->attributes[i].value);
    }
    g_free(node->attributes);

    g_free(node);

    if (emit_signal)
    {
	    gtk_tree_model_row_deleted(GTK_TREE_MODEL(ctm), path);
	    gtk_tree_path_free(path);
    }

    return TRUE;
}

gboolean dyntree_model_iter_remove(DyntreeModel *ctm,
                                   GtkTreeIter *iter)
{
	return internal_dyntree_model_iter_remove(ctm,iter,TRUE);	
}


#define dyntree_model_clear(ctm) dyntree_model_iter_remove(ctm,NULL)

void dyntree_model_iter_append (DyntreeModel *ctm,
                                GtkTreeIter *child,
                                GtkTreeIter *parent)
{
    DyntreeModelNode *pred_node;
    DyntreeModelNode *node;
    DyntreeModelNode *parent_node;
    GtkTreePath *path;

    g_return_if_fail( child!=NULL );

    if (parent==NULL)
    {
        parent_node = NULL;

        if (ctm->root==NULL)
        {
            ctm->root = dyntree_model_node_new(ctm);
            child->stamp = ctm->stamp;
            child->user_data = node = ctm->root;
            child->user_data2 = NULL;
            child->user_data3 = NULL;
            goto row_inserted;
        }
        pred_node = ctm->root;
    }
    else
    {
        parent_node = parent->user_data;

        g_assert(parent_node != NULL);

        if (parent_node->children==NULL)
        {
            parent_node->children = dyntree_model_node_new(ctm);
            (parent_node->children)->parent = parent_node;
            parent_node->n_children = 1;
            child->stamp = ctm->stamp;
            child->user_data = node = parent_node->children;
            child->user_data2 = NULL;
            child->user_data3 = NULL;
            goto row_inserted;
        }
        parent_node->n_children++;
        pred_node = parent_node->children;
    }
    while (pred_node->next)
        pred_node = pred_node->next;

    node = dyntree_model_node_new(ctm);

    pred_node->next = node;
    node->prev = pred_node;

    node->sibling_index = pred_node->sibling_index+1;
    node->parent = parent_node;

    child->stamp = ctm->stamp;
    child->user_data = node;
    child->user_data2 = NULL;
    child->user_data3 = NULL;

row_inserted:

    path = dyntree_model_get_path(GTK_TREE_MODEL(ctm), child);

    gtk_tree_model_row_inserted(GTK_TREE_MODEL(ctm), path, child);

    gtk_tree_path_free(path);
}

static gboolean dyntree_model_iter_attributes_setv(DyntreeModel *ctm,
        GtkTreeIter *iter,
        va_list al)
{
    int column;

    g_assert(DYNTREE_IS_MODEL(ctm));
    g_assert(iter!=NULL);

    while ((column = va_arg(al,int))>=0)
    {
        if (dyntree_model_iter_attribute_set(ctm, iter, column,va_arg(al,char *))==FALSE)
            return FALSE;
    }

    return TRUE;
}

gboolean dyntree_model_iter_attributes_set(DyntreeModel *ctm,
        GtkTreeIter *iter,
        ...)
{
    va_list al;
    gboolean retval;

    va_start(al,iter);
    retval = dyntree_model_iter_attributes_setv(ctm,iter,al);
    va_end(al);

    return retval;
}

static gboolean dyntree_model_iter_attributes_getv(DyntreeModel *ctm,
        GtkTreeIter *iter,
        va_list al)
{
    int column;

    g_assert(DYNTREE_IS_MODEL(ctm));
    g_assert(iter!=NULL);

    while ((column = va_arg(al,int))>=0)
    {
        if (dyntree_model_iter_attribute_get(ctm,iter,column,va_arg(al,const char **))==FALSE)
            return FALSE;
    }
    return TRUE;
}

gboolean dyntree_model_iter_attributes_get(DyntreeModel *ctm,
        GtkTreeIter *iter,
        ...)
{
    va_list al;
    gboolean retval;

    va_start(al,iter);
    retval = dyntree_model_iter_attributes_getv(ctm,iter,al);
    va_end(al);
    return retval;
}

/* XML EXPORT */

static gboolean internal_node_to_xml(a_string_t* res, DyntreeModel *store, GtkTreeIter *iter, int depth)
{
    int i,attr_index;
    GtkTreeIter child;
    gchar* esc_value;
    DyntreeModelNode *node;
    const char *col_name;

    do
    {
        node = iter->user_data;

        g_assert(node != NULL);

        for(i=0; i<depth; i++) a_strcat(res,"  ");

        if (dyntree_model_iter_n_attributes(store,iter)==0
                && dyntree_model_iter_n_children(GTK_TREE_MODEL(store),iter)==0)
            a_strcat(res,"<node />\n");
        else
        {
            a_strcat(res,"<node>\n");

            for (attr_index=0; attr_index<store->n_columns && attr_index<node->max_attributes; attr_index++)
            {
                col_name = dyntree_model_column_index_to_name(store,attr_index);
                if (node->attributes[attr_index].value && col_name[0]!='-')
                {
                    for(i=0; i<=depth; i++) a_strcat(res,"  ");
                    a_strcat(res,"<attr name=\"");
                    a_strcat(res,col_name);
                    switch (node->attributes[attr_index].value[0])
                    {
                    case '8':
                    case '4':
                    case '2':
                        a_strcat(res,"\" encoding=\"bytes\">");
                        a_strcat(res,node->attributes[attr_index].value);
                        break;
                    default:
                        a_strcat(res,"\">");
                        esc_value = g_markup_escape_text(node->attributes[attr_index].value+2,-1);
                        a_strcat(res,esc_value);
                        g_free(esc_value);
                    }
                    a_strcat(res,"</attr>\n");
                }
            }

            if (gtk_tree_model_iter_children(GTK_TREE_MODEL(store),&child,iter))
            {
                internal_node_to_xml(res,store,&child,depth+1);
            }

            for(i=0; i<depth; i++) a_strcat(res,"  ");
            a_strcat(res,"</node>\n");
        }
    }
    while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store),iter));

    return TRUE;
}

char* dyntree_model_iter_to_xml(DyntreeModel* ct, GtkTreeIter *root, gboolean full_xml)
{
    a_string_t *res;
    GtkTreeIter root_copy;
    int initial_depth;

    if (full_xml)
    {
        res = a_strnew("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");

        a_strcat(res,"<cardpeek>\n");
        a_strcat(res,"  <version>0.8</version>\n");
        initial_depth = 1;
    }
    else
    {
        res = a_strnew("");
        initial_depth = 0;
    }

    if (root==NULL)
    {
        if (dyntree_model_iter_children(GTK_TREE_MODEL(ct),&root_copy,NULL)==TRUE)
            internal_node_to_xml(res,ct,&root_copy,initial_depth);
    }
    else
    {
        root_copy = *root;
        internal_node_to_xml(res,ct,&root_copy,initial_depth);
    }

    if (full_xml)
    {
        a_strcat(res,"</cardpeek>");
    }
    return a_strfinalize(res);
}

gboolean dyntree_model_iter_to_xml_file(DyntreeModel* ct, GtkTreeIter *root, const char *fname)
{
    int output;
    char *xml;
    gboolean retval;

    if ((output = g_open(fname, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR))<0)
    {
        log_printf(LOG_ERROR,"Could not open '%s' for output (%s)",fname,strerror(errno));
        return FALSE;
    }
    xml = dyntree_model_iter_to_xml(ct,root,TRUE);

    if (write(output,xml,strlen(xml))<0)
    {
        log_printf(LOG_ERROR,"Output error on '%s' (%s)",fname,strerror(errno));
        retval = FALSE;
    }
    else
        retval = TRUE;
    g_free(xml);
    close(output);
    return retval;
}


/* XML IMPORT */

enum
{
    ND_NONE,
    ND_ROOT,
    ND_NODE,
    ND_ATTR,
    ND_VERSION,
    ND_ADEF
};

const char *ND_STATE[6] =
{
    "file",
    "root",
    "<node>",
    "<attr>",
    "<version>",
    NULL
};


typedef struct
{
    unsigned      ctx_version;
    DyntreeModel *ctx_tree;
    GtkTreeIter   ctx_node;
    int		  ctx_column;
    int           ctx_state;
    int		  ctx_encoded;
} xml_context_data_t;

static void xml_start_element_cb  (GMarkupParseContext *context,
                                   const gchar         *element_name,
                                   const gchar        **attribute_names,
                                   const gchar        **attribute_values,
                                   gpointer             user_data,
                                   GError             **error)
{
    xml_context_data_t *ctx = (xml_context_data_t *)user_data;
    GtkTreeIter child;
    int line_number;
    int char_number;
    int attr_index;
    const char *attr_name;

    g_markup_parse_context_get_position(context,&line_number,&char_number);

    if (g_strcmp0(element_name,"node")==0)
    {
        if (ctx->ctx_state==ND_ROOT)
        {
            if ((ctx->ctx_tree)->root == NULL)
                dyntree_model_iter_append (ctx->ctx_tree, &child, NULL);
            else
                dyntree_model_iter_append (ctx->ctx_tree, &child, &ctx->ctx_node);
        }
        else if (ctx->ctx_state==ND_NODE)
        {
            dyntree_model_iter_append (ctx->ctx_tree, &child, &ctx->ctx_node);
        }
        else
        {
            g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT,
                        "Error on line %i[%i]: unexpected <node> in %s",
                        line_number,char_number,ND_STATE[ctx->ctx_state]);
            return;
        }
        ctx->ctx_node = child;
        ctx->ctx_state=ND_NODE;
    }
    else if (g_strcmp0(element_name,"attr")==0)
    {
        if (ctx->ctx_state!=ND_NODE)
        {
            g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT,
                        "Error on line %i[%i]: unexpected <attr> in %s",
                        line_number,char_number,ND_STATE[ctx->ctx_state]);
            return;
        }

        attr_name = NULL;
        attr_index = 0;
        ctx->ctx_encoded =0;

        while (attribute_names[attr_index])
        {
            if (g_strcmp0("name",attribute_names[attr_index])==0)
            {
                attr_name = attribute_values[attr_index];

                ctx->ctx_column = dyntree_model_column_register(ctx->ctx_tree,attr_name);

                if (ctx->ctx_column<0)
                {
                    g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT,
                                "Error on line %i[%i]: failed to register attribute '%s'",
                                line_number,char_number,attr_name);
                    return;
                }

            }
            else if (g_strcmp0("encoding",attribute_names[attr_index])==0)
            {
                if (g_strcmp0("bytes",attribute_values[attr_index])==0)
                {
                    ctx->ctx_encoded = 1;
                }
                else
                {
                    g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT,
                                "Error on line %i[%i]: unrecognized encoding '%s' in <attr>",
                                line_number,char_number,attribute_values[attr_index]);
                    return;
                }
            }
            else
            {
                g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT,
                            "Error on line %i[%i]: unrecognized attribute '%s' in <attr>",
                            line_number,char_number,attribute_names[attr_index]);
                return;
            }
            attr_index++;
        }

        ctx->ctx_state=ND_ATTR;
    }
    else if (g_strcmp0(element_name,"cardpeek")==0)
    {
        if (ctx->ctx_state!=ND_NONE)
        {
            g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT,
                        "Error on line %i[%i]: unexpected <cardpeek> in %s",
                        line_number,char_number,ND_STATE[ctx->ctx_state]);
            return;
        }
        ctx->ctx_state=ND_ROOT;

        if (attribute_names[0])
        {
            g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT,
                        "Error on line %i[%i]: unrecognized attribute '%s' in <cardpeek>",
                        line_number,char_number,attribute_names[0]);
            return;
        }
    }
    else if (g_strcmp0(element_name,"version")==0)
    {
        if (ctx->ctx_state!=ND_ROOT)
        {
            g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT,
                        "Error on line %i[%i]: unexpected <version> in %s",
                        line_number,char_number,ND_STATE[ctx->ctx_state]);
            return;
        }
        ctx->ctx_state=ND_VERSION;

    }
    else /* error */
    {
        g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                    "Error on line %i[%i]: unrecognized element <%s> in %s",
                    line_number,char_number,element_name,ND_STATE[ctx->ctx_state]);
    }
}

static void xml_end_element_cb  (GMarkupParseContext *context,
                                 const gchar         *element_type,
                                 gpointer             user_data,
                                 GError             **error)
{
    xml_context_data_t *ctx = (xml_context_data_t *)user_data;
    GtkTreeIter parent;

    UNUSED(element_type);
    UNUSED(context);
    UNUSED(error);

    switch (ctx->ctx_state)
    {
    case ND_NODE:
        if (dyntree_model_iter_parent(GTK_TREE_MODEL(ctx->ctx_tree),&parent,&(ctx->ctx_node))==TRUE)
        {
            ctx->ctx_state=ND_NODE;
            ctx->ctx_node = parent;
        }
        else
        {
            ctx->ctx_state=ND_ROOT;
        }
        break;
    case ND_ATTR:
        ctx->ctx_state=ND_NODE;
        break;
    case ND_ROOT:
        ctx->ctx_state=ND_NONE;
        break;
    case ND_VERSION:
	ctx->ctx_state=ND_ROOT;
	break;
    }
}

static void xml_text_cb  (GMarkupParseContext *context,
                          const gchar         *text,
                          gsize                text_len,
                          gpointer             user_data,
                          GError             **error)
{
    xml_context_data_t *ctx = (xml_context_data_t *)user_data;
    int line_number;
    int char_number;
    unsigned i;
    a_string_t *str;

    g_markup_parse_context_get_position(context,&line_number,&char_number);

    if (ctx->ctx_state==ND_ATTR)
    {
        if (ctx->ctx_encoded || (ctx->ctx_version==0 && text_len>=2 && text[1]==':'))
        {
            if (text_len>=2 && (text[0]=='8' || text[0]=='4' || text[0]==2) && text[1]==':')
            {
                str = a_strnnew(text_len,text);
                dyntree_model_iter_attribute_set(ctx->ctx_tree,&(ctx->ctx_node),ctx->ctx_column,a_strval(str));
                a_strfree(str);
            }
            /* FIXME: ignore other cases ? */
        }
        else
        {
            str = a_strnew("t:");
            a_strncat(str,text_len,text);
            dyntree_model_iter_attribute_set(ctx->ctx_tree,&(ctx->ctx_node),ctx->ctx_column,a_strval(str));
            a_strfree(str);
        }
    }
    else if (ctx->ctx_state==ND_VERSION)
    {
        unsigned version_high = 0;
        unsigned version_low = 0;

        sscanf(text,"%u.%u",&version_high,&version_low);
        ctx->ctx_version = (version_high<<8)+version_low;
    }
    else
    {
        for (i=0; i<text_len; i++)
        {
            if (text[i]!=' ' && text[i]!='\r' && text[i]!='\n' && text[i]!='\t')
            {
                g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                            "Error on line %i[%i]: unexpected character '%c'",line_number,char_number,text[i]);
                break;
            }
        }
    }
}

static void xml_error_cb  (GMarkupParseContext *context,
                           GError              *error,
                           gpointer             user_data)
{
    UNUSED(context);
    UNUSED(user_data);

    log_printf(LOG_ERROR,"XML %s",error->message);
}

static GMarkupParser dyntree_parser =
{
    xml_start_element_cb,
    xml_end_element_cb,
    xml_text_cb,
    NULL,
    xml_error_cb
};


gboolean dyntree_model_iter_from_xml(DyntreeModel *ct, GtkTreeIter *parent, gboolean full_xml, const char *source_text, int source_len)
{
    xml_context_data_t ctx;
    GMarkupParseContext *markup_ctx;
    GError *err = NULL;

    /* dyntree_model_clear(ct); */
    if (source_len<0)
        source_len = strlen(source_text);

    ctx.ctx_tree  = ct;

    if (full_xml)
        ctx.ctx_state = ND_NONE;
    else
        ctx.ctx_state = ND_ROOT;

    if (parent != NULL)
        ctx.ctx_node = *parent;

    markup_ctx = g_markup_parse_context_new(&dyntree_parser,0,&ctx,NULL);
    if (g_markup_parse_context_parse(markup_ctx,source_text,strlen(source_text),&err)==TRUE)
    {
        g_markup_parse_context_end_parse(markup_ctx,&err);
    }

    g_markup_parse_context_free(markup_ctx);

    if (err!=NULL)
    {
        g_error_free(err);
        return FALSE;
    }
    return TRUE;
}

gboolean dyntree_model_iter_from_xml_file(DyntreeModel *ct, GtkTreeIter *iter, const char *fname)
{
    char *buf_val;
    int  buf_len;
    int  rd_len;
    GStatBuf st;
    gboolean retval;
    int input;

    if (g_stat(fname,&st)!=0)
    {
        log_printf(LOG_ERROR,"Could not stat '%s' (%s)",fname,strerror(errno));
        return FALSE;
    }
#ifdef _WIN32
    if ((input=g_open(fname,O_RDONLY | O_BINARY,0))<0)
#else
    if ((input=g_open(fname,O_RDONLY,0))<0)
#endif
    {
        log_printf(LOG_ERROR,"Could not open '%s' for input (%s)",fname,strerror(errno));
        return FALSE;
    }
    buf_len = st.st_size;
    buf_val = g_malloc(buf_len);
    if ((rd_len=read(input,buf_val,buf_len))==buf_len)
    {
        if (dyntree_model_iter_from_xml(ct,iter,TRUE,buf_val,buf_len)==FALSE)
        {
            retval = FALSE;
            dyntree_model_clear(ct);
        }
        else
            retval = TRUE;
    }
    else
    {
        log_printf(LOG_ERROR,"Could not read all data (%i bytes of %i) from %s (%s)",
                   rd_len,buf_len,fname,strerror(errno));
        retval = FALSE;
    }
    g_free(buf_val);
    close(input);
    return retval;
}

/* searching */

static gboolean internal_dyntree_model_iter_match(DyntreeModel *ctm,
        GtkTreeIter *iter,
        int *indices,
        char **str,
        int n_values)
{
    int i;
    const char *candidate;

    if (n_values==0)
        return FALSE;

    for (i=0; i<n_values; i++)
    {
        if (dyntree_model_iter_attribute_get(ctm,iter,indices[i],&candidate)==FALSE)
            return FALSE;
        if (g_strcmp0(candidate,str[i]))
            return FALSE;
    }
    return TRUE;
}


gboolean dyntree_model_iter_find_first(DyntreeModel *ctm,
                                       GtkTreeIter *result,
                                       GtkTreeIter *root,
                                       int *indices,
                                       char **str,
                                       int n_values)
{
    GtkTreeIter def_root;

    if (root==NULL)
    {
        if (dyntree_model_iter_children(GTK_TREE_MODEL(ctm),&def_root,NULL)==FALSE)
            return FALSE; /* empty tree: search fails */
        root = &def_root;
    }

    *result = *root;

    if (internal_dyntree_model_iter_match(ctm,result,indices,str,n_values))
        return TRUE;

    return dyntree_model_iter_find_next(ctm,result,root,indices,str,n_values);
}

gboolean dyntree_model_iter_find_next(DyntreeModel *ctm,
                                      GtkTreeIter *result,
                                      GtkTreeIter *root,
                                      int *indices,
                                      char **str,
                                      int n_values)
{
    GtkTreeIter next;
    GtkTreeIter child;
    GtkTreeIter def_root;

    if (root==NULL)
    {
        if (dyntree_model_iter_children(GTK_TREE_MODEL(ctm),&def_root,NULL)==FALSE)
            return FALSE; /* empty tree: search fails */
        root = &def_root;
    }

    for (;;)
    {
        if (dyntree_model_iter_children(GTK_TREE_MODEL(ctm),&next,result)==FALSE)
        {
            next = *result;
            if (dyntree_model_iter_next(GTK_TREE_MODEL(ctm),&next)==FALSE)
            {
                child = *result;
                do
                {
                    if (dyntree_model_iter_parent(GTK_TREE_MODEL(ctm),&next,&child)==FALSE)
                        return FALSE;
                    if (root->user_data == next.user_data) return FALSE;
                    child = next;
                }
                while (dyntree_model_iter_next(GTK_TREE_MODEL(ctm),&next)==FALSE);
            }
        }

        *result = next;

        if (internal_dyntree_model_iter_match(ctm,result,indices,str,n_values))
            return TRUE;
    }
    return FALSE;
}

gboolean dyntree_model_foreach(DyntreeModel *ctm, GtkTreeIter *root, DyntreeModelFunc func, gpointer user_data)
{
    GtkTreeIter iter;
    GtkTreeIter child;

    if (root!=NULL)
        iter = *root;
    else
    {
        if (dyntree_model_iter_children(GTK_TREE_MODEL(ctm),&iter,NULL)==FALSE)
            return FALSE;
    }

    if (func(ctm,&iter,user_data)==FALSE)
        return FALSE;

    if (dyntree_model_iter_children(GTK_TREE_MODEL(ctm),&child,&iter))
    {
        do
        {
            if (dyntree_model_foreach(ctm,&child,func,user_data)==FALSE)
                return FALSE;
        }
        while (dyntree_model_iter_next(GTK_TREE_MODEL(ctm),&child));
    }
    return TRUE;
}


