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

#define HIREDIS_CONTEXT_FROM_OBJECT(intern, zv) \
    { \
        php_hiredis_object *obj = Z_HIREDIS_P(zv); \
        intern = obj->rc; \
        if (!intern) { \
            HIREDIS_EXCEPTION("Invalid or uninitialized Redis object", 0); \
            RETURN_FALSE; \
        } \
    }

#define HIREDIS_EXCEPTION(msg, code) zend_throw_exception(hiredis_exception_ce, msg, code);

#define HIREDIS_COMMAND(reply, format, ...) \
if (!(reply = php_hiredis_command(getThis(), format , ##__VA_ARGS__))) { \
	return; \
}

#define HIREDIS_COMMAND_PREFIX(reply, format, ...) { \
	zend_string *_p = Z_HIREDIS_P(getThis())->prefix; \
	HIREDIS_COMMAND(reply, format, _p->val, _p->len , ##__VA_ARGS__); \
}

#define HIREDIS_RETURN(reply) \
if (reply) { \
    RETURN_ZVAL(reply, 0, 0); \
}

#define HIREDIS_FREE(reply) { \
    zval_dtor(reply); \
    efree(reply); \
}

#define REGISTER_HIREDIS_CLASS_CONST_LONG(class_name, const_name, value) \
zend_declare_class_constant_long(class_name, const_name, sizeof(const_name)-1, (zend_long)value);


extern void *php_hiredis_createStringObject(const redisReadTask *task, char *str, size_t len);
extern void *php_hiredis_createArrayObject(const redisReadTask *task, int elements);
extern void *php_hiredis_createIntegerObject(const redisReadTask *task, long long value);
extern void *php_hiredis_createNilObject(const redisReadTask *task);
extern void php_hiredis_freeReplyObject(void *reply);

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
