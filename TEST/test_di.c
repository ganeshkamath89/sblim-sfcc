/*
 * test_di.c
 *
 * (C) Copyright IBM Corp. 2005
 * (C) Copyright Intel Corp. 2005
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE COMMON PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Common Public License from
 * http://oss.software.ibm.com/developerworks/opensource/license-cpl.html
 *
 * Author:        Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 *  Test for deleteInstance() library API. Note that this test case
 *  requires that the CIM schema 2.10 final and the test instance MOF
 *  be installed in root/iicmv1. For more information see library README.
 */
#include <cmci.h>
#include <native.h>
#include <unistd.h>

#include "show.h"

static char * _HOSTNAME = "bestorga.ibm.com";

int main( int argc, char * argv[] )
{
    CMCIClient *cc;
    CMPIObjectPath * objectpath;
    CMPIInstance * instance;
    CMPIStatus status;
    char hostName[512];

    /* Setup a conncetion to the CIMOM */   
    cc = cmciConnect("localhost", NULL, "5988", "clp", NULL, NULL);
   
    gethostname(hostName,511);
    _HOSTNAME=strdup(hostName);

    /* Test deleteInstance() */
    printf("\n----------------------------------------------------------\n");
    printf("Testing deleteInstance() ...\n");
    objectpath = newCMPIObjectPath("root/cimv2", "CWS_Authorization", NULL);
    CMAddKey(objectpath, "Username", "bestorga", CMPI_chars);
    CMAddKey(objectpath, "Classname", "foobar", CMPI_chars);

    status = cc->ft->deleteInstance(cc, objectpath);

    /* Print the results */
    printf( "deleteInstance() rc=%d, msg=%s\n", 
            status.rc, (status.msg)? (char *)status.msg->hdl : NULL);

    if (instance) CMRelease(instance);
    if (objectpath) CMRelease(objectpath);

    return 0;
}
