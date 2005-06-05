/*!
  \file objectpath.c
  \brief Native CMPIObjectPath implementation.

  This is the native CMPIObjectPath implementation as used for remote
  providers. It reflects the well-defined interface of a regular
  CMPIObjectPath, however, it works independently from the management broker.
  
  It is part of a native broker implementation that simulates CMPI data
  types rather than interacting with the entities in a full-grown CIMOM.

  (C) Copyright IBM Corp. 2003
 
  THIS FILE IS PROVIDED UNDER THE TERMS OF THE COMMON PUBLIC LICENSE 
  ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE 
  CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 
  You can obtain a current copy of the Common Public License from
  http://oss.software.ibm.com/developerworks/opensource/license-cpl.html

  \author Frank Scheffler
  $Revision: 1.1 $
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmcidt.h"
#include "cmcift.h"
#include "cmcimacs.h"
#include "tool.h"
#include "native.h"
#include "utilStringBuffer.h"


extern char *keytype2Chars(CMPIType type);

struct native_cop {
	CMPIObjectPath cop;
	int mem_state;

	char * namespace;
	char * classname;
	struct native_property * keys;
};


static struct native_cop * __new_empty_cop ( int, 
					     const char *,
					     const char *,
					     CMPIStatus * );

/****************************************************************************/


static CMPIStatus __oft_release ( CMPIObjectPath * cop )
{
	struct native_cop * o = (struct native_cop *) cop;

	if ( o->mem_state == TOOL_MM_NO_ADD ) {

		o->mem_state = TOOL_MM_ADD;

		tool_mm_add ( o );
		tool_mm_add ( o->classname );
		tool_mm_add ( o->namespace );

		propertyFT.release ( o->keys );

		CMReturn ( CMPI_RC_OK );
	}

	CMReturn ( CMPI_RC_ERR_FAILED );
}


static CMPIObjectPath * __oft_clone ( CMPIObjectPath * cop, CMPIStatus * rc )
{
	CMPIStatus tmp;
	struct native_cop * o   = (struct native_cop *) cop;
	struct native_cop * new = __new_empty_cop ( TOOL_MM_NO_ADD,
						    o->namespace,
						    o->classname,
						    &tmp );

	if ( tmp.rc == CMPI_RC_OK ) {
		new->keys = propertyFT.clone ( o->keys, rc );

	} else if ( rc ) CMSetStatus ( rc, tmp.rc );

	return (CMPIObjectPath *) new;
}



static CMPIStatus __oft_setNameSpace ( CMPIObjectPath * cop,
				       const char * namespace )
{
	struct native_cop * o = (struct native_cop *) cop;

	char * ns = ( namespace )? strdup ( namespace ): NULL;
  
	if ( o->mem_state == TOOL_MM_NO_ADD ) {
		free ( o->namespace );
	} else {
		tool_mm_add ( ns );
	}

	o->namespace = ns;
	CMReturn ( CMPI_RC_OK );
}


static CMPIString * __oft_getNameSpace ( CMPIObjectPath * cop,
					 CMPIStatus * rc )
{
	struct native_cop * o = (struct native_cop *) cop;

	return native_new_CMPIString ( o->namespace, rc );
}


static CMPIStatus __oft_setHostName ( CMPIObjectPath * cop, const char * hn )
{
	CMReturn ( CMPI_RC_ERR_NOT_SUPPORTED );
}


static CMPIString * __oft_getHostName ( CMPIObjectPath * cop,
					CMPIStatus * rc )
{
	if ( rc ) CMSetStatus ( rc, CMPI_RC_ERR_NOT_SUPPORTED );
	return NULL;
}


static CMPIStatus __oft_setClassName ( CMPIObjectPath * cop,
				       const char * classname )
{
	struct native_cop * o = (struct native_cop *) cop;

	char * cn = ( classname )? strdup ( classname ): NULL;
  
	if ( o->mem_state == TOOL_MM_NO_ADD ) {
		free ( o->classname );
	} else {
		tool_mm_add ( cn );
	}

	o->classname = cn;
	CMReturn ( CMPI_RC_OK );
}


static CMPIString * __oft_getClassName ( CMPIObjectPath * cop,
					 CMPIStatus * rc )
{
	struct native_cop * o = (struct native_cop *) cop;

	return native_new_CMPIString ( o->classname, rc );
}


static CMPIStatus __oft_addKey ( CMPIObjectPath * cop,
				 const char * name,
				 CMPIValue * value,
				 CMPIType type )
{
	struct native_cop * o = (struct native_cop *) cop;

	CMReturn ( ( propertyFT.addProperty ( &o->keys,
					      o->mem_state,
					      name,
					      type,
					      CMPI_keyValue,
					      value ) )?
		   CMPI_RC_ERR_ALREADY_EXISTS: 
		   CMPI_RC_OK );
}


static CMPIData __oft_getKey ( CMPIObjectPath * cop,
			       const char * name,
			       CMPIStatus * rc )
{
	struct native_cop * o = (struct native_cop *) cop;

	return propertyFT.getDataProperty ( o->keys, name, rc );
}


static CMPIData __oft_getKeyAt ( CMPIObjectPath * cop, 
				 unsigned int index,
				 CMPIString ** name,
				 CMPIStatus * rc )
{
	struct native_cop * o = (struct native_cop *) cop;

	return propertyFT.getDataPropertyAt ( o->keys, index, name, rc );
}


static unsigned int __oft_getKeyCount ( CMPIObjectPath * cop, CMPIStatus * rc )
{
	struct native_cop * o = (struct native_cop *) cop;
  
	return propertyFT.getPropertyCount ( o->keys, rc );
}


static CMPIStatus __oft_setNameSpaceFromObjectPath ( CMPIObjectPath * cop,
						     CMPIObjectPath * src )
{
	struct native_cop * s = (struct native_cop *) src;
	return __oft_setNameSpace ( cop, s->namespace );
}

static CMPIString *__oft_toString(CMPIObjectPath * cop, CMPIStatus * rc);

static struct native_cop * __new_empty_cop ( int mm_add,
					     const char * namespace,
					     const char * classname,
					     CMPIStatus * rc )
{
	static CMPIObjectPathFT oft = { 
		NATIVE_FT_VERSION,
		__oft_release,
		__oft_clone,
		__oft_setNameSpace,
		__oft_getNameSpace,
		__oft_setHostName,
		__oft_getHostName,
		__oft_setClassName,
		__oft_getClassName,
		__oft_addKey,
		__oft_getKey,
		__oft_getKeyAt,
		__oft_getKeyCount,
		__oft_setNameSpaceFromObjectPath,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		__oft_toString
	};
        
	static CMPIObjectPath o = {
		"CMPIObjectPath",
		&oft
	};

	struct native_cop * cop =
		(struct native_cop *) 
		tool_mm_alloc ( mm_add, sizeof ( struct native_cop ) );

	cop->cop       = o;
	cop->mem_state = mm_add;
	cop->classname = ( classname )? strdup ( classname ): NULL;
	cop->namespace = ( namespace )? strdup ( namespace ): NULL;

	if ( mm_add == TOOL_MM_ADD ) {
		tool_mm_add ( cop->classname );
		tool_mm_add ( cop->namespace );
	}

	if ( rc ) CMSetStatus ( rc, CMPI_RC_OK );
	return cop;
}


CMPIObjectPath * newCMPIObjectPath ( const char * namespace, 
					     const char * classname,
					     CMPIStatus * rc )
{
	return (CMPIObjectPath *) __new_empty_cop ( TOOL_MM_ADD,
						    namespace,
						    classname,
						    rc );
}

extern char *value2Chars(CMPIType type, CMPIValue * value);


char *pathToChars(CMPIObjectPath * cop, CMPIStatus * rc, char *str, int uri)
{
//            "//atp:9999/root/cimv25:TennisPlayer.first="Patrick",last="Rafter";

   CMPIString *ns;
   CMPIString *cn;
   CMPIString *name;
   CMPIData data;
   unsigned int i, m, s;
   char *v;
   char *colon=":";
   
   *str = 0;
   if (uri) colon="%3A";

   ns = cop->ft->getNameSpace(cop, rc);
   cn = cop->ft->getClassName(cop, rc);
   
   if (ns && ns->hdl && *(char*)ns->hdl) {
      if (uri==0) strcpy(str,(char*)ns->hdl);
      else {
         char *cns=(char*)ns->hdl;
         for (s=i=0, m=strlen(cns); i<m; i++,s++) {
            if (cns[i]=='/') {
               str[s++]='%';
               str[s++]='2';
               str[s]='F';
            }
            else str[s]=cns[i];    
         } 
         str[s]=0;   
      }
      CMRelease(ns);
      strcat(str,colon);
   }   
   
   strcat(str, (char *) cn->hdl);
   CMRelease(cn);
   
   for (i = 0, m = cop->ft->getKeyCount(cop, rc); i < m; i++) {
      data = cop->ft->getKeyAt(cop, i, &name, rc);
      if (i) strcat(str, ",");
      else strcat(str, ".");
      strcat(str, (char *) name->hdl);
      strcat(str, "=");
      v = value2Chars(data.type, &data.value);
      strcat(str, v);
      free(v);
      CMRelease(name);
   };
 
   return str;
}

static CMPIString *__oft_toString(CMPIObjectPath * cop, CMPIStatus * rc)
{
   char str[4096] = { 0 };
   pathToChars(cop, rc, str,0);
   return native_new_CMPIString(str, rc);
}

const char *getNameSpaceChars(CMPIObjectPath * cop)
{
	struct native_cop * o = (struct native_cop *) cop;
   return o->namespace;
}

UtilList *getNameSpaceComponents(CMPIObjectPath * cop)
{
   UtilList *ul=newList();
   CMPIString *nss=__oft_getNameSpace(cop, NULL);
   int s=0,i,m;
   char nsc[256],*ns;
   
   if (nss && nss->hdl) {
      ns=(char*)nss->hdl;
      if (ns) for (s=i=0, m=strlen(ns); i<m; i++,s++) {
         if (ns[i]=='/') {
            nsc[s]=0;
            ul->ft->append(ul,strdup(nsc));
            s=-1;
         }
         else nsc[s]=ns[i];    
      } 
      nsc[s]=0;
   }   
   
   if (s) ul->ft->append(ul,strdup(nsc));
   CMRelease(nss);
   return ul;
}

void pathToXml(UtilStringBuffer *sb, CMPIObjectPath *cop)
{
   int i,s,m,state=0;
   CMPIData data;
   CMPIString *name;

   for (i=0,s=__oft_getKeyCount(cop,NULL); i<s; i++) {
      data=__oft_getKeyAt(cop,i,&name,NULL);
      switch (state) {
      case 0:
         sb->ft->append3Chars(sb,"<KEYBINDING NAME=\"",(char*)name->hdl,"\">");
         if (data.type==CMPI_ref) {
            CMPIObjectPath *ref=data.value.ref;           
            sb->ft->appendChars(sb, "<VALUE.REFERENCE><INSTANCEPATH>");
            
            CMPIString *hn=__oft_getHostName(ref, NULL);;
            sb->ft->append3Chars(sb,"<NAMESPACEPATH><HOST>",(char*)hn->hdl,"</HOST>");
            CMRelease(hn);
            
            sb->ft->appendChars(sb, "<VALUE.REFERENCE><INSTANCEPATH><LOCALNAMESPACEPATH>");
            CMPIString *nss=__oft_getNameSpace(ref, NULL);
            if (nss && nss->hdl) {
               char nsc[256];
               char *ns=(char*)nss->hdl;
               if (ns) for (s=i=0, m=strlen(ns); i<m; i++,s++) {
                  if (ns[i]=='/') {
                     nsc[s]=0;
                     sb->ft->append3Chars(sb,"<NAMESPACE NAME=\"",nsc,"\"></NAMESPACE>");
                     s=0;
                  }
                  else nsc[s]=ns[i];    
              } 
              nsc[s]=0;
              if (s) sb->ft->append3Chars(sb,"<NAMESPACE NAME=\"",nsc,"\"></NAMESPACE>");
              CMRelease(nss);
            }   
            
            sb->ft->appendChars(sb,"</LOCALNAMESPACEPATH></NAMESPACEPATH>");
            
            CMPIString *cn=__oft_getClassName(ref, NULL);
            sb->ft->append3Chars(sb,"<INSTANCENAME CLASSNAME=\"",(char*)cn->hdl,"\">");
            state=1;
            continue;
         }
         else {
            sb->ft->append5Chars(sb,"<KEYVALUE VALUETYPE=\"",keytype2Chars(data.type),"\">",
                value2Chars(data.type,&data.value),"</KEYVALUE>");
            state=0;
         }
         break; 
      case 1:
         if (data.type==CMPI_ref) {
            sb->ft->appendChars(sb,"</INSTANCENAME></INSTANCEPATH></VALUE.REFERENCE>");
            i--;
            state=0;
         }
         else {
            sb->ft->append3Chars(sb,"<KEYBINDING NAME=\"",(char*)name->hdl,"\">");
            sb->ft->append5Chars(sb,"<KEYVALUE VALUETYPE=\"",keytype2Chars(data.type),"\">",
                value2Chars(data.type,&data.value),"</KEYVALUE>");
         }
         break;
      }
      sb->ft->appendChars(sb,"</KEYBINDING>\n");
   }

   if (state==1)
      sb->ft->appendChars(sb,"</INSTANCENAME></INSTANCEPATH></VALUE.REFERENCE></KEYBINDING>");
}

/****************************************************************************/

/*** Local Variables:  ***/
/*** mode: C           ***/
/*** c-basic-offset: 8 ***/
/*** End:              ***/