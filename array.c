/*!
  \file array.c
  \brief Native CMPIArray implementation.

  This is the native CMPIArray implementation as used for remote
  providers. It reflects the well-defined interface of a regular
  CMPIArray object, however, it works independently from the management broker.
  
  It is part of a native broker implementation that simulates CMPI data
  types rather than interacting with the entities in a full-grown CIMOM.

  In contrast to a regular array, there exists an additional increase()
  method, which is only used by the native CMPIResult implementation to
  grow an array stepwise.

  (C) Copyright IBM Corp. 2003
 
  THIS FILE IS PROVIDED UNDER THE TERMS OF THE COMMON PUBLIC LICENSE 
  ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE 
  CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 
  You can obtain a current copy of the Common Public License from
  http://oss.software.ibm.com/developerworks/opensource/license-cpl.html

  \author Frank Scheffler
  $Revision: 1.1 $
*/

#include <stdlib.h>
#include <string.h>
#include "cmcidt.h"
#include "cmcift.h"
#include "cmcimacs.h"
#include "tool.h"
#include "native.h"


struct native_array_item {
  CMPIValueState state;
  CMPIValue      value;
};


struct native_array {
   CMPIArray array;
   int mem_state;

   CMPICount size,max,dynamic;
   CMPIType type;
   struct native_array_item * data;
};


static struct native_array * __new_empty_array ( int, CMPICount,
    CMPIType, CMPIStatus * );


/*****************************************************************************/

static void __make_NULL ( struct native_array * a,
     int from, int to, int release )
{
   for ( ; from <= to; from++ ) {
      a->data[from].state = CMPI_nullValue;

      if ( release ) {
         native_release_CMPIValue ( a->type, &a->data[from].value );
      }
   }
}

void native_array_increase_size(CMPIArray * array, CMPICount increment)
{
   struct native_array *a = (struct native_array *) array;

   if ((a->size+increment)>a->max) {
      if (a->size==0) a->max=8;
      else while ((a->size+increment)>a->max) a->max*=2;
      a->data = (struct native_array_item *)
         tool_mm_realloc(a->data, a->max * sizeof(struct native_array_item));
      memset(&a->data[a->size], 0, sizeof(struct native_array_item) * increment);
   }
   a->size += increment;
}


static CMPIStatus __aft_release ( CMPIArray * array )
{
   struct native_array * a = (struct native_array *) array;

   if ( a->mem_state == TOOL_MM_NO_ADD ) {

      int i = a->size;

      tool_mm_add ( a );
      tool_mm_add ( a->data );

      while ( i-- ) {
         if ( ! ( a->data[i].state & CMPI_nullValue ) ) {
            native_release_CMPIValue ( a->type, &a->data[i].value );
         }
      }

      CMReturn ( CMPI_RC_OK );
   }

   CMReturn ( CMPI_RC_ERR_FAILED );
}


static CMPIArray * __aft_clone ( CMPIArray * array, CMPIStatus * rc )
{
   CMPIStatus tmp;
   struct native_array * a   = (struct native_array *) array;
   struct native_array * new =
      __new_empty_array ( TOOL_MM_NO_ADD, a->size, a->type, &tmp );

   int i = a->size;

   while ( i-- && tmp.rc == CMPI_RC_OK ) {
      new->data[i].state = a->data[i].state;
      if ( ! ( new->data[i].state & CMPI_nullValue ) ) {
         new->data[i].value = native_clone_CMPIValue ( a->type,
             &a->data[i].value, &tmp );
      }
   }

   if ( rc ) CMSetStatus ( rc, tmp.rc );

   return (CMPIArray *) new;
}


static CMPICount __aft_getSize ( CMPIArray * array, CMPIStatus * rc )
{
   struct native_array * a = (struct native_array *) array;

   if ( rc ) CMSetStatus ( rc, CMPI_RC_OK );
   return a->size;
}


static CMPIType __aft_getSimpleType ( CMPIArray * array, CMPIStatus * rc )
{
   struct native_array * a = (struct native_array *) array;

   if ( rc ) CMSetStatus ( rc, CMPI_RC_OK );
   return a->type;
}


static CMPIData __aft_getElementAt ( CMPIArray * array, CMPICount index, CMPIStatus * rc )
{
   struct native_array * a = (struct native_array *) array;

   CMPIData result = { a->type, CMPI_badValue };

   if ( index < a->size ) {
      result.state = a->data[index].state;
      result.value = a->data[index].value;
   }

   if ( rc ) CMSetStatus ( rc, CMPI_RC_OK );
   return result;
}


static CMPIStatus setElementAt ( CMPIArray * array, CMPICount index, CMPIValue * val,
       CMPIType type, int opt )
{
   struct native_array * a = (struct native_array *) array;

   if ( a->dynamic && index==a->size ) {
      native_array_increase_size(array, 1); 
   }
      
   if ( index < a->size ) {
      CMPIValue v;

      if ( type == CMPI_chars && a->type == CMPI_string ) {

         v.string = native_new_CMPIString ( val->chars, NULL );
         type = CMPI_string;
         val  = &v;
      }

      if ( opt || type == a->type ) {

         CMPIStatus rc = {CMPI_RC_OK, NULL};

         a->data[index].state = 0;
         if (opt) a->data[index].value = *val;
         else a->data[index].value =  ( a->mem_state == TOOL_MM_ADD )?  *val:
               native_clone_CMPIValue ( type, val, &rc );
         return rc;
      }

      if ( type == CMPI_null ) {

         if ( ! ( a->data[index].state & CMPI_nullValue ) ) {
            __make_NULL ( a, index, index, a->mem_state == TOOL_MM_NO_ADD );
         }
         CMReturn ( CMPI_RC_OK );
      }
   }
   CMReturn ( CMPI_RC_ERR_FAILED );
}

static CMPIStatus __aft_setElementAt ( CMPIArray * array, CMPICount index, CMPIValue * val,
          CMPIType type )
{ 
   return setElementAt(array,index,val,type,0);
}


static struct native_array * __new_empty_array ( int mm_add, CMPICount size, CMPIType type,
      CMPIStatus * rc )
{
   static CMPIArrayFT aft = {
      NATIVE_FT_VERSION,
      __aft_release,
      __aft_clone,
      __aft_getSize,
      __aft_getSimpleType,
      __aft_getElementAt,
      __aft_setElementAt
   };
   static CMPIArray a = {
      "CMPIArray",
      &aft
   };

   struct native_array * array = (struct native_array *)
      tool_mm_alloc ( mm_add, sizeof ( struct native_array ) );

   array->array     = a;
   array->mem_state = mm_add;

   type        &= ~CMPI_ARRAY;
   array->type  = ( type == CMPI_chars )? CMPI_string: type;
   array->size  = size;
 
   if (array->size==0) {
      array->max=8;
      array->dynamic=1;
   }
   else {
      array->max=array->size;
      array->dynamic=0;
   }    
     
   array->data  = (struct native_array_item *) 
      tool_mm_alloc ( mm_add, array->max * sizeof ( struct native_array_item ) );

   __make_NULL ( array, 0, array->max - 1, 0 );

   if ( rc ) CMSetStatus ( rc, CMPI_RC_OK );
   return array;
}


CMPIArray * native_new_CMPIArray ( CMPICount size, CMPIType type, CMPIStatus * rc )
{
   return (CMPIArray *) __new_empty_array ( TOOL_MM_ADD, size, type, rc );
}

CMPIStatus simpleArrayAdd(CMPIArray * array, CMPIValue * val, CMPIType type)
{
   struct native_array * a = (struct native_array *) array;
   if (a->dynamic) {
      if (a->size==0) a->type=type;
      setElementAt(array,a->size,val,type,1);
   }   
   CMReturn ( CMPI_RC_ERR_FAILED );
} 

/****************************************************************************/

/*** Local Variables:  ***/
/*** mode: C           ***/
/*** c-basic-offset: 8 ***/
/*** End:              ***/