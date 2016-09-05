/*
 *
 * Include file for yajl package
 *
 * Copyright (C) 2010 by FlightAware, All Rights Reserved
 *
 * Freely redistributable under the Berkeley copyright, see license.terms
 * for details.
 */


/* NB - fix the configure script */
#include <yajl/yajl_common.h>
#include <yajl/yajl_gen.h>
#include <yajl/yajl_parse.h>
#include <yajl/yajl_tree.h>

/*TODO 2.x.x HAVE_YAJL_VERSION_H */
#define YAJL_MAJOR 2
#ifndef YAJL_MAJOR
#define YAJL1      1
#endif

extern int
yajltcl_yajlObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objvp[]);

typedef struct yajltcl_clientData
{
    Tcl_Interp *interp;
    yajl_gen    genHandle;
    yajl_handle parseHandle;
    Tcl_DString dString;
#ifdef YAJL1
    yajl_gen_config    genConfig;
    yajl_parser_config parseConfig;
#else
    struct { unsigned beautify       :1;
             unsigned validate_utf8  :1;
	     unsigned escape_solidus :1;
             char    *indentString;
    }                  genConfig;  /**< 2.x.x yajl_gen_config() + mask enum yajl_gen_option */
    struct { unsigned allowComments  :1;
             unsigned checkUTF8 :1; // == !dont_validate_strings
             unsigned dont_validate_strings   :1;
             unsigned allow_trailling_garbage :1;
             unsigned allow_multiple_values   :1;
             unsigned allow_partial_values    :1;
    }                  parseConfig;/**< 2.x.x yajl_config() + enum yajl_option */
    yajl_val    tree;//tree_parse()
#endif
    Tcl_Command cmdToken;
} yajltcl_clientData;

