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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include <hiredis/hiredis.h>

#include "php_hiredis.h"

inline zval *getZval(const redisReadTask *task, zval *z)
{
	zval *new;

	if (task && task->parent) {
		new = zend_hash_index_add(Z_ARRVAL_P((zval *)task->parent->obj), task->idx, z);
	} else {
		new = emalloc(sizeof(zval));
		ZVAL_DUP(new, z);
	}

	zval_dtor(z);

	return new;
}

extern void *php_hiredis_createStringObject(const redisReadTask *task, char *str, size_t len)
{
	zval z;
	zend_string *zstr;

	if (!(zstr = zend_string_init(str, len, 0))) {
		return NULL;
	}

	ZVAL_STR(&z, zstr);

	return getZval(task, &z);
}

extern void *php_hiredis_createArrayObject(const redisReadTask *task, int elements)
{
	zval z;

	array_init(&z);

	return getZval(task, &z);
}

extern void *php_hiredis_createIntegerObject(const redisReadTask *task, long long value)
{
	zval z;

	if (value > ZEND_LONG_MAX || value < ZEND_LONG_MIN) {
		ZVAL_DOUBLE(&z, (double)value);
	} else {
		ZVAL_LONG(&z, (zend_long)value);
	}

	return getZval(task, &z);
}

extern void *php_hiredis_createNilObject(const redisReadTask *task)
{
	zval z;

	ZVAL_NULL(&z);

	return getZval(task, &z);
}

extern void php_hiredis_freeReplyObject(void *reply) {
	zval *r = reply;

	zval_dtor(reply);
	efree(reply);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
