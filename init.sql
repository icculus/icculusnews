drop database if exists IcculusNews;
create database IcculusNews;
use IcculusNews;

create table news_items (
    id int unsigned not null auto_increment,
    queueid int unsigned not null default 0,
    ip int unsigned not null default 0,
    deleted boolean  not nulldefault 0,
    approved boolean  not null default 0,
    title varchar(128) not null,
    text mediumtext not null,
    author int unsigned default 0,
    postdate datetime not null,
    primary key (id)
) character set utf8;

create table news_queue_rights (
    uid int unsigned not null default 0,
    qid int unsigned not null default 0,
    rights int unsigned not null default 0
) character set utf8;

create table news_queues (
    id int unsigned not null auto_increment,
    flags int unsigned not null default 0,
    name varchar(64) not null,
    description varchar(128) not null,
    rdffile varchar(128) not null,
    siteurl varchar(128) not null,
    rdfurl varchar(128) not null,
    rdfitemcount int unsigned not null default 0,
    itemarchiveurl varchar(128) not null,
    itemviewurl varchar(128) not null,
    rdfimageurl varchar(128),
    created datetime not null,
    owner int unsigned not null default 0,
    primary key (id)
) character set utf8;

create table news_users (
    id int unsigned not null auto_increment,
    name varchar(32) not null,
    password varchar(32) not null,
    email varchar(64) not null,
    defaultqueue int unsigned not null default 0,
    globalrights int unsigned not null default 0,
    created datetime not null
) character set utf8;

-- end of init.sql ...

