/*
 *
 */

#include <tcl.h>
#include "yajltcl.h"
#include <string.h>

#ifndef CONST84
#define CONST84
#include "tclDict.h"
#endif

#ifndef EVENTCACHE
#define EVENTCACHE 1
#endif
//#undef  EVENTCACHE


/* PARSER STUFF */

//XXX is Tcl_GetObjResult(interp) OK for stream parse?

#ifndef EVENTCACHE
static int
append_result_list (Tcl_Interp *interp, char *type, Tcl_Obj *object) 
{
    Tcl_Obj *resultObj = Tcl_GetObjResult (interp);
    Tcl_ListObjAppendElement (interp, resultObj, Tcl_NewStringObj (type, -1));
    Tcl_ListObjAppendElement (interp, resultObj, object);

    return 1;
}

static int
append_string (Tcl_Interp *interp, char *string)
{
     Tcl_ListObjAppendElement (interp, Tcl_GetObjResult (interp), Tcl_NewStringObj (string, -1));
    return 1;
}
#endif


static CONST84 char *options[] = {
        "array_open",
        "array_close",
	"bool",
	"double",
	"integer",
	"map_close",
	"map_open",
	"null",
	"number",
	"string",
	"map_key",
	"parse",
	"parse_complete",
	"tree",
	"getkey",
	"get",
	"clear",
	"reset",
	"delete",
	"free",
	NULL
    };

enum options {
        OPT_ARRAY_OPEN,
	OPT_ARRAY_CLOSE,
	OPT_BOOL,
	OPT_DOUBLE,
	OPT_INTEGER,
	OPT_MAP_CLOSE,
	OPT_MAP_OPEN,
	OPT_NULL,
	OPT_NUMBER,
	OPT_STRING,
	OPT_MAP_KEY,
/* ^--- eventCache ---^ */
	OPT_PARSE,
	OPT_PARSE_COMPLETE,
	OPT_TREE,//tree_parse()
	OPT_GETKEY,//tree_get()
	OPT_GET, //_tree or _gen?
	OPT_CLEAR,
	OPT_RESET,
	OPT_DELETE,
	OPT_FREE   //unused? noop
    };

/** "array_open" .. "map_key" index ObjType, parse events.
 */
static Tcl_Obj * eventCache[OPT_PARSE] = {NULL};

/** cache, shared values Tcl_Obj: null, false/0, true/1, 2 ... 9?
 */
static Tcl_Obj * valueCache[10+1] = {NULL};

static int
null_callback (void *context)
{
    Tcl_Interp *interp = (Tcl_Interp *)context;

#ifndef EVENTCACHE
    append_string (interp, "null");
#else
    Tcl_ListObjAppendElement (interp, Tcl_GetObjResult (interp), valueCache[10]);
#endif
    return 1;
}

static int
boolean_callback (void *context, int boolean)
{
    Tcl_Interp *interp = (Tcl_Interp *)context;

#ifndef EVENTCACHE
    append_result_list (interp, "boolean", Tcl_NewBooleanObj(boolean));
#else
    Tcl_Obj *resultObj = Tcl_GetObjResult (interp);
    Tcl_ListObjAppendElement (interp, resultObj, eventCache[OPT_BOOL]);
    Tcl_ListObjAppendElement (interp, resultObj, valueCache[!!boolean]);
#endif
    return 1;
}

#ifdef YAJL1
static int
integer_callback (void *context, long integerVal)
#else
static int
integer_callback (void *context, long long integerVal)
#endif
{
    Tcl_Interp *interp = (Tcl_Interp *)context;

#ifndef EVENTCACHE
     append_result_list (interp, "integer", Tcl_NewLongObj(integerVal));
#else
    Tcl_Obj *resultObj = Tcl_GetObjResult (interp);
    Tcl_ListObjAppendElement (interp, resultObj, eventCache[OPT_INTEGER]);
    Tcl_ListObjAppendElement (interp, resultObj, Tcl_NewLongObj(integerVal));//TODO Wide
#endif
    return 1;
}

static int
double_callback (void *context, double doubleVal)
{
    Tcl_Interp *interp = (Tcl_Interp *)context;

#ifndef EVENTCACHE
     append_result_list (interp, "double", Tcl_NewDoubleObj(doubleVal));
#else
    Tcl_Obj *resultObj = Tcl_GetObjResult (interp);
    Tcl_ListObjAppendElement (interp, resultObj, eventCache[OPT_DOUBLE]);
    Tcl_ListObjAppendElement (interp, resultObj, Tcl_NewDoubleObj(doubleVal));
#endif
    return 1;
}

/*
 * @note pokud je number callback, integer/double unused!!!
 */
static int
number_callback (void *context, const char *s, unsigned int l)
{
    Tcl_Interp *interp = (Tcl_Interp *)context;

#ifndef EVENTCACHE
     append_result_list (interp, "number", Tcl_NewStringObj(s, l));
#else
    Tcl_Obj *resultObj = Tcl_GetObjResult (interp);
    Tcl_ListObjAppendElement (interp, resultObj, eventCache[OPT_NUMBER]);
    Tcl_ListObjAppendElement (interp, resultObj, Tcl_NewStringObj(s, l));//>=Tcl8.5!!! Number
#endif
    return 1;
}

static int
string_callback (void *context, const unsigned char *stringVal, unsigned int stringLen)
{
    Tcl_Interp *interp = (Tcl_Interp *)context;

#ifndef EVENTCACHE
     append_result_list (interp, "string", Tcl_NewStringObj((char *)stringVal, stringLen));
#else
    Tcl_Obj *resultObj = Tcl_GetObjResult (interp);
    Tcl_ListObjAppendElement (interp, resultObj, eventCache[OPT_STRING]);
    Tcl_ListObjAppendElement (interp, resultObj, Tcl_NewStringObj((char*)stringVal, stringLen));
#endif
    return 1;
}

static int
map_key_callback (void *context, const unsigned char *stringVal, unsigned int stringLen)
{
    Tcl_Interp *interp = (Tcl_Interp *)context;

#ifndef EVENTCACHE
     append_result_list (interp, "map_key", Tcl_NewStringObj((char *)stringVal, stringLen));
#else
    Tcl_Obj *resultObj = Tcl_GetObjResult (interp);
    Tcl_ListObjAppendElement (interp, resultObj, eventCache[OPT_MAP_KEY]);
    Tcl_ListObjAppendElement (interp, resultObj, Tcl_NewStringObj((char*)stringVal, stringLen));
#endif
    return 1;
}

static int
map_start_callback (void *context)
{
    Tcl_Interp *interp = (Tcl_Interp *)context;

#ifndef EVENTCACHE
     append_string (interp, "map_open");
#else
    Tcl_ListObjAppendElement (interp, Tcl_GetObjResult (interp), eventCache[OPT_MAP_OPEN]);
#endif
    return 1;
}

static int
map_end_callback (void *context)
{
    Tcl_Interp *interp = (Tcl_Interp *)context;

#ifndef EVENTCACHE
     append_string (interp, "map_close");
#else
    Tcl_ListObjAppendElement (interp, Tcl_GetObjResult (interp), eventCache[OPT_MAP_CLOSE]);
#endif
    return 1;
}

static int
array_start_callback (void *context)
{
    Tcl_Interp *interp = (Tcl_Interp *)context;

#ifndef EVENTCACHE
     append_string (interp, "array_open");
#else
    Tcl_ListObjAppendElement (interp, Tcl_GetObjResult (interp), eventCache[OPT_ARRAY_OPEN]);
#endif
    return 1;
}

static int
array_end_callback (void *context)
{
    Tcl_Interp *interp = (Tcl_Interp *)context;

#ifndef EVENTCACHE
     append_string (interp, "array_close");
#else
    Tcl_ListObjAppendElement (interp, Tcl_GetObjResult (interp), eventCache[OPT_ARRAY_CLOSE]);
#endif
    return 1;
}

static yajl_callbacks callbacks = {
    null_callback,
    boolean_callback,
#ifdef YAJL1
    integer_callback,
#else
    integer_callback, /**< 2.x.x: (void *ctx, long long integerVal) */
#endif
    double_callback,
#if 1
    number_callback,  /**< => double/integer unused!!! TODO configurable? Tcl8.5? */
#else
    NULL,
#endif
    string_callback,
    map_start_callback,
    map_key_callback,
    map_end_callback,
    array_start_callback,
    array_end_callback
};


/*
 *--------------------------------------------------------------
 *
 * yajltcl_free_parser -- free the YAJL parser and associated
 *  data.
 *
 * Results:
 *      frees the YAJL parser handle if it exists.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */
void
yajltcl_free_parser (yajltcl_clientData *yajlData)
{
    if (yajlData->parseHandle != NULL) {
	yajl_free (yajlData->parseHandle);
    }
}


/*
 *--------------------------------------------------------------
 *
 * yajltcl_recreate_parser -- create or recreate the YAJL parser 
 * and associated data.
 *
 * Results:
 *      ...frees the YAJL parser handle if it exists.
 *      ...creates a new YAJL parser object.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */
void
yajltcl_recreate_parser (yajltcl_clientData *yajlData)
{
    yajltcl_free_parser (yajlData);

#ifdef YAJL1
    yajlData->parseHandle = yajl_alloc (&callbacks, &yajlData->parseConfig, NULL, yajlData->interp);
#else
    yajlData->parseHandle = yajl_alloc (&callbacks, NULL, yajlData->interp);
//  yajl_config (yajlData->parseHandle, _opt, yajlData->parseConfig.xxx);
#endif
}


/* GENERATOR STUFF */



/*
 *--------------------------------------------------------------
 *
 * yajltcl_print_callback -- callback routine for when YAJL wants to "print"
 * something -- we grab it and append it to a Tcl dynamic string in the
 * yajltcl clientData that we maintain.
 *
 * Results:
 *      yajlData->dString is appended to.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */
void
yajltcl_print_callback (void *context, const char *str, unsigned int len)
{
    yajltcl_clientData *yajlData = (yajltcl_clientData *)context;

    Tcl_DStringAppend (&yajlData->dString, str, len);
}


/*
 *--------------------------------------------------------------
 *
 * yajltcl_free_generator -- free the YAJL generator and associated
 *  data.
 *
 * Results:
 *      frees the YAJL generator handle if it exists.
 *      frees the Tcl Dynamic string we use to build up the JSON.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */
void
yajltcl_free_generator (yajltcl_clientData *yajlData)
{
    if (yajlData->genHandle != NULL) {
	yajl_gen_free (yajlData->genHandle);
    }

    Tcl_DStringFree (&yajlData->dString);
}


/*
 *--------------------------------------------------------------
 *
 * yajltcl_recreate_generator -- create or recreate the YAJL generator 
 * and associated data.
 *
 * Results:
 *      ...frees the YAJL generator handle if it exists.
 *      ...frees the Tcl Dynamic string we use to build up the JSON.
 *      ...creates a new YAJL generator object.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */
void
yajltcl_recreate_generator (yajltcl_clientData *yajlData)
{
    yajltcl_free_generator (yajlData);
#ifdef YAJL1
    yajlData->genHandle = yajl_gen_alloc2 (yajltcl_print_callback, &yajlData->genConfig, NULL, yajlData);
#else
    yajlData->genHandle = yajl_gen_alloc (NULL);
    yajl_gen_config (yajlData->genHandle, yajl_gen_print_callback, yajltcl_print_callback, yajlData);
//  yajl_gen_config (yajlData->genHandle, yajlData->genConfig);
#endif
}


/*
 *--------------------------------------------------------------
 *
 * yajltcl_yajlObjectDelete -- command deletion callback routine.
 *
 * Results:
 *      ...frees the YAJL generator handle if it exists.
 *      ...frees the Tcl Dynamic string we use to build up the JSON.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */
void
yajltcl_yajlObjectDelete (ClientData clientData)
{
    yajltcl_clientData *yajlData = (yajltcl_clientData *)clientData;

    yajltcl_free_generator (yajlData);
}


/*
 *----------------------------------------------------------------------
 *
 * yajltcl_yajlObjectObjCmd --
 *
 *    dispatches the subcommands of a yajl object command
 *
 * Results:
 *    stuff
 *
 *----------------------------------------------------------------------
 */
int
yajltcl_yajlObjectObjCmd(ClientData cData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int         optIndex;
    int         arg;
    yajltcl_clientData *yajlData = (yajltcl_clientData *)cData;
    yajl_gen hand = yajlData->genHandle;
    yajl_gen_status status = yajl_gen_status_ok;
    char *errString = NULL;
    char *string = NULL; int len; //for jsonData

    if (objc < 2) {
        Tcl_WrongNumArgs (interp, 1, objv, "option ?value? ?option ?value?...?");
	return TCL_ERROR;
    }

    if (NULL == eventCache[0]) {
       for (arg = 0; arg < OPT_PARSE; arg++) {
           eventCache[arg] = Tcl_NewStringObj(options[arg], -1);
	   Tcl_GetIndexFromObj (NULL, eventCache[arg], options, "option", TCL_EXACT, &optIndex);
	   //check: arg == optIndex ?
           Tcl_IncrRefCount(eventCache[arg]);//shared
       }
    }

/* integer 0..9, 0 ~ false, 1 ~ true */
    if (NULL == valueCache[0]) {
       for (arg = 0; arg < 10; arg++) {
            Tcl_IncrRefCount(valueCache[arg] = Tcl_NewLongObj(arg));
       }
            Tcl_IncrRefCount(valueCache[arg] = Tcl_NewStringObj("null",4));
    }

    for (arg = 1; arg < objc; arg++) {

	if (Tcl_GetIndexFromObj (interp, objv[arg], options, "option",
	    TCL_EXACT, &optIndex) != TCL_OK) {
	    return TCL_ERROR;
	}
//
//nebo plnit eventCache pomoci objv[arg] tady a ted!? pokud optIndex < OPT_PARSE
// jde to? "map_key" pouziva "string"

	switch ((enum options) optIndex) {
	  case OPT_ARRAY_OPEN: {
	      status = yajl_gen_array_open (hand);
	      break;
	  }

	  case OPT_ARRAY_CLOSE: {
	      status = yajl_gen_array_close (hand);
	      break;
	  }

	  case OPT_MAP_OPEN: {
	      status = yajl_gen_map_open (hand);
	      break;
	  }

	  case OPT_MAP_CLOSE: {
	      status = yajl_gen_map_close (hand);
	      break;
	  }

	  case OPT_NULL: {
	      status = yajl_gen_null (hand);
	      break;
	  }

	  case OPT_GET: {
	      Tcl_DStringResult (interp, &yajlData->dString);
	      Tcl_DStringFree (&yajlData->dString);
	      return TCL_OK;
	  }

	  case OPT_RESET: {
	      yajltcl_recreate_generator (yajlData);
	      yajltcl_recreate_parser (yajlData);
	      return TCL_OK;
	  }

	  case OPT_DELETE: {
	      Tcl_DeleteCommandFromToken(interp, yajlData->cmdToken);
	      return TCL_OK;
	  }

	  case OPT_CLEAR: {
	      yajl_gen_clear (hand);
	      status = yajl_gen_status_ok;
	      Tcl_DStringFree (&yajlData->dString);
	      break;
	  }

	  case OPT_BOOL: {
	      int bool;

	      if (arg + 1 >= objc) {
		Tcl_WrongNumArgs (interp, 1, objv, "bool value");
		return TCL_ERROR;
	      }

	      if (Tcl_GetBooleanFromObj (interp, objv[++arg], &bool) == TCL_ERROR) {
		  return TCL_ERROR;
	      }

	      status = yajl_gen_bool (hand, bool);
	      break;
	  }

	  case OPT_DOUBLE: {
	      double doub;

	      if (arg + 1 >= objc) {
		Tcl_WrongNumArgs (interp, 1, objv, "double value");
		return TCL_ERROR;
	      }

	      if (Tcl_GetDoubleFromObj (interp, objv[++arg], &doub) == TCL_ERROR) {
		  return TCL_ERROR;
	      }

	      status = yajl_gen_double (hand, doub);
	      break;
	  }

	  case OPT_INTEGER: {
	      long lon;

	      if (arg + 1 >= objc) {
		Tcl_WrongNumArgs (interp, 1, objv, "integer value");
		return TCL_ERROR;
	      }

	      if (Tcl_GetLongFromObj (interp, objv[++arg], &lon) == TCL_ERROR) {
		  return TCL_ERROR;
	      }

	      status = yajl_gen_integer (hand, lon);
	      break;
	  }

	  case OPT_NUMBER: {
	      char *number;
	      int   len;

	      if (arg + 1 >= objc) {
		Tcl_WrongNumArgs (interp, 1, objv, "number value");
		return TCL_ERROR;
	      }

	      number = Tcl_GetStringFromObj (objv[++arg], &len);
	      status = yajl_gen_number (hand, number, len);
	      break;
	  }

	  case OPT_MAP_KEY:
	  case OPT_STRING: {

	      if (arg + 1 >= objc) {
		Tcl_WrongNumArgs (interp, 1, objv, "string value");
		return TCL_ERROR;
	      }

	      string = Tcl_GetStringFromObj (objv[++arg], &len);
	      status = yajl_gen_string (hand, (unsigned char *)string, len);
	      break;
	  }

	  case OPT_PARSE: {

	      if (arg + 1 >= objc) {
err_need_json:	Tcl_WrongNumArgs (interp, 2, objv, "jsonText");
		return TCL_ERROR;
	      }

//FIXME binary data ByteArray + encoding
	      string = Tcl_GetStringFromObj (objv[++arg], &len);
	      status = yajl_parse (yajlData->parseHandle, (unsigned char *)string, len);

check_parse_status:
	      if (status != yajl_status_ok
#ifdef YAJL1
	       && status != yajl_status_insufficient_data
#endif
	         ) {
	          unsigned char *str = yajl_get_error (yajlData->parseHandle, 1, (unsigned char *)string, len);
		  Tcl_ResetResult (interp);
		  Tcl_SetObjResult (interp, Tcl_NewStringObj ((char *)str, -1));
		  return TCL_ERROR;
	      }
	      break;
	  }

	  case OPT_PARSE_COMPLETE: {
#ifdef YAJL1
	      yajl_parse_complete (yajlData->parseHandle);
#else
	      status = yajl_complete_parse (yajlData->parseHandle); //FIXME check error!?
  	      goto check_parse_status;
#endif
	      break;
	  }

	  case OPT_TREE: { //XXX
	      char errbuf[1024];
	      if (arg + 1 >= objc) {
		 goto err_need_json;
	      }
//FIXME binary data ByteArray + encoding, XXX zutf8 string!
	      string = Tcl_GetString (objv[++arg]);
	      yajl_tree_free (yajlData->tree);
	      if (NULL == (yajlData->tree = yajl_tree_parse (string, errbuf, sizeof(errbuf)))) {
	         Tcl_SetObjResult(interp, Tcl_NewStringObj(string, -1));
		 return TCL_ERROR;
	      }
  	      goto check_parse_status;
	  }

	  case OPT_GETKEY: { //getkey pathlist ?type?
	      CONST84 char **pathv; int pathc;
	      yajl_val node;

	      if (arg + 1 >= objc) {
                Tcl_WrongNumArgs (interp, 2, objv, "pathlist ?type?");
		return TCL_ERROR;
	      }

	      if (TCL_OK != Tcl_SplitList(interp, Tcl_GetString(objv[++arg]), &pathc, &pathv)) {
	         return TCL_ERROR;
	      }

	      node = yajl_tree_get(yajlData->tree, (const char**)pathv, yajl_t_any);
//printf("node=%p type=%d\n", node, node ? node->type: -1);
	      switch (node->type) {
	      case yajl_t_number://IS_DOUBLE,INTEGER
	           if      (node->u.number.flags & 0x01) {//INT
	           Tcl_SetObjResult(interp, Tcl_NewIntObj(node->u.number.i));
		   }
	           else if (node->u.number.flags & 0x02) {//DOUBLE
	           Tcl_SetObjResult(interp, Tcl_NewDoubleObj(node->u.number.d));
		   }
		   else {
	           Tcl_SetObjResult(interp, Tcl_NewStringObj(node->u.number.r, -1));
		   }
		   break;
	      case yajl_t_string:
	           Tcl_SetObjResult(interp, Tcl_NewStringObj(node->u.string, -1));
		   break;
	      case yajl_t_null:
	      case yajl_t_true:
	      case yajl_t_false:
	           Tcl_SetObjResult(interp, Tcl_NewBooleanObj(node->type == yajl_t_true));
		   break;
	      case yajl_t_array:
	      case yajl_t_object:
	      default:
	        return TCL_ERROR;
	      }
	      break;
	  }

	  case OPT_FREE: { //XXX
	  }

	}

	switch (status) {
	  case yajl_gen_invalid_string: /*2.x.x TODO! */
	  case yajl_gen_status_ok: {
	      break;
	  }

	  case yajl_gen_keys_must_be_strings: {
	      errString = "map key needed but string not called";
	      break;
	  }

	  case yajl_max_depth_exceeded: {
	      errString = "maximum generation depth exceeded";
	      break;
	  }

	  case yajl_gen_in_error_state: {
	      errString = "generator option called while in error state";
	      break;
	  }

	  case yajl_gen_generation_complete: {
	      errString = "generation complete, reset the object before reuse";
	      break;
	  }

	  case yajl_gen_invalid_number: {
	      errString = "invalid floating point value";
	      break;
	  }

	  case yajl_gen_no_buf: {
	      errString = "no internal buffer";
	      break;
	  }
	}

	if (errString != NULL) {
	    char argString[32];

	    Tcl_SetObjResult (interp, Tcl_NewStringObj (errString, -1));
	    Tcl_AddErrorInfo (interp, " while processing argument ");
	    sprintf (argString, "%d", arg);
	    Tcl_AddErrorInfo (interp, argString);
	    Tcl_AddErrorInfo (interp," \"");
	    Tcl_AddErrorInfo (interp, Tcl_GetString (objv[arg]));
	    Tcl_AddErrorInfo (interp, "\"");
	    return TCL_ERROR;
	}
    }

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * yajltcl_yajlObjCmd --
 *
 *	This procedure is invoked to process the "yajl" command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

    /* ARGSUSED */
int
yajltcl_yajlObjCmd(clientData, interp, objc, objv)
    ClientData clientData;		/* registered proc hashtable ptr. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int objc;				/* Number of arguments. */
    Tcl_Obj   *CONST objv[];
{
    yajltcl_clientData *yajlData;
    int                 optIndex;
    int                 suboptIndex;
    int                 i;
    char               *commandName;

    static CONST84 char *options[] = {
        "create",//TODO version|info?
	NULL
    };

    enum options {
        OPT_CREATE
    };

    static CONST84 char *subOptions[] = {
	//2.x.x enum yajl_gen_option + yajl_gen_config()
        "-beautify",     //
        "-indent",
	//2.x.x enum yajl_option + yajl_config()
	"-allowComments",//allow_comments
	"-checkUTF8",    //!dont_validate_strings
	NULL
    };

    enum suboptions {
        SUBOPT_BEAUTIFY,
	SUBOPT_INDENT,
	SUBOPT_ALLOWCOMMENTS,
	SUBOPT_CHECKUTF8
    };

    if (objc < 3 || (objc & 1) == 0) {
        Tcl_WrongNumArgs (interp, 1, objv, "create name ?-beautify 0|1? ?-indent string?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj (interp, objv[1], options, "option",
	TCL_EXACT, &optIndex) != TCL_OK) {
	return TCL_ERROR;
    }

    yajlData = (yajltcl_clientData *)ckalloc (sizeof (yajltcl_clientData));

    yajlData->genConfig.beautify = 0;
    yajlData->genConfig.indentString = "\t";

    yajlData->parseConfig.checkUTF8 = 0;
    yajlData->parseConfig.allowComments = 0;

    yajlData->interp = interp;
    yajlData->genHandle = NULL;
    yajlData->parseHandle = NULL;
#ifndef YAJL1
    yajlData->tree = NULL;
#endif
    Tcl_DStringInit (&yajlData->dString);

    for (i = 3; i < objc; i += 2) {
        if (Tcl_GetIndexFromObj (interp, objv[i], subOptions, "suboption",
	    TCL_EXACT, &suboptIndex) != TCL_OK) {
	    return TCL_ERROR;
	}

	switch ((enum suboptions)suboptIndex ) {
	    case SUBOPT_BEAUTIFY: {
	        int beautify;

	        if (Tcl_GetBooleanFromObj (interp, objv[i+1], &beautify) == TCL_ERROR) {
		    return TCL_ERROR;
		}
		yajlData->genConfig.beautify = beautify;
	        break;
	    }

	    case SUBOPT_INDENT: {
	        yajlData->genConfig.indentString = Tcl_GetString (objv[i+1]);
	        break;
	    }

	    case SUBOPT_ALLOWCOMMENTS: {
	        int allowComments;

	        if (Tcl_GetBooleanFromObj (interp, objv[i+1], &allowComments) == TCL_ERROR) {
		    return TCL_ERROR;
		}
	        yajlData->parseConfig.allowComments = allowComments;
	        break;
	    }

	    case SUBOPT_CHECKUTF8: {
	        int checkUTF8;

	        if (Tcl_GetBooleanFromObj (interp, objv[i+1], &checkUTF8) == TCL_ERROR) {
		    return TCL_ERROR;
		}
	        yajlData->parseConfig.checkUTF8 = checkUTF8;
	        break;
	    }
	}
    }

    yajltcl_recreate_generator (yajlData);
    yajltcl_recreate_parser (yajlData);

    commandName = Tcl_GetString (objv[2]);

    // if commandName is #auto, generate a name
    if (strcmp (commandName, "#auto") == 0) {
        static unsigned long nextAutoCounter = 0;
	char *objName;
	int    baseNameLength;

	objName = Tcl_GetStringFromObj (objv[0], &baseNameLength);
	baseNameLength += 42;
	commandName = ckalloc (baseNameLength);
	snprintf (commandName, baseNameLength, "%s%lu", objName, nextAutoCounter++);
    }


    yajlData->cmdToken = Tcl_CreateObjCommand (interp, commandName, yajltcl_yajlObjectObjCmd, yajlData, yajltcl_yajlObjectDelete);
    Tcl_SetObjResult (interp, Tcl_NewStringObj (commandName, -1));
    return TCL_OK;
}

/* JSON2DICT stuff */

const char *nasrat(const char *lbl) { return lbl; }

/** JSON2Dict callbacks context. */
typedef struct StackVal_s {
  Tcl_Obj *value;
  Tcl_Obj *key;
  enum options state; //-1
} StackVal;

typedef struct TableCtx_s {
  Tcl_Interp  *interp;
  yajl_handle  parseHandle; //config?
  StackVal     stack[YAJL_MAX_DEPTH];
  int          level; // 0..MAX-1
//Tcl_CommandToken command;
} TableCtx;

static int null_dict_callback (void *context) {
    TableCtx *ctx = (TableCtx *)context;
    StackVal *stk;
    if (ctx->level < 0) {
	 Tcl_SetObjResult(ctx->interp, valueCache[10]);
	 return 1;
    }
    stk = ctx->stack + ctx->level;
    switch (stk->state) {
    case OPT_ARRAY_OPEN:
	 Tcl_ListObjAppendElement(ctx->interp, stk->value, valueCache[10]);
         break;
    case OPT_MAP_OPEN:
	 Tcl_DictObjPut(ctx->interp, stk->value, stk->key, valueCache[10]);
	 break;
    default:
	 return 0;
    }
    return 1;
}

static int boolean_dict_callback (void *context, int boolVal) {
    TableCtx *ctx = (TableCtx *)context;
    StackVal *stk;
    if (ctx->level < 0) {
	 Tcl_SetObjResult(ctx->interp, valueCache[!!boolVal]);
	 return 1;
    }
    stk = ctx->stack + ctx->level;
    switch (stk->state) {
    case OPT_ARRAY_OPEN:
	 Tcl_ListObjAppendElement(ctx->interp, stk->value, valueCache[!!boolVal]);
         break;
    case OPT_MAP_OPEN:
	 Tcl_DictObjPut(ctx->interp, stk->value, stk->key, valueCache[!!boolVal]);
	 break;
    default:
	 return 0;//memleak?
    }
    return 1;
}

static int string_dict_callback (void *context, const unsigned char *bytes, unsigned int length) {
    TableCtx *ctx = (TableCtx *)context;
    StackVal *stk;
    Tcl_Obj *stringObj = Tcl_NewStringObj((char *)bytes, length);
    if (ctx->level < 0) {
	 Tcl_SetObjResult(ctx->interp, stringObj);
	 return 1;
    }
    stk = ctx->stack + ctx->level;
    switch (stk->state) {
    case OPT_ARRAY_OPEN:
	 Tcl_ListObjAppendElement(ctx->interp, stk->value, stringObj);
         break;
    case OPT_MAP_OPEN:
	 Tcl_DictObjPut(ctx->interp, stk->value, stk->key, stringObj);
	 break;
    default:
	 Tcl_DecrRefCount(stringObj);
	 return 0;//TableCtx cleanup? memleaks?
    }
    return 1;
}

/* .state to MAP_OPEN, save .key
 */
static int map_key_dict_callback (void *context, const unsigned char *bytes, unsigned int length) {
    TableCtx *ctx = (TableCtx *)context;
    ctx->stack[ctx->level].key = Tcl_NewStringObj((char *)bytes, length);//cache/share keys?
    return 1;
}

/** start map/object.
- init/clear???
- check: state==0/INIT?
*/
static int map_start_dict_callback (void *context) {
    TableCtx *ctx = (TableCtx *)context;
    StackVal *stk;
/*re/initializace vyssi level: */
    stk = ctx->stack + ++ctx->level;
    stk->state = OPT_MAP_OPEN;
    stk->value = Tcl_NewDictObj();
    return 1;
}

/** end/close  map/object.
- corrent state in map_open|map_value 
- append current level to level-1, clean, level--
- level==0 => final state!
*/
static int map_end_dict_callback (void *context) {
    TableCtx *ctx = (TableCtx *)context;
    StackVal *stk;
    stk = ctx->stack + ctx->level--;
    if (ctx->level == -1) { //final/DONE set ObjResult()
       Tcl_SetObjResult(ctx->interp, stk->value);
    }
    else {//[level].value pripiseme na [level-1].value
	switch (stk[-1].state) {
	case OPT_ARRAY_OPEN:
	     Tcl_ListObjAppendElement(ctx->interp, stk[-1].value, stk->value);
	     break;
	case OPT_MAP_OPEN:
	     Tcl_DictObjPut(ctx->interp, stk[-1].value, stk[-1].key, stk->value);
	     break;
	default:
	     return 0;//cleanup, memleaks?
	}
    }
    return 1;
}

static int array_start_dict_callback (void *context) {
    TableCtx *ctx = (TableCtx *)context;
    StackVal *ctx2;
/*re/initializace vyssi level: */
    ctx2 = ctx->stack + ++ctx->level;
    ctx2->state = OPT_ARRAY_OPEN;
    ctx2->value = Tcl_NewListObj(0,NULL);
    return 1;
}


static yajl_callbacks dict_callbacks = {
    null_dict_callback,
    boolean_dict_callback,
    NULL,//integer_dict_callback,
    NULL,//double_dict_callback,
//  number_dict_callback,  /**< => double/integer unused!!! TODO configurable? Tcl8.5? */
    string_dict_callback,//dtto number?
    string_dict_callback,
    map_start_dict_callback,
    map_key_dict_callback,
    map_end_dict_callback,
    array_start_dict_callback,
    map_end_dict_callback//array_end!
};

/** Parse JSONText info Tcl value, ListObj, DictObj or combined.
 
 @todo TODO yajltcl_ErrorCode()
 @todo C API for Mongrel2 headers?
 */
int
yajltcl_Json2dictObjCmd(
    ClientData clientData,		/* registered proc hashtable ptr. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj   *CONST objv[]
) {
    TableCtx *yajlData;
    char *string ; int len;
    yajl_status status;
    int completed = 0;

    yajlData = (TableCtx *)clientData;

    if (objc != 2) {
	Tcl_WrongNumArgs (interp, 1, objv, "jsonText");
	return TCL_ERROR;
    }

//FIXME binary data ByteArray + encoding
    string = Tcl_GetStringFromObj (objv[1], &len);
    status = yajl_parse (yajlData->parseHandle, (unsigned char *)string, len);

//complete()!!!

check_parse_status:
    if (status != yajl_status_ok
#ifdef YAJL1
     && status != yajl_status_insufficient_data
#endif
       ) {
	unsigned char *str = yajl_get_error (yajlData->parseHandle, 1, (unsigned char *)string, len);
	Tcl_ResetResult (interp);
	Tcl_SetObjResult (interp, Tcl_NewStringObj ((char *)str, -1));
	//free_error()?
	return TCL_ERROR;
    }

//TODO yajltcl_ErrorCode() !!!
    if (!completed) {
         completed=1;
	 status = yajl_complete_parse(yajlData->parseHandle);
	 goto check_parse_status;
    }

    return TCL_OK;
}

/**
 @todo TODO Json2dictDeleteProc
 */
int
yajltcl_Json2dictInit(Tcl_Interp *interp) {
  TableCtx *ctx;

  ctx = malloc(sizeof *ctx);
  ctx->parseHandle = yajl_alloc(&dict_callbacks, NULL , ctx);
  yajl_config(ctx->parseHandle, yajl_allow_multiple_values, 1);//Hmm, reset ::= free+alloc ?
  ctx->level = -1;
  ctx->interp = interp;
  Tcl_CreateObjCommand(interp, "::yajl::json2dict",yajltcl_Json2dictObjCmd,(ClientData)ctx, NULL);
  return TCL_OK;
}
