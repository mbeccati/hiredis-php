<?php

$r = new Redis();
var_dump($r);

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

