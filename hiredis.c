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

/* If you declare any globals in php_hiredis.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(hiredis)
*/

/* True global resources - no need for thread safety here */
static int le_hiredis;

/* class entries and custom object handlers */
static zend_class_entry *hiredis_ce;
static zend_class_entry *hiredis_exception_ce;
static zend_object_handlers hiredis_object_handlers;

static zend_object *php_hiredis_object_new(zend_class_entry *class_type) /* {{{ */
{
    php_hiredis_object *intern;

    intern = ecalloc(1, sizeof(php_hiredis_object) + zend_object_properties_size(class_type));
	intern->prefix = STR_EMPTY_ALLOC();

    zend_object_std_init(&intern->zo, class_type);
    object_properties_init(&intern->zo, class_type);
    intern->zo.handlers = &hiredis_object_handlers;

    return &intern->zo;
}
/* }}} */

static void php_hiredis_object_free(zend_object *object) /* {{{ */
{
	php_hiredis_object *intern = php_hiredis_fetch_object(object);

	if (intern->rc) {
		redisFree(intern->rc);
	}

	if (intern->prefix) {
		zend_string_release(intern->prefix);
	}
}
/* }}} */

/* Hiredis reader callbacks */
static redisReplyObjectFunctions php_hiredis_reader_functions = {
    php_hiredis_createStringObject,
    php_hiredis_createArrayObject,
    php_hiredis_createIntegerObject,
    php_hiredis_createNilObject,
    php_hiredis_freeReplyObject
};

static zval *php_hiredis_command(zval *zv, const char *format, ...) /* {{{ */
{
	va_list ap;

	php_hiredis_object *obj = Z_HIREDIS_P(zv);
	zval *reply = NULL;

	if (!obj->rc) {
		HIREDIS_EXCEPTION("Not connected", 0);
		return NULL;
	}

	va_start(ap, format);
	reply = redisvCommand(obj->rc, format, ap);
	va_end(ap);

	if (!reply) {
		char *msg;
		size_t len = spprintf(&msg, 0, "Command error: %s", obj->rc->errstr);

		redisFree(obj->rc);
		obj->rc = NULL;
		HIREDIS_EXCEPTION(msg, 0);
		efree(msg);
	}

	return reply;
}
/* }}} */

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("hiredis.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_hiredis_globals, hiredis_globals)
    STD_PHP_INI_ENTRY("hiredis.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_hiredis_globals, hiredis_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ proto Redis Redis::__construct()
Public constructor */
PHP_METHOD(Redis, __construct)
{
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto bool Redis::connect(string hostname, int port = 6379, double timeout = 0, int retry_interval = 0)
   CONNECT */
PHP_METHOD(Redis, connect)
{
	php_hiredis_object *obj = Z_HIREDIS_P(getThis());
	zend_string *hostname;
	zend_long port = 6379, retry_interval = 0;
	double timeout = 0.0;

	redisContext *c;
	struct timeval tv;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|ldl",
		&hostname, &port, &timeout, &retry_interval) == FAILURE)
	{
		return;
	}

	if (timeout < 0) {
		REDIS_EXCEPTION("Timeout must be >= 0", 0);
		return;
	}

	if (obj->rc) {
		redisFree(obj->rc);
		obj->rc = NULL;
	}

	if (timeout) {
		tv.tv_sec = ceil(timeout);
		tv.tv_usec = 1000000 * (timeout - tv.tv_sec);

		c = redisConnectWithTimeout(hostname->val, port, tv);
	} else {
		c = redisConnect(hostname->val, port);
	}

	if (c == NULL || c->err) {
		if (c) {
			char *msg;
			size_t len = spprintf(&msg, 0, "Connection error: %s", c->errstr);

			redisFree(c);
			HIREDIS_EXCEPTION(msg, 0);
			efree(msg);
		} else {
			HIREDIS_EXCEPTION("Connection error: can't allocate redis context", 0);
		}

		return;
	}

	c->reader->fn = &php_hiredis_reader_functions;

	obj->rc = c;

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool Redis::close()
CLOSE */
PHP_METHOD(Redis, close)
{
	php_hiredis_object *obj = Z_HIREDIS_P(getThis());

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) {
		RETURN_FALSE;
	}

	if (obj->rc) {
		redisFree(obj->rc);
		obj->rc = NULL;

		RETURN_TRUE;
	}

	RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool Redis::ping()
PING */
PHP_METHOD(Redis, ping)
{
	zval *reply;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) {
		RETURN_FALSE;
	}

	HIREDIS_COMMAND(reply, "PING")

	if (reply && Z_TYPE_P(reply) == IS_STRING && zend_string_equals_literal(Z_STR_P(reply), "PONG")) {
		RETVAL_TRUE;
	} else {
		RETVAL_FALSE;
	}

	HIREDIS_FREE(reply);
}
/* }}} */

/* {{{ proto string Redis::echo(string key)
ECHO */
PHP_METHOD(Redis, echo)
{
	zval *reply;
	zend_string *key;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S", &key) == FAILURE) {
		RETURN_FALSE;
	}

	HIREDIS_COMMAND(reply, "ECHO %s", key->val, key->len);

	HIREDIS_RETURN(reply);
}
/* }}} */

/* {{{ proto string Redis::get(string key)
   GET */
PHP_METHOD(Redis, get)
{
	zval *reply;
	zend_string *key;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &key) == FAILURE) {
		return;
	}

	HIREDIS_COMMAND_PREFIX(reply, "GET %b%b", key->val, key->len);

	HIREDIS_RETURN(reply);
}
/* }}} */

/* {{{ proto bool Redis::set(string key, mixed value)
   SET */
PHP_METHOD(Redis, set)
{
	zval *reply;
	zend_string *key, *val;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "SS", &key, &val) == FAILURE) {
		return;
	}

	HIREDIS_COMMAND_PREFIX(reply, "SET %b%b %b", key->val, key->len, val->val, val->len);

	if (reply) {
		HIREDIS_FREE(reply);

		RETURN_TRUE;
	}
}
/* }}} */

/* {{{ proto bool Redis::setex(string key, int expire, mixed value)
   SETEX */
PHP_METHOD(Redis, setex)
{
	zval *reply;
	zend_string *key, *val;
	zend_long expire;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "SlS", &key, &expire, &val) == FAILURE) {
		return;
	}

	HIREDIS_COMMAND_PREFIX(reply, "SETEX %b%b %d %b", key->val, key->len, expire, val->val, val->len);

	if (reply) {
		HIREDIS_FREE(reply);

		RETURN_TRUE;
	}
}
/* }}} */

/* {{{ proto bool Redis::psetex(string key, int expire, mixed value)
   PSETEX */
PHP_METHOD(Redis, psetex)
{
	zval *reply;
	zend_string *key, *val;
	zend_long expire;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "SlS", &key, &expire, &val) == FAILURE) {
		return;
	}

	HIREDIS_COMMAND_PREFIX(reply, "PSETEX %b%b %d %b", key->val, key->len, expire, val->val, val->len);

	if (reply) {
		HIREDIS_FREE(reply);

		RETURN_TRUE;
	}
}
/* }}} */

/* {{{ proto bool Redis::setnx(string key, mixed value)
   SETNX */
PHP_METHOD(Redis, setnx)
{
	zval *reply;
	zend_string *key, *val;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "SS", &key, &val) == FAILURE) {
		return;
	}

	HIREDIS_COMMAND_PREFIX(reply, "SETNX %b%b %b", key->val, key->len, val->val, val->len);

	if (reply) {
		convert_to_boolean(reply);
		HIREDIS_RETURN(reply);
	}
}
/* }}} */



/* {{{ proto string Redis::getOption(int option)
   GETOPTION */
PHP_METHOD(Redis, getOption)
{
    php_hiredis_object *obj = Z_HIREDIS_P(getThis());

	zend_long option;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &option) == FAILURE) {
		return;
	}

	switch (option) {
		case HIREDIS_OPT_PREFIX:
			if (obj->prefix) {
				RETURN_STR(zend_string_copy(obj->prefix));
			}
			break;

		default:
			HIREDIS_EXCEPTION("Unknown option", 0);
			return;
	}
}
/* }}} */

/* {{{ proto bool Redis::setOption(int option, string value)
   SETOPTION */
PHP_METHOD(Redis, setOption)
{
    php_hiredis_object *obj = Z_HIREDIS_P(getThis());

	zend_long option;
	zend_string *val;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lS", &option, &val) == FAILURE) {
		return;
	}

	switch (option) {
		case HIREDIS_OPT_PREFIX:
			if (obj->prefix) {
				zend_string_release(obj->prefix);
			}

			obj->prefix = zend_string_copy(val);
			break;

		default:
			HIREDIS_EXCEPTION("Unknown option", 0);
			return;
	}

	RETURN_TRUE;
}
/* }}} */


/* __function_stubs_here__ */

/* {{{ php_hiredis_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_hiredis_init_globals(zend_hiredis_globals *hiredis_globals)
{
	hiredis_globals->global_value = 0;
	hiredis_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ hiredis_methods[]
 */
const zend_function_entry hiredis_methods[] = {
	PHP_ME(Redis, __construct, NULL, ZEND_ACC_CTOR | ZEND_ACC_PUBLIC)
	PHP_ME(Redis, connect, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Redis, close, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Redis, ping, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Redis, echo, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Redis, get, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Redis, set, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Redis, setex, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Redis, psetex, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Redis, setnx, NULL, ZEND_ACC_PUBLIC)

	PHP_ME(Redis, getOption, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Redis, setOption, NULL, ZEND_ACC_PUBLIC)

	PHP_FE_END	/* Must be the last line  */
};
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(hiredis)
{
	zend_class_entry ce, cee;
	/* If you have INI entries, uncomment these lines
	REGISTER_INI_ENTRIES();
	*/

	memcpy(&hiredis_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    hiredis_object_handlers.offset = XtOffsetOf(php_hiredis_object, zo);
    hiredis_object_handlers.free_obj = php_hiredis_object_free;
    hiredis_object_handlers.clone_obj = NULL;

	INIT_CLASS_ENTRY(ce, "Redis", hiredis_methods);
	ce.create_object = php_hiredis_object_new;
	hiredis_ce = zend_register_internal_class(&ce);

	INIT_CLASS_ENTRY(cee, "RedisException", NULL);
	hiredis_exception_ce = zend_register_internal_class_ex(&cee,
		zend_exception_get_default());

	REGISTER_HIREDIS_CLASS_CONST_LONG(hiredis_ce, "OPT_PREFIX", HIREDIS_OPT_PREFIX);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(hiredis)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(hiredis)
{
#if defined(COMPILE_DL_HIREDIS) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE;
#endif
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(hiredis)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(hiredis)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "hiredis support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ hiredis_module_entry
 */
zend_module_entry hiredis_module_entry = {
	STANDARD_MODULE_HEADER,
	"hiredis",
	NULL,
	PHP_MINIT(hiredis),
	PHP_MSHUTDOWN(hiredis),
	PHP_RINIT(hiredis),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(hiredis),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(hiredis),
	PHP_HIREDIS_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_HIREDIS
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE;
#endif
ZEND_GET_MODULE(hiredis)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
