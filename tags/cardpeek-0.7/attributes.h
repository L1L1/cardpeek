#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#define ATTRIBUTE_OK    0
#define ATTRIBUTE_ERROR 1
#define ATTRIBUTE_END   2

typedef struct {
  int attr_name_start;
  int attr_name_stop;
  int attr_value_start;
  int attr_value_stop;
} attr_t;

int attribute_get_first(attr_t *attr, const char* attrlist);

int attribute_get_next(attr_t *attr, const char* attrlist);

int attribute_find(attr_t* attr, const char* attrlist, const char* attrname);

char *attribute_get(const char *attrlist, const char *attrname);

char *attribute_set(const char *attrlist, const char *attrname, const char *attrvalue);

#define attribute_clear(attrlist,attrname) (attribute_set(attrlist,attrname,NULL))

#endif
