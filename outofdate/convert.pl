#!/usr/bin/perl

use strict;
use warnings;
use DBI;

# Global rights constants.
use constant canSeeAllQueues           => 1 << 0;  # !!! FIXME: not used.
use constant canSeeAllDeleted          => 1 << 1;
use constant canSeeAllUnapproved       => 1 << 2;
use constant canDeleteAllItems         => 1 << 3;
use constant canPurgeAllItems          => 1 << 4;
use constant canApproveAllItems        => 1 << 5;
use constant canTweakUsers             => 1 << 6;
use constant canCreateQueues           => 1 << 7;
use constant canDeleteUsers            => 1 << 8;
use constant canDeleteQueues           => 1 << 9;
use constant canChangeOthersPasswords  => 1 << 10;
use constant canEditAllItems           => 1 << 11;
use constant canNotAuthorize           => 1 << 12;

# Queue rights constants.
use constant canSee           => 1 << 0; # !!! FIXME: not used.
use constant canSeeDeleted    => 1 << 1;
use constant canSeeUnapproved => 1 << 2;
use constant canDeleteItems   => 1 << 3;
use constant canPurgeItems    => 1 << 4;
use constant canApproveItems  => 1 << 5;
use constant canEditItems     => 1 << 5;

# Queue flags constants.
use constant queueInvisible => 1 << 0;  # !!! FIXME: not used.

# syslog constants.
use constant syslogNone      => 0;
use constant syslogError     => 1;
use constant syslogDaemon    => 2;
use constant syslogAuth      => 3;
use constant syslogCommand   => 4;
use constant syslogSuccess   => 5;


#use Sys::Hostname;

# feel free to hardcode an IP if you are concerned about this lookup's
#  efficiency, or you need different information, or you prefer
#  to use localhost's IP or whatnot.
#my $ipaddr = unpack('N', gethostbyname(hostname()));

my $dbhost = 'localhost';
my $dbuser = 'newsmgr';

# The password can be entered in three ways: Either hardcode it into $dbpass,
#  (which is a security risk, but is faster if you have a completely closed
#  system), or leave $dbpass set to undef, in which case this script will try
#  to read the password from the file specified in $dbpassfile (which means
#  that this script and the database can be touched by anyone with read access
#  to that file), or leave both undef to have DBI get the password from the
#  DBI_PASS environment variable, which is the most secure, but least
#  convenient.
my $dbpass = undef;
my $dbpassfile = '/etc/IcculusNews_dbpass.txt';

my @default_users = (
   {
        name => "icculus",
        password => "blah",
        email => "icculus\@clutteredmind.org",
        globalrights => 0xFFFFFFFF & ~canNotAuthorize,
   },
   {
        name => "fingermaster",
        password => "blah",
        email => "fingermaster\@icculus.org",
        globalrights => 0x00000000,
   },
   {
        name => "cvsmaster",
        password => "blah",
        email => "cvsmaster\@icculus.org",
        globalrights => 0x00000000,
   },
   {
        name => "mailgateway",
        password => "blah",
        email => "cvsmaster\@icculus.org",
        globalrights => 0x00000000,
   },
);

my @default_queues = (
   {
        name => "icculus.org news",
        flags => "0x00000000",
        description => "news from icculus.org: the open source incubator",
        rdffile => "news_icculus_org2.rdf",
        siteurl => "http://icculus.org/",
        rdfurl => "http://icculus.org/news/news_icculus_org2.rdf",
        rdfitemcount => "5",
        itemviewurl => "http://icculus.org/news/news2.php?id=%id",
        rdfimageurl => "http://icculus.org/icculus-org-now.png",
        owner => "1",
   },
);

my $dbname_src = 'IcculusNews';
my $dbname_dst = 'IcculusNewsBeta';
my $dbtable_items = 'news_items';
my $dbtable_users = 'news_users';
my $dbtable_queues = 'news_queues';
my $dbtable_queue_rights = 'news_queue_rights';

if (not defined $dbpass) {
    if (defined $dbpassfile) {
	open(FH, $dbpassfile) or die("failed to open $dbpassfile: $!\n");
	$dbpass = <FH>;
        chomp($dbpass);
	$dbpass =~ s/\A\s*//;
        $dbpass =~ s/\s*\Z//;
	close(FH);
    }
}

my $dsn;
my $sql;

if (0) {
$dsn = "DBI:mysql:database=$dbname_src;host=$dbhost";
print(" * Connecting to [$dsn] ...\n");
my $link_src = DBI->connect($dsn, $dbuser, $dbpass, {'AutoCommit' => 0});
}

$dsn = "DBI:mysql:database=$dbname_dst;host=$dbhost";
print(" * Connecting to [$dsn] ...\n");
my $link_dst = DBI->connect($dsn, $dbuser, $dbpass, {'AutoCommit' => 0});

print(" * Nuking destination tables to prepare for copying...\n");
$link_dst->do("drop table $dbtable_items");
$link_dst->do("drop table $dbtable_users");
$link_dst->do("drop table $dbtable_queues");
$link_dst->do("drop table $dbtable_queue_rights");

print(" * Recreating tables...\n");

$sql = "create table $dbtable_items (" .
       " id mediumint unsigned not null auto_increment," .
       " queueid mediumint unsigned not null," .
       " ip int unsigned not null," .
       " deleted tinyint unsigned default 0," .
       " approved tinyint unsigned default 0," .
       " title varchar(128) not null," .
       " text text not null," .
       " author mediumint unsigned default 0," .
       " postdate datetime not null," .
       " primary key (id)" .
       ")";
$link_dst->do($sql) or die "can't execute the query: $link_dst->errstr";

$sql = "create table $dbtable_users (" .
       " id int unsigned not null auto_increment," .
       " name varchar(32) not null," .
       " password varchar(32) not null," .
       " email varchar(64) not null," .
       " defaultqueue int unsigned not null," .
       " globalrights int unsigned not null," .
       " created datetime not null," .
       " primary key (id)" .
       ")";
$link_dst->do($sql) or die "can't execute the query: $link_dst->errstr";

$sql = "create table $dbtable_queues (" .
       " id int unsigned not null auto_increment," .
       " flags int unsigned not null," .
       " name varchar(32) not null," .
       " description varchar(255) not null," .
       " rdffile varchar(255) not null," .
       " siteurl varchar(255) not null," .
       " rdfurl varchar(255) not null," .
       " rdfitemcount tinyint unsigned not null," .
       " itemarchiveurl varchar(255) not null," .
       " itemviewurl varchar(255) not null," .
       " rdfimageurl varchar(255)," .
       " created datetime not null," .
       " owner int unsigned not null," .
       " primary key (id)" .
       ")";
$link_dst->do($sql) or die "can't execute the query: $link_dst->errstr";

$sql = "create table $dbtable_queue_rights (" .
       " uid int unsigned not null," .
       " qid int unsigned not null," .
       " rights int unsigned not null" .
       ")";
$link_dst->do($sql) or die "can't execute the query: $link_dst->errstr";

print(" * Creating default users...\n");
foreach (@default_users) {
    my $name = $link_dst->quote($_->{"name"});
    my $salt = (join('', ('.', '/', 0..9, 'A'..'Z', 'a'..'z')[rand(64), rand(64)]));
    my $pass = $link_dst->quote(crypt($_->{"password"}, $salt));
    my $email = $link_dst->quote($_->{"email"});
    my $globalrights = $_->{"globalrights"};

    $sql = "insert into $dbtable_users" .
           " (name, password, globalrights, email, created)" .
           " values ($name, $pass, $globalrights, $email, NOW())";
    $link_dst->do($sql) or die "can't execute the query: $link_dst->errstr";
}

print(" * Creating default queues...\n");
foreach (@default_queues) {
    my $n = $link_dst->quote($_->{"name"});
    my $f = $_->{"flags"};
    my $d = $link_dst->quote($_->{"description"});
    my $rdff = $link_dst->quote($_->{"rdffile"});
    my $rdfu = $link_dst->quote($_->{"rdfurl"});
    my $url = $link_dst->quote($_->{"siteurl"});
    my $rdfitemcount = $_->{"rdfitemcount"};
    my $viewurl = $link_dst->quote($_->{"itemviewurl"});
    my $owner = $_->{"owner"};
    my $rdfimg = undef;

    if (defined $_->{"rdfimageurl"}) {
        $rdfimg = $link_dst->quote($_->{"rdfimageurl"});
    }

    $sql = "insert into $dbtable_queues" .
           " (name, flags, description, rdffile, rdfurl, siteurl," .
           " rdfitemcount, itemviewurl, created, owner";

    $sql .= ", rdfimageurl" if (defined $rdfimg);

    $sql .= ") values ($n, $f, $d, $rdff, $rdfu, $url," .
            " $rdfitemcount, $viewurl, NOW(), $owner";
    $sql .= ", $rdfimg" if (defined $rdfimg);
    $sql .= ")";
    $link_dst->do($sql) or die "can't execute the query: $link_dst->errstr";
}

if (0) {

$sql = "select id, ip, approved, deleted, title, text, author, postdate" .
       " from $dbtable_items order by id";
print(" * Initial query...\n");
#print(" * SQL is $sql\n");
my $sth = $link_src->prepare($sql);
$sth->execute() or die "can't execute the query: $sth->errstr";

my $lastid = 0;
while (my @row = $sth->fetchrow_array()) {
    my $id = $row[0];
    my $ipaddr = $row[1];
    my $approved = $row[2];
    my $deleted = $row[3];
    my $title = $link_dst->quote($row[4]);
    my $text = $link_dst->quote($row[5]);
    my $postdate = $link_dst->quote($row[7]);
    my $author = undef;

    $author = 1 if ($row[6] eq 'icculus');
    $author = 2 if ($row[6] eq 'fingermaster');
    print("Unknown author $row[6]\n"), $author = 0 if (not defined $author);

    $lastid++;
    while ($lastid < $id) {
        $sql = "insert into $dbtable_items" .
               " (queueid, ip, approved, deleted, title, text, author, postdate)" .
               " values (1, 0, 0, 1, 'delete me', 'delete me', 0, NOW())";
        print(" * Inserting dummy news item ($lastid)...\n");
        #print(" * SQL is $sql\n");
        $link_dst->do($sql) or die "can't execute the query: $link_dst->errstr";
        $lastid++;
    }

    $sql = "insert into $dbtable_items" .
           " (queueid, ip, approved, deleted, title, text, author, postdate)" .
           " values (1, $ipaddr, $approved, $deleted, $title, $text," .
           " $author, $postdate)";
    print(" * Inserting news item ($id)...\n");
    #print(" * SQL is $sql\n");
    $link_dst->do($sql) or die "can't execute the query: $link_dst->errstr";
}

print(" * Committing...\n");
$link_dst->commit();

print(" * purging deleted/dummy news items...\n");
$sql = "delete from $dbtable_items where deleted=1";
$link_dst->do($sql) or die "can't execute the query: $link_dst->errstr";

$sth->finish();
}

print(" * Committing...\n");
$link_dst->commit();

print(" * Disconnecting...\n");

if (0) {
    $link_src->disconnect();
}

$link_dst->disconnect();

print(" * done.\n");
exit 0;

# end of convert.pl ...

