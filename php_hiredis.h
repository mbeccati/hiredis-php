/* -*- Mode: C; tab-width: 4 -*- */
/*
+----------------------------------------------------------------------+
| hiredis extension for PHP                                            |
+----------------------------------------------------------------------+
| Copyright (c) 2014 Matteo Beccati                                    |
+----------------------------------------------------------------------+
| This source file is subject to version 3.01 of the PHP license,      |
| that is bundled with this package in the file LICENSE, and is        |
| available through the world-wide-web at the following url:           |
| http://www.php.net/license/3_01.txt                                  |
| If you did not receive a copy of the PHP license and are unable to   |
| obtain it through the world-wide-web, please send a note to          |
| license@php.net so we can mail you a copy immediately.               |
+----------------------------------------------------------------------+
| Author: Matteo Beccati <matteo@beccati.com>                          |
+----------------------------------------------------------------------+
*/

#ifndef PHP_HIREDIS_H
#define PHP_HIREDIS_H

/* Extends zend object */
typedef struct _php_hiredis_object {
	redisContext *rc;
	zend_string *prefix;
	zend_object zo;
} php_hiredis_object;

typedef enum {
	HIREDIS_OPT_PREFIX = 1
} php_hiredis_opt;

static inline php_hiredis_object *php_hiredis_fetch_object(zend_object *obj) {
    return (php_hiredis_object *)((char*)(obj) - XtOffsetOf(php_hiredis_object, zo));
}

#define Z_HIREDIS_P(zv) php_hiredis_fetch_object(Z_OBJ_P((zv)))

#define HIREDIS_EXCEPTION(msg, code) zend_throw_exception(hiredis_exception_ce, msg, code);

#define REGISTER_HIREDIS_CLASS_CONST_LONG(class_name, const_name, value) \
zend_declare_class_constant_long(class_name, const_name, sizeof(const_name)-1, (zend_long)value);


extern zend_module_entry hiredis_module_entry;
#define phpext_hiredis_ptr &hiredis_module_entry

#define PHP_HIREDIS_VERSION "0.1.0" /* Replace with version number for your extension */

PHP_METHOD(Redis, connect);
PHP_METHOD(Redis, get);

#ifdef PHP_WIN32
#	define PHP_HIREDIS_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_HIREDIS_API __attribute__ ((visibility("default")))
#else
#	define PHP_HIREDIS_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

/*
  	Declare any global variables you may need between the BEGIN
	and END macros here:

ZEND_BEGIN_MODULE_GLOBALS(hiredis)
	zend_long  global_value;
	char *global_string;
ZEND_END_MODULE_GLOBALS(hiredis)
*/

/* Always refer to the globals in your function as HIREDIS_G(variable).
   You are encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define HIREDIS_G(v) ZEND_TSRMG(hiredis_globals_id, zend_hiredis_globals *, v)
#ifdef COMPILE_DL_HIREDIS
ZEND_TSRMLS_CACHE_EXTERN;
#endif
#else
#define HIREDIS_G(v) (hiredis_globals.v)
#endif

#endif	/* PHP_HIREDIS_H */

/* __footer_here__ */
