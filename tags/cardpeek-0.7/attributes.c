#include "attributes.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define ATTR_NAME_LEN(a) ((a)->attr_name_stop - (a)->attr_name_start)
#define ATTR_VALUE_LEN(a) ((a)->attr_value_stop - (a)->attr_value_start)
#define ATTR_LEN(a) ((a)->attr_value_stop - (a)->attr_name_start)

int attribute_get_next(attr_t *attr, const char* attrlist)
{
  int p;

  if (attrlist==NULL) return ATTRIBUTE_ERROR;

  p=attr->attr_name_start=attr->attr_value_stop+1;

  if (attrlist[p]==0) return ATTRIBUTE_END;

  while (attrlist[p] && attrlist[p]!='=') p++;

  if (!attrlist[p]) return ATTRIBUTE_ERROR;

  attr->attr_name_stop   = p;
  attr->attr_value_start = ++p;
  while (attrlist[p] && attrlist[p]!='\n') p++;
  attr->attr_value_stop  = p;
  if (attrlist[p]!='\n')
    return ATTRIBUTE_ERROR;
  return ATTRIBUTE_OK;
}

int attribute_get_first(attr_t *attr, const char* attrlist)
{
  if (attrlist==NULL) return ATTRIBUTE_ERROR;

  attr->attr_value_stop  = -1;
  return attribute_get_next(attr,attrlist);
}

int attribute_find(attr_t* attr, const char* attrlist, const char* attrname)
{
  int r;

  r = attribute_get_first(attr,attrlist);
  while (r==ATTRIBUTE_OK) {
    if (strncasecmp(attrlist+attr->attr_name_start,attrname,ATTR_NAME_LEN(attr))==0)
      return ATTRIBUTE_OK;
    r = attribute_get_next(attr,attrlist);
  }
  return r;
}

char *attribute_get(const char *attrlist, const char *attrname)
{
  attr_t attr;
  char *retval;
  int vlen;
  
  if (attribute_find(&attr,attrlist,attrname)!=ATTRIBUTE_OK)
    return NULL;
  
  vlen = ATTR_VALUE_LEN(&attr);
  retval=malloc(vlen+1);
  memcpy(retval,attrlist+attr.attr_value_start,vlen);
  retval[vlen]=0;
  return retval;
}

char *attribute_set(const char *attrlist, const char *attrname, const char *attrvalue)
{
  attr_t attr;
  char *retval;
  int o_vlen,n_vlen,r;


  r = attribute_find(&attr,attrlist,attrname);
  if (r==ATTRIBUTE_OK)
  {
    if (attrvalue==NULL)
    {
      retval = malloc(strlen(attrlist)-ATTR_LEN(&attr));
      if (attr.attr_name_start)
        memcpy(retval,attrlist,attr.attr_name_start);
      strcpy(retval+attr.attr_name_start,attrlist+attr.attr_value_stop+1);
    }
    else
    {
      o_vlen = ATTR_VALUE_LEN(&attr);
      n_vlen = strlen(attrvalue);
      retval = malloc(strlen(attrlist)-o_vlen+n_vlen);
      memcpy(retval,attrlist,attr.attr_value_start);
      memcpy(retval+attr.attr_value_start,attrvalue,n_vlen);
      strcpy(retval+attr.attr_value_start+n_vlen,attrlist+attr.attr_value_stop);
    }
  }
  else if (r==ATTRIBUTE_END)
  {
    retval = malloc(strlen(attrlist)+4+strlen(attrname)+strlen(attrvalue));
    sprintf(retval,"%s%s=%s\n",attrlist,attrname,attrvalue);
  }
  else
    retval=NULL;

  return retval;
}

#ifdef TEST_ATTRIBUTES_C
void dump(char *s)
{
  int i;
  for (i=0;i<strlen(s);i++)
    fprintf(stderr,"%i",i%10);
  fprintf(stderr,"\n");
  for (i=0;i<strlen(s);i++)
  {
    if (s[i]>' ' && s[i]<127)
      fprintf(stderr,"%c",s[i]);
    else if (s[i]=='\n')
      fprintf(stderr,"|");
    else
      fprintf(stderr,".");	
  }
  fprintf(stderr,"\n");
}

int main()
{
  int r;
  attr_t attr;
  const char* test="NameA=Value A\nNameB=Value BB\nNameC=Value-CCC\n";
  char* res=attributes_set(test,"NameD","Yahiiiiiiiiiio"); 

  dump(res);
  r = attribute_get_first(&attr,test);
  while (r==ATTRIBUTE_OK) {
    fprintf(stderr,"%i %i %i %i\n",
	    attr.attr_name_start,
            attr.attr_name_stop,
            attr.attr_value_start,
            attr.attr_value_stop);
    r = attribute_get_next(&attr,test);
  }
  fprintf(stderr,"NameB='%s'\n",attributes_get(test,"NameB"));
}
#endif
