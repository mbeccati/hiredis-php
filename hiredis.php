<?php

$r = new Redis();
var_dump($r);

var_dump($r->connect('127.0.0.1'));

var_dump($r->close());

try {
	var_dump($r->get('foo'));
	die("OH NO!\n");
} catch (\Exception $e) {

}

var_dump($r->connect('127.0.0.1'));

var_dump('set/get foo');
var_dump($r->set('foo', 45));
var_dump($r->get('foo'));

var_dump('set/get prefix');
var_dump($r->getOption(Redis::OPT_PREFIX));
var_dump($r->setOption(Redis::OPT_PREFIX, "foo_"));
var_dump($r->getOption(Redis::OPT_PREFIX));

var_dump('set/get foo_var');
var_dump($r->set('var', '12-01-76'));
var_dump($r->get('var'));

var_dump($r->ping());
var_dump($r->echo('redis is here'));

var_dump($r->setnx('var', date('Y-m-d')));
var_dump($r->del('var'));
var_dump($r->setnx('var', date('Y-m-d')));

$r->setex('xxx', 1, 'foo');
sleep(2);
var_dump($r->get('xxx'));

var_dump($r->close());
