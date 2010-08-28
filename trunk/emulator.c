/********************************************************************** 
*
* This file is part of Cardpeek, the smartcard reader utility.
*
* Copyright 2009 by 'L1L1'
*
* Cardpeek is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Cardpeek is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Cardpeek.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include "emulator.h"
#include <stdlib.h>
#include <stdio.h>
#include "misc.h"


anyemul_t* cardemul_new_item(cardemul_t* ce, int type)
{
  emul_t emul;

  if (type==CARDEMUL_COMMAND)
  {
    emul.com = (comemul_t *)malloc(sizeof(comemul_t));
    emul.com -> type = CARDEMUL_COMMAND;
  }
  else
  {
    emul.res = (resemul_t *)malloc(sizeof(resemul_t));
    emul.res -> type = CARDEMUL_RESET;
  }

  if (ce->start.any==NULL)
  {
    emul.any -> next = NULL;
    ce->start = ce->pos = emul;
  }
  else
  {
    emul.any -> next = (ce->pos.any)->next;
    (ce->pos.any)->next= emul.any;
    ce->pos = emul;
  }
  return emul.any;
}

int cardemul_add_command(cardemul_t* ce, const bytestring_t* command, unsigned sw, const bytestring_t* result)
{
  comemul_t* com = (comemul_t *)cardemul_new_item(ce,CARDEMUL_COMMAND);
 
  if (com==NULL)
   return CARDEMUL_ERROR;
 
  com->query    = bytestring_duplicate(command);
  com->sw       = sw;
  com->response = bytestring_duplicate(result);
  ce->count++;
  return CARDEMUL_OK;
}

int cardemul_add_reset(cardemul_t* ce, const bytestring_t* atr)
{
  resemul_t* res = (resemul_t *)cardemul_new_item(ce,CARDEMUL_RESET);

  if (res==NULL)
    return CARDEMUL_ERROR;
  
  res->atr = bytestring_duplicate(atr);
  ce->count++;
  return CARDEMUL_OK;
} 

cardemul_t* cardemul_new()
{
  cardemul_t* ce=(cardemul_t*)malloc(sizeof(cardemul_t));
  ce -> start.any = ce -> pos.any = ce -> atr.any = NULL;
  ce -> count = 0;
  return ce;
}

void cardemul_free(cardemul_t* ce)
{
  emul_t item;
  anyemul_t *next;

  if (ce==NULL)
  {
    log_printf(LOG_WARNING,"cardemul_free(): Attempt to free an NULL pointer.");
    return;
  }

  item = ce -> start;
  
  while (item.any)
  {
    next = item.any -> next;
    if (item.any->type==CARDEMUL_COMMAND)
    {
      bytestring_free(item.com->query);
      bytestring_free(item.com->response);
    }
    else
    {
      bytestring_free(item.res->atr);
    }
    free(item.any);
    item.any = next;
  }
  free(ce);
}

anyemul_t* cardemul_after_atr(cardemul_t* ce)
{
  if (ce->atr.any==NULL)
    return NULL;
  return (ce->atr.any)->next;
}

int cardemul_run_command(cardemul_t* ce, 
			 const bytestring_t* command, 
			 unsigned short *sw, 
			 bytestring_t *response)
{
  anyemul_t* init;
  emul_t cur; 

  bytestring_clear(response);
  *sw=0x6FFF;

  if (ce->start.any==NULL || ce->pos.any==NULL)
    return CARDEMUL_ERROR;

  init    = ce->pos.any;
  cur.any = init;


  do
  {
    if (cur.any->type==CARDEMUL_COMMAND)
    {
      if (bytestring_is_equal(command,cur.com->query))
      {
	bytestring_copy(response,cur.com->response);
	*sw = cur.com->sw;
	if (cur.any->next)
	  ce->pos.any = cur.any->next;
	else
	  ce->pos.any = cardemul_after_atr(ce);
	return CARDEMUL_OK;
      }
      if (cur.any->next)
	cur.any = cur.any->next;
      else
	cur.any = cardemul_after_atr(ce);
    }
    else /* CARDEMUL_RESET */
    {
      cur.any = cardemul_after_atr(ce);
    }

  } while (cur.any!=init && cur.any!=NULL);

  return CARDEMUL_ERROR;
}

int cardemul_run_cold_reset(cardemul_t* ce)
{
  if (ce->start.any==NULL)
    return CARDEMUL_ERROR;

  ce->atr=ce->start; 
  ce->pos.any=(ce->start.any)->next;

  if ((ce->atr.res)->type!=CARDEMUL_RESET)
  {
    log_printf(LOG_ERROR,"cardemul_run_cold_atr(): reset error.");
    return CARDEMUL_ERROR;
  }
  return CARDEMUL_OK;
}

int cardemul_run_warm_reset(cardemul_t* ce)
{
  emul_t cur = ce->pos;

  if (ce->start.any==NULL || cur.any==NULL)
    return CARDEMUL_ERROR;

  if (ce->atr.any==NULL)
    log_printf(LOG_WARNING,"cardemul_run_warm_reset(): no previous cold reset");

  for (;;)
  {
    if (cur.any->type==CARDEMUL_RESET)
    {
      ce->atr=cur;
      ce->pos.any=cur.any->next;
      return CARDEMUL_OK;
    }
    if (cur.any->next)
      cur.any=cur.any->next;
    else if (ce->atr.any)
      cur = ce->atr;
    else
      cur = ce->start;
  }
  return CARDEMUL_OK;
}

int cardemul_run_last_atr(const cardemul_t* ce, bytestring_t *atr)
{
  if (ce->atr.any==NULL)
  {
    bytestring_clear(atr);
    return CARDEMUL_ERROR;
  }
  bytestring_copy(atr,(ce->atr.res)->atr);
  return CARDEMUL_OK;
}

int cardemul_save_to_file(const cardemul_t* ce, const char *filename)
{
  FILE *f;
  emul_t cur;
  char *a;
  char *b;

  if ((f=fopen(filename,"w"))==NULL)
    return CARDEMUL_ERROR;
  fprintf(f,"# cardpeek emulator\n");
  fprintf(f,"# version 0\n");

  for (cur=ce->start;cur.any!=NULL;cur.any=cur.any->next)
  {
    if (cur.any->type==CARDEMUL_COMMAND)
    {
      a = bytestring_to_format("%D",cur.com->query);
      b = bytestring_to_format("%D",cur.com->response);
      fprintf(f,"C:%s:%04X:%s\n",a,cur.com->sw,b);
      free(a);
      free(b);
    }
    else /* CARDTREE_RESET */
    {
      a = bytestring_to_format("%D",cur.res->atr);
      fprintf(f,"R:%s\n",a);
      free(a);
    }
  }
  fprintf(f,"# end\n");
  fclose(f);
  return CARDEMUL_OK;
}

cardemul_t* cardemul_new_from_file(const char *filename)
{
  FILE* f;
  cardemul_t *ce;
  char BUF[1024];
  char *a;
  char *b;
  char *c;
  char *p;
  bytestring_t* query;
  bytestring_t* response;
  bytestring_t* atr;
  unsigned sw;
  int something_to_read;
  int lineno=0;

  if ((f=fopen(filename,"r"))==NULL)
    return NULL;

  ce = cardemul_new();

  query = bytestring_new(8);
  response = bytestring_new(8);
  atr = bytestring_new(8);
  
  while ((something_to_read=(fgets(BUF,1024,f)!=NULL)))
  {
    lineno++;

    if (BUF[0]=='#')
      continue;
    if (BUF[0]=='C' && BUF[1]==':')
    {
      p=a=BUF+2;
      while (*p!=':' && *p) p++;
      if (*p==0)
	break;
      *p=0;
      p=b=p+1;
      while (*p!=':' && *p) p++;
      if (*p==0)
	break;
      *p=0;
      c=p+1;
      bytestring_assign_digit_string(query,a);
      sw = strtol(b,NULL,16);
      bytestring_assign_digit_string(response,c);
      cardemul_add_command(ce,query,sw,response);
    }
    else if (BUF[0]=='R' && BUF[1]==':')
    {
      a=BUF+2;
      bytestring_assign_digit_string(atr,a);
      cardemul_add_reset(ce,atr);
    }
    else if (BUF[0]=='\r' || BUF[0]=='\n')
      continue;
    else
      break;
  }
  bytestring_free(query);
  bytestring_free(response);
  bytestring_free(atr); 

  fclose(f);
  if (something_to_read)
  {
    log_printf(LOG_ERROR,
	 "cardemul_new_from_file(): syntax error on line %i in %s",
	 lineno,filename);
    cardemul_free(ce);
    return NULL;
  }
  return ce;
}

int cardemul_count_records(const cardemul_t* ce)
{
  return ce->count;
}

