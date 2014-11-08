#ifndef dyntree_model_h
#define dyntree_model_h

#include <gtk/gtk.h>

#define DYNTREE_MODEL_TYPE		(dyntree_model_get_type())
#define DYNTREE_MODEL(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj),DYNTREE_MODEL_TYPE, DyntreeModel))
#define DYNTREE_MODEL_CLASS(klass) 	(G_TYPE_CHECK_CLASS_CAST ((klass), DYNTREE_MODEL, DyntreeModelClass))
#define DYNTREE_IS_MODEL(obj) 		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), DYNTREE_MODEL_TYPE))
#define DYNTREE_IS_MODEL_CLASS(klass) 	(G_TYPE_CHECK_CLASS_TYPE ((klass), DYNTREE_MODEL_TYPE))
#define DYNTREE_MODEL_GET_CLASS(obj) 	(G_TYPE_INSTANCE_GET_CLASS ((obj), DYNTREE_MODEL_TYPE, CustomListClass))

enum {
        CC_CLASSNAME = 0,       /* 0 */
        CC_LABEL,
        CC_ID,
        CC_SIZE,
        CC_VAL,
        CC_ALT,                 /* 5 */
        CC_MIME_TYPE,       	/* 6 */
        CC_INITIAL_COUNT        /* 7 */
};

typedef struct {
	char *name;
} DyntreeModelAttributeDescriptor;

typedef struct {
	char *value;
} DyntreeModelAttributeValue;


typedef struct _DyntreeModelNode {
	struct _DyntreeModelNode *parent;
	struct _DyntreeModelNode *prev;
	struct _DyntreeModelNode *next;
	struct _DyntreeModelNode *children;
	DyntreeModelAttributeValue *attributes;
	int max_attributes;
	int sibling_index;
	int n_children;
} DyntreeModelNode;

typedef struct {
	GObject parent;
	DyntreeModelNode *root;

	/* extras */
	GHashTable *columns_by_name;
	DyntreeModelAttributeDescriptor *columns_by_index;
	gint n_columns;
	gint max_columns;
	gint stamp;
} DyntreeModel;

typedef struct {
	GObjectClass parent_class;
} DyntreeModelClass;

typedef gboolean (* DyntreeModelFunc)(DyntreeModel *, GtkTreeIter *, gpointer);


GType dyntree_model_get_type(void);

/***** USER FUNCS *****/

extern DyntreeModel *CARD_DATA_STORE;

/* Creating our model */

DyntreeModel *dyntree_model_new (void);

/* Adding columns to our model */

int dyntree_model_column_register(DyntreeModel *ctm, 
		const char *column_name);

int dyntree_model_column_name_to_index(DyntreeModel *ctm, 
		const char *column_name);

const char *dyntree_model_column_index_to_name(DyntreeModel *ctm, 
		int index);

gint dyntree_model_get_n_columns (GtkTreeModel *tree_model);

/* Setting attributes */

gboolean dyntree_model_iter_attribute_set(DyntreeModel *m, 
		GtkTreeIter *iter, 
		int index, 
		const char *str);

gboolean dyntree_model_iter_attribute_set_by_name(DyntreeModel *m, 
		GtkTreeIter *iter, 
		const char *attr_name, 
		const char *str);

gboolean dyntree_model_iter_attributes_setvalist(DyntreeModel *ctm,
		GtkTreeIter *iter,
		va_list al);

gboolean dyntree_model_iter_attributes_set(DyntreeModel *ctm,
		GtkTreeIter *iter,
		...);

/* Reading attributes */ 

gboolean dyntree_model_iter_attribute_get(DyntreeModel *m, 
		GtkTreeIter *iter, 
		int index, 
		const char **str);

gboolean dyntree_model_iter_attribute_get_by_name(DyntreeModel *m, 
		GtkTreeIter *iter, 
		const char *attr_name, 
		const char **ptr);

gboolean dyntree_model_iter_attributes_getvalist(DyntreeModel *ctm,
		GtkTreeIter *iter,
		va_list al);

gboolean dyntree_model_iter_attributes_get(DyntreeModel *ctm,
		GtkTreeIter *iter,
		...);

/* Removing a node */

gboolean dyntree_model_iter_remove(DyntreeModel *ctm, 
		GtkTreeIter *iter);

void dyntree_model_iter_clear(DyntreeModel *ctm);

/* Adding a node */

void dyntree_model_iter_append (DyntreeModel *ctm, 
		GtkTreeIter *child,
		GtkTreeIter *parent);

/* Testing if empty */

#define dyntree_model_is_empty(ctm) (ctm->root==NULL)

/* exporting/importing XML */

char* dyntree_model_iter_to_xml(DyntreeModel *ctm, GtkTreeIter *root, gboolean full_xml);
                /* result (char*) must be freed with g_free() */

gboolean dyntree_model_iter_to_xml_file(DyntreeModel* ct, GtkTreeIter *root, const char *fname);

gboolean dyntree_model_iter_from_xml(DyntreeModel *ct, GtkTreeIter *parent, gboolean full_xml, const char *source_val, int source_len);
 
gboolean dyntree_model_iter_from_xml_file(DyntreeModel *ct, GtkTreeIter *parent, const char *fname);

/* exporting to other formats */
char* dyntree_model_iter_to_text(DyntreeModel *ctm, GtkTreeIter *root);

/* searching */

gboolean dyntree_model_iter_find_first(DyntreeModel *ctm,
		GtkTreeIter *result,
		GtkTreeIter *root,
		int *indices,
		char **str,
		int n_values);

gboolean dyntree_model_iter_find_next(DyntreeModel *ctm,
		GtkTreeIter *result,
		GtkTreeIter *root,
		int *indices,
		char **str,
		int n_values);

/* iterating */

gboolean dyntree_model_foreach(DyntreeModel *ctm, GtkTreeIter *root, DyntreeModelFunc func, gpointer user_data);

#endif
