/*
 * yajltcl_Init and yajltcl_SafeInit
 *
 * Copyright (C) 2010 FlightAware
 *
 * Freely redistributable under the Berkeley copyright.  See license.terms
 * for details.
 */

#include <tcl.h>
#include "yajltcl.h"

#ifndef  TLC_LINK_WIDE_INT
#include "tclDict.h"
#endif

#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT

EXTERN int yajltcl_Json2dictInit(Tcl_Interp *interp);


/*
 *----------------------------------------------------------------------
 *
 * yajltcl_Init --
 *
 *	Initialize the yajltcl extension.  The string "yajltcl" 
 *      in the function name must match the PACKAGE declaration at the top of
 *	configure.in.
 *
 * Results:
 *	A standard Tcl result
 *
 * Side effects:
 *	One new command "yajl" is added to the Tcl interpreter.
 *
 *----------------------------------------------------------------------
 */

EXTERN int
Yajltcl_Init(Tcl_Interp *interp)
{
    /*
     * This may work with 8.0, but we are using strictly stubs here,
     * which requires 8.1.
     */
    if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
	return TCL_ERROR;
    }

    if (Tcl_PkgRequire(interp, "Tcl", "8.1", 0) == NULL) {
	return TCL_ERROR;
    }

/* Tcl <= 8.4 needs tclDict, package dict */
#ifdef USE_DICT_STUBS
    if (Dict_InitStubs(interp, "8.5", 0) == NULL) {
	return TCL_ERROR;
    }
#else
#error PkgRequire(interp, "dict", "8.5", 0)
#endif

    if (Tcl_PkgProvide(interp, "yajltcl", PACKAGE_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }

    /* Create the yajl command  */
    Tcl_CreateObjCommand(interp, "yajl", (Tcl_ObjCmdProc *) yajltcl_yajlObjCmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    yajltcl_Json2dictInit(interp);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * yajltcl_SafeInit --
 *
 *	Initialize the yajltcl in a safe interpreter.
 *
 *      This should be totally safe.
 *
 * Results:
 *	A standard Tcl result
 *
 * Side effects:
 *	One new command "yajl" is added to the Tcl interpreter.
 *
 *----------------------------------------------------------------------
 */

EXTERN int
Yajltcl_SafeInit(Tcl_Interp *interp)
{
    /*
     * This may work with 8.0, but we are using strictly stubs here,
     * which requires 8.1.
     */
    if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
	return TCL_ERROR;
    }

    if (Tcl_PkgRequire(interp, "Tcl", "8.1", 0) == NULL) {
	return TCL_ERROR;
    }

    if (Tcl_PkgProvide(interp, "yajltcl", PACKAGE_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }

    /* Create the yajl command  */
    Tcl_CreateObjCommand(interp, "yajl", (Tcl_ObjCmdProc *) yajltcl_yajlObjCmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    return TCL_OK;
}

