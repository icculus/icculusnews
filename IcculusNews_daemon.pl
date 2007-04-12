#!/usr/bin/perl -w -T
#-----------------------------------------------------------------------------
#
#  Copyright (C) 2000 Ryan C. Gordon (icculus@icculus.org)
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
#-----------------------------------------------------------------------------

# !!! FIXME: Remove most of the "die()" calls.

#-----------------------------------------------------------------------------
# Revision history:
#-----------------------------------------------------------------------------

use strict;      # don't touch this line, nootch.
use warnings;    # don't touch this line, either.
use DBI;         # or this. I guess. Maybe.
#use Socket;      # in fact, if it says "use", don't touch it.
#use IO::Handle;  # blow.
use HTML::Entities;

# Version of IcculusNews. Change this if you are forking the code.
my $version = "2.0.2";


# Global rights constants.
use constant canSeeAllQueues           => 1 << 0;
use constant canSeeAllDeleted          => 1 << 1;
use constant canSeeAllUnapproved       => 1 << 2;
use constant canDeleteAllItems         => 1 << 3;
use constant canPurgeAllItems          => 1 << 4;
use constant canApproveAllItems        => 1 << 5;
use constant canTweakUsers             => 1 << 6;
use constant canCreateQueues           => 1 << 7;
use constant canLockUsers              => 1 << 8;
use constant canLockAllQueues          => 1 << 9;
use constant canChangeOthersPasswords  => 1 << 10;
use constant canEditAllItems           => 1 << 11;
use constant canNotAuthorize           => 1 << 12;
use constant canAccessAllLockedQueues  => 1 << 13;
use constant canCreateUsers            => 1 << 14;
use constant canMoveAllItems           => 1 << 15;

# Queue rights constants.
use constant canSeeInvisible  => 1 << 0;
use constant canSeeDeleted    => 1 << 1;
use constant canSeeUnapproved => 1 << 2;
use constant canDeleteItems   => 1 << 3;
use constant canPurgeItems    => 1 << 4;
use constant canApproveItems  => 1 << 5;
use constant canEditItems     => 1 << 6;
use constant canMakeInvisible => 1 << 7;
use constant canLockQueue     => 1 << 8;
use constant canAccessLocked  => 1 << 9;
use constant canMoveItems     => 1 << 10;

# Queue flags constants.
use constant queueInvisible => 1 << 0;
use constant queueLocked    => 1 << 1;

# syslog constants.
use constant syslogNone     => 0;
use constant syslogError    => 1;
use constant syslogDaemon   => 2;
use constant syslogAuth     => 3;
use constant syslogCommand  => 4;
use constant syslogSuccess  => 5;
use constant syslogAll      => 0xFFFFFFFF;

#-----------------------------------------------------------------------------#
#             CONFIGURATION VARIABLES: Change to suit your needs...           #
#-----------------------------------------------------------------------------#

# Set this to one to log just errors to the standard Unix syslog facility
#  (requires Sys::Syslog qw(:DEFAULT setlogsock) ...). Set it to two to also
#  log daemon start/stop info. Three logs all auth attempts. Set it to four to
#  also log all news commands sent by a client. Set it to zero to disable.
#  (see constants, above)
my $use_syslog = syslogDaemon;

# Set this to name of your site. Some daemon messages and forgotten password
#  emails will include it.
my $sitename = 'icculus.org';

# Set this to the email address for the guy that takes care of problems.
#  Some error messages will include it.
my $admin_email = 'newsmaster@icculus.org';

# Set this to the number of seconds you'd like to delay before responding to
#  an incorrect authentication. The longer the delay, the more you frustrate
#  crackers that are trying to brute-force their way into someone else's
#  login. Then again, the longer the delay, the more you frustrate your valid
#  users when they mistype their passwords. 2 seconds seems to be a good
#  compromise. This must be an integer value; no fractions, please.
my $invalid_auth_delay = 2;

# This is the maximum size, in bytes, that a command can be. This is
#  to prevent malicious clients from trying to fill all of system memory.
my $max_command_size = 1024;

# This is the maximum size, in bytes, that a news entry can be when submitted
#  through this daemon. This is to prevent malicious clients from trying to
#  fill all of system memory. Make this big, though. Stuff in the database
#  is free game; there is no size limit on what is read back from there, since
#  we assume that data can be trusted.
my $max_news_size = 128 * 1024;

# This is the name of your anonymous account.
my $anon = "anonymous hoser";

# You can screw up your output with this, if you like.
#  (Not used at the moment, check syslog instead.)
my $debug = 0;

# The processes path is replaced with this string, for security reasons, and
#  to satisfy the requirements of Taint mode. Make this as simple as possible.
my $safe_path = '/usr/bin:/usr/local/bin';

# Turn the process into a daemon. This will handle creating/answering socket
#  connections, and forking off children to handle them. This flag can be
#  toggled via command line options (--daemonize, --no-daemonize, -d), but
#  this sets the default. Daemonizing tends to speed up processing (since the
#  script stays loaded/compiled), but may cause problems on systems that
#  don't have a functional fork() or IO::Socket::INET package. If you don't
#  daemonize, this program reads requests from stdin and writes results to
#  stdout, which makes it suitable for command line use or execution from
#  inetd and equivalents.
my $daemonize = 0;

# This is only used when daemonized. Specify the port on which to listen for
#  incoming connections. The randomly-chosen "official" IcculusNews port is
#  currently 263. Hopefully this won't have to change.
my $server_port = 263;

# Set this to immediately drop priveledges by setting uid and gid to these
#  values. Set to undef to not attempt to drop privs. You can keep the privs
#  by setting these to undef (risky!), if you really want to.
#my $wanted_uid = undef;
#my $wanted_gid = undef;
my $wanted_uid = 1083;  # (This is the uid of "iccnews" ON _MY_ SYSTEM.)
my $wanted_gid = 1006;  # (This is the gid of "iccnews" ON _MY_ SYSTEM.)

# This is only used when daemonized. Specify the maximum number of clients
#  to service at once. A separate child process is fork()ed off for each
#  client, and if there are more simulatenous connections then this value, the
#  extra clients will be made to wait until some of the current requests are
#  serviced. 5 to 10 is usually a good number. Set it higher if you get a
#  massive amount of finger requests simultaneously.
my $max_connects = 10;

# This is how long, in seconds, before an idle connection will be summarily
#  dropped. This prevents abuse from people hogging a connection without
#  actually sending a request, without this, enough connections like this
#  will block legitimate ones. At worst, they can only block for this long
#  before being booted and thus freeing their connection slot for the next
#  guy in line. Setting this to undef lets people sit forever, but removes
#  reliance on the IO::Select package. Note that this timeout is how long
#  the user has to complete the read_from_client() function, so don't set
#  it so low that legitimate lag can kill them. The default is usually safe.
my $read_timeout = 15;

# This is the base directory for writing RDF files to. Make sure that this
#  process has write access and that there's a dir separator at the end!
my $rdfbase = '/webspace/rdf/';

# New users are assigned these rights by default. Refer to "Global rights
#  constants", above. If you want to lock out new accounts until you manually
#  approve them, "canNotAuthorize" is appropriate. If you want anyone to be
#  able to create personal news queues without running it by you first,
#  "canCreateQueues" is a good idea.
my $default_user_rights = canCreateQueues | canCreateUsers;

# This is the same as $default_user_rights, but for the anonymous account.
#  If you want to lock out new users altogether, don't set canCreateUsers
#  here; this way account creation has to go through users you have
#  authorized to create accounts, which is militant, but potentially useful.
my $anonymous_user_rights = canCreateUsers;

# Owners of a queue, when initially creating the queue, are assigned these
#  rights by default. Refer to "Queue rights constants", above. You probably
#  want to give them everything. These rights only apply to the queue creator
#  when interacting with her own queue.
my $default_queue_owner_rights = canSeeInvisible | canSeeDeleted |
                                 canSeeUnapproved | canDeleteItems |
                                 canPurgeItems | canApproveItems |
                                 canEditItems | canMakeInvisible |
                                 canLockQueue | canAccessLocked;

# New queues are assigned these flags by default. Refer to "Queue flags
#  constants", above. This should probably be left at zero, unless you want
#  to make queues globally invisible until the creator toggles them.
my $default_queue_flags = 0;

# Newly created users are assigned to this queue by default. The first queue
#  created is 1, so that's probably a good one, unless you've got some other
#  queue you prefer. Users can change to a different queue via the QUEUE
#  command, and assign a new default queue with the SETDEFAULTQUEUE command.
# If the default queue is 0, the web interface takes users to the "post"
#  action by default, instead of the queue view.
my $default_queue = 0;

# This is the minimum number of characters that will be used when generating
#  a new password to replace a forgotten one. Eight is usually reasonable.
my $forgotten_password_minsize = 8;

# This is the maximum number of characters that will be used when generating
#  a new password to replace a forgotten one. Ten to twelve is usually
#  reasonable.
my $forgotten_password_maxsize = 12;

# Set this to the name of a device to read random data from. If you don't have
#  such a device (not on a Unix box or something), set it to undef to just
#  use the built-in rand() function.
my $random_device = '/dev/urandom';

# This is the host to connect to for database access.
my $dbhost = 'localhost';

# This is the username for the database connection.
my $dbuser = 'newsmgr';

# The database password can be entered in three ways: Either hardcode it into
#  $dbpass, (which is a security risk, but is faster if you have a completely
#  closed system), or leave $dbpass set to undef, in which case this script
#  will try to read the password from the file specified in $dbpassfile (which
#  means that this script and the database can be touched by anyone with read
#  access to that file), or leave both undef to have DBI get the password from
#  the DBI_PASS environment variable, which is the most secure, but least
#  convenient.
my $dbpass = undef;
my $dbpassfile = '/etc/IcculusNews_dbpass.txt';

# The name of the database to use once connected to the database server.
my $dbname = 'IcculusNews';

# Names of database tables.
my $dbtable_users = 'news_users';
my $dbtable_queues = 'news_queues';
my $dbtable_items = 'news_items';
my $dbtable_queue_rights = 'news_queue_rights';


#-----------------------------------------------------------------------------#
#     The rest is probably okay without you laying yer dirty mits on it.      #
#-----------------------------------------------------------------------------#

my $link = undef;      # link to database.
my $auth_uid = undef;  # authenticated user id.
my $ipaddr = undef;    # IP address of client.
my $queue = 0;
my $current_global_rights = 0;
my $current_queue_rights = 0;
my %commands;


sub long2ip {
    my $l = shift;
    my $x1 = (($l & 0xFF000000) >> 24);
    my $x2 = (($l & 0x00FF0000) >> 16);
    my $x3 = (($l & 0x0000FF00) >>  8);
    my $x4 = (($l & 0x000000FF) >>  0);
    return("$x1.$x2.$x3.$x4");
}


sub do_log {
    my $level = shift;
    my $text = shift;

    if ($use_syslog >= $level) {
        $text =~ s/%/%%/g;  # syslog apparently does formatting or something.
        syslog("info", "$text\n")
             or report_fatal("Couldn't write to syslog: $!");
    }
}


my $in_error_handler = 0;
sub handle_error {
    my ($errmsg, $fatal) = @_;

    print "- $errmsg\012";

    if (not $in_error_handler) {
        $in_error_handler = 1;
        do_log(syslogError, "IcculusNews error: \"$errmsg\"");
        $in_error_handler = 0;
    }

    if ($fatal) {
        $link->disconnect() if defined $link;
        exit 42;
    }
}


sub report_error {
    handle_error($_[0], 0);
}


sub report_fatal {
    handle_error($_[0], 1);
}


sub report_success {
    my $errmsg = shift;
    do_log(syslogSuccess, "IcculusNews success: \"$errmsg\"");
    print "+ $errmsg\012";
}


sub new_crypt_salt {
    return(join('', ('.', '/', 0..9, 'A'..'Z', 'a'..'z')[rand(64), rand(64)]));
}


sub get_database_link {
    if (not defined $link) {
        if (not defined $dbpass) {
            if (defined $dbpassfile) {
                open(FH, $dbpassfile)
                    or report_fatal("failed to open $dbpassfile: $!");
                $dbpass = <FH>;
                chomp($dbpass);
                $dbpass =~ s/\A\s*//;
                $dbpass =~ s/\s*\Z//;
                close(FH);
            }
        }

        my $dsn = "DBI:mysql:database=$dbname;host=$dbhost";
        $link = DBI->connect($dsn, $dbuser, $dbpass)
            or report_fatal(DBI::errstr);
    }

    return($link);
}


sub read_from_client {
    my $max_chars = shift;
    my $retval = '';
    my $count = 0;
    my $s = undef;
    my $elapsed = undef;
    my $starttime = undef;

    if (defined $read_timeout) {
        $s = new IO::Select();
        $s->add(fileno(STDIN));
        $starttime = time();
        $elapsed = 0;
    }

    while (1) {
        if (defined $read_timeout) {
            my $ready = scalar($s->can_read($read_timeout - $elapsed));
            report_fatal("input timeout.") if (not $ready);
            $elapsed = (time() - $starttime);
        }

        my $ch;
        my $rc = sysread(STDIN, $ch, 1);
        report_fatal("unexpected EOF.") if ($rc != 1);
        if ($ch ne "\015") {
            return($retval) if ($ch eq "\012");
            $retval .= $ch;
            $count++;
            report_fatal("input overflow. chatter less.") if ($count >= $max_chars);
        }
    }

    return(undef);  # shouldn't ever hit this.
}


sub process_command {
    my $req = shift;
    my $cmdstr = read_from_client($max_command_size);
    my ($cmd, $args) = $cmdstr =~ /\A\s*([a-zA-Z]+)\s*(.*)\Z/;

    $args = undef if ((defined $args) and ($args eq ''));

    if (not defined $cmd) {
        $cmd = '';
    } else {
        $cmd =~ tr/a-z/A-Z/;
    }

    report_fatal("Required $req") if ((defined $req) and ($req ne $cmd));
    report_success("Uh, okay."), return 1 if ($cmd eq '');  # blank line.

    do_log(syslogCommand, "IcculusNews command: \"$cmdstr\"");

    if (defined $commands{$cmd}) {
        return($commands{$cmd}->($args));
    }

    report_error("Unknown command \"$cmd\".");
    return(1);
}

sub update_queue_rights {
    $current_queue_rights = shift;

    if ($current_global_rights & canSeeAllDeleted) {
        $current_queue_rights |= canSeeDeleted;
    }

    if ($current_global_rights & canSeeAllUnapproved) {
        $current_queue_rights |= canSeeUnapproved;
    }

    if ($current_global_rights & canDeleteAllItems) {
        $current_queue_rights |= canDeleteItems;
    }

    if ($current_global_rights & canPurgeAllItems) {
        $current_queue_rights |= canPurgeItems;
    }

    if ($current_global_rights & canApproveAllItems) {
        $current_queue_rights |= canApproveItems;
    }

    if ($current_global_rights & canEditAllItems) {
        $current_queue_rights |= canEditItems;
    }

    if ($current_global_rights & canSeeAllQueues) {
        $current_queue_rights |= canSeeInvisible;
    }

    if ($current_global_rights & canLockAllQueues) {
        $current_queue_rights |= canLockQueue;
    }

    if ($current_global_rights & canAccessAllLockedQueues) {
        $current_queue_rights |= canAccessLocked;
    }

    if ($current_global_rights & canMoveAllItems) {
        $current_queue_rights |= canMoveItems;
    }
}


sub generate_rdf {
    my $qid = shift;
    my $max = shift;

    $qid = $queue if not defined $qid;
    return ('bogus queue id', undef, undef) if ($qid <= 0);

    my $sql;
    my $link = get_database_link();
    my @row = undef;
    my $sth = undef;
    $sql = "select name, description, itemarchiveurl, itemviewurl, rdffile," .
           " siteurl, rdfurl, rdfimageurl, rdfitemcount" .
           " from $dbtable_queues where id=$qid";
    $sth = $link->prepare($sql);
    $sth->execute() or die "can't execute the query: $sth->errstr";
    @row = $sth->fetchrow_array();
    $sth->finish();
    return ('failed to read queue attributes', undef, undef) if (not @row);

    my $queuename   = encode_entities($row[0]);
    my $queuedesc   = encode_entities($row[1]);
    my $itemarcurl  = encode_entities($row[2]);
    my $itemviewurl = $row[3]; # must html encode later!
    my $rdffile     = $row[4];
    my $siteurl     = encode_entities($row[5]);
    my $rdfurl      = encode_entities($row[6]);
    my $rdfimgurl   = encode_entities($row[7]);
    $max = $row[8] if not defined $max;

    $sql = "select t1.author, t1.id, t1.title, t1.postdate, t2.name," .
           " t1.ip, t1.approved, t1.deleted, t1.text" .
           " from $dbtable_items as t1" .
           " left outer join $dbtable_users as t2" .
           " on t1.author=t2.id" .
           " where t1.queueid=$queue" .
           " and t1.approved=1 and t1.deleted=0" .
           " order by postdate desc limit $max";

    $sth = $link->prepare($sql);
    $sth->execute() or die "can't execute the query: $sth->errstr";

    my $channelimg = '';
    my $rdfimg = '';
    if (defined $rdfimgurl) {
        $channelimg = "<image rdf:resource=\"$rdfimgurl\" />";
        $rdfimg .=   "<image>\n";
        $rdfimg .= "    <title>$queuename</title>\n";
        $rdfimg .= "    <url>$rdfimgurl</url>\n";
        $rdfimg .= "    <link>$siteurl</link>\n";
        $rdfimg .= "  </image>";
    }

    my $rdfitems = '';
    my $digestitems = '';
    while (my @row = $sth->fetchrow_array()) {
        my $authorid     = $row[0];
        my $itemid       = $row[1];
#        my $itemtitle    = encode_entities($row[2]);
        my $itemtitle    = $row[2];
        my $itempostdate = encode_entities($row[3]);
        my $authorname   = encode_entities(((not defined $row[4]) ? $anon : $row[4]));
        my $ipaddr       = long2ip($row[5]);
        my $approved     = $row[6];
        my $deleted      = $row[7];
        my $text         = encode_entities($row[8]);
        my $viewurl = $itemviewurl;
        1 while ($viewurl =~ s/\%id/$itemid/);
        $viewurl = encode_entities($viewurl);

        $digestitems .= "<rdf:li rdf:resource=\"$viewurl\" />\n        ";

        $rdfitems .= "\n";
        $rdfitems .= "  <item rdf:about=\"$viewurl\">\n";
        $rdfitems .= "    <title>$itemtitle</title>\n";
        $rdfitems .= "    <link>$viewurl</link>\n";
        $rdfitems .= "    <author>$authorname</author>\n";
        $rdfitems .= "    <authorid>$authorid</authorid>\n";
        $rdfitems .= "    <itemid>$itemid</itemid>\n";
        $rdfitems .= "    <postdate>$itempostdate</postdate>\n";
        $rdfitems .= "    <ipaddr>$ipaddr</ipaddr>\n";
        $rdfitems .= "    <approved>$approved</approved>\n";
        $rdfitems .= "    <deleted>$deleted</deleted>\n";
        $rdfitems .= "    <description>\n$text\n    </description>\n";
        $rdfitems .= "  </item>";
    }
    $sth->finish();

    my $rdf = <<__EOF__;
<?xml version="1.0" encoding="utf-8"?>

<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns="http://purl.org/rss/1.0/">

  <channel rdf:about="$rdfurl">
    <title>$queuename</title>
    <link>$siteurl</link>
    <archives>$itemarcurl</archives>
    <description>$queuedesc</description>
    $channelimg
    <items>
      <rdf:Seq>
        $digestitems
      </rdf:Seq>
    </items>
  </channel>

  $rdfimg
  $rdfitems
</rdf:RDF>


__EOF__

    return(undef, $rdf, $rdffile);
}


sub update_rdf {
    my ($err, $rdf, $filename) = generate_rdf();
    do_log(syslogError, "RDF build failed: $err"), return 0 if (defined $err);

    $filename ="$rdfbase$filename";
    if (not open(RDF, '>', $filename)) {
        do_log(syslogError, "Failed to open $filename: $!");
        return 0;
    }

    print RDF $rdf;
    close(RDF);
    return 1;
}


sub toggle_approve {
    my $id = shift;
    my $newval = shift;
    if ((not defined $id) or ($id !~ /\A\d+\Z/)) {
        report_error('argument must be number.');
        return 1;
    }
    report_error('no queue selected.'), return 1 if $queue == 0;

    if (!($current_queue_rights & canApproveItems)) {
        report_error("You don't have permission to do that.");
        return 1;
    }

    $newval = 1 if $newval;  # make sure it's '1', as opposed to non-zero.
    my $otherval = (($newval) ? 0 : 1);

    my $link = get_database_link();
    my $sql = "update $dbtable_items set approved=$newval" .
              " where id=$id and queueid=$queue and" .
              " deleted=0 and approved=$otherval";

    my $rc = $link->do($sql);
    if (not defined $rc) {
        report_error("can't execute the query: $link->errstr");
    } elsif ($rc > 0) {
        report_success("Approve flag toggled.");
        update_rdf();
    } else {
        my $reason = "no idea why";
        $sql = "select approved, deleted from $dbtable_items" .
               " where id=$id and queueid=$queue";
        my $sth = $link->prepare($sql);
        $sth->execute() or die "can't execute the query: $sth->errstr";
        my @row = $sth->fetchrow_array();
        if (not @row) {
            $reason = "no such item";
        } else {
            if ($row[0] == $newval) {
                $reason = "already set like that";
            } elsif ($row[1]) {
                $reason = "item is flagged for deletion";
            }
        }
        $sth->finish();
        report_error("Failed to toggle approval flag: $reason.");
    }
    return(1);
}


sub toggle_delete {
    my $id = shift;
    my $newval = shift;
    if ((not defined $id) or ($id !~ /\A\d+\Z/)) {
        report_error('argument must be number.');
        return 1;
    }
    report_error('no queue selected.'), return 1 if $queue == 0;

    if (!($current_queue_rights & canDeleteItems)) {
        report_error("You don't have permission to do that.");
        return(1);
    }

    $newval = 1 if $newval;  # make sure it's '1', as opposed to non-zero.
    my $otherval = (($newval) ? 0 : 1);

    my $link = get_database_link();
    my $sql = "update $dbtable_items set deleted=$newval" .
              " where id=$id and queueid=$queue and deleted=$otherval";

    my $rc = $link->do($sql);
    if (not defined $rc) {
        report_error("can't execute: $link->errstr");
    } elsif ($rc > 0) {
        report_success("Deletion flag toggled.");
        update_rdf();
    } else {
        my $reason = "no idea why";
        $sql = "select deleted from $dbtable_items" .
                " where id=$id and queueid=$queue";
        my $sth = $link->prepare($sql);
        $sth->execute() or die "can't execute the query: $sth->errstr";
        my @row = $sth->fetchrow_array();
        if (not @row) {
            $reason = "no such item";
        } else {
            if ($row[0] == $otherval) {
                $reason = "already set like that";
            }
        }
        $sth->finish();
        report_error("Failed to toggle deletion flag: $reason.");
    }
    return(1);
}


sub queue_is_forbidden {
    my $queueflags = shift;
    my $userrights = shift;
    my $owner = shift;

    #return 0 if ((defined $owner) and ($owner == $auth_uid));

    if ($queueflags & queueInvisible) {
        if (!($current_global_rights & canSeeAllQueues)) {
            if (defined $userrights) {
                return 1 if (!($userrights & canSeeInvisible));
            }
        }
    }

    if ($queueflags & queueLocked) {
        if (!($current_global_rights & canAccessAllLockedQueues)) {
            if (defined $userrights) {
                return 1 if (!($userrights & canAccessLocked));
            }
        }
    }

    return 0;  # it's kosher.
}


sub change_queue {
    my $args = shift;

    if ($args == 0) {
        $queue = 0;
        update_queue_rights(0);
    } else {
        my $link = get_database_link();
        my $sql = "select q.flags, r.rights, q.owner" .
                  " from $dbtable_queues as q" .
                  " left outer join $dbtable_queue_rights as r" .
                  " on r.qid=q.id and r.uid=$auth_uid" .
                  " where q.id=$args";

        my $sth = $link->prepare($sql);
        $sth->execute() or report_fatal("can't execute query: $sth->errstr");
        my @row = $sth->fetchrow_array();
        $sth->finish();

        $row[1] = 0 if ((@row) and (not defined $row[1]));

        if ((not @row) or (queue_is_forbidden($row[0], $row[1], $row[2]))) {
            return("Can't select that queue.");
        } else {
            update_queue_rights($row[1]);
            $queue = $args;
        }
    }

    return(undef); # no error.
}


sub can_change_queue {
    my $newqueue = shift;

    my $starting_queue = $queue;
    my $starting_queue_rights = $current_queue_rights;
    my $err = change_queue($newqueue);

    # set this directly back.
    $queue = $starting_queue;
    $current_queue_rights = $starting_queue_rights;
    return($err);
}


sub send_forgotten_password {
    my ($user, $email, $pword) = @_;

    $email = $1 if ($email =~ /\A(.*)\Z/); # untaint email address var.

    my $msg = <<__EOF__;
From: $admin_email
Reply-To: $admin_email
To: $email
Subject: Forgotten IcculusNews password.

Hello, $user.

 This is the IcculusNews daemon at $sitename. Someone, possibly you, has
  told us that the account password associated with this email address has
  been forgotten. In response, we have chosen a new password for the account.
  If it wasn't you, don't worry; the smart-ass who entered the forgotten
  password request does not have your newly-generated password, unless they
  are reading this email.

 Please log in to IcculusNews with the following password and change it to
  something else. The sooner the better, too, since this email was probably
  sent unencrypted across the Internet, and we're paranoid about wiretaps and
  packet sniffers.

     Username     : $user
     New Password : $pword

--The McManagement, $sitename.
   $admin_email


__EOF__

    open(MAILH, '|/usr/sbin/sendmail -t') or
        return("Failed to run qmail-inject: $!");

    if ( (not print MAILH $msg) or (not close(MAILH)) ) {
	my $err = "Failed to write to qmail-inject pipe; $!";
	close(MAILH);
        return($err);
    }

    return(undef);  # no error.
}


sub generate_pword {
    my $retval = '';
    my $diff = $forgotten_password_maxsize - $forgotten_password_minsize;
    my $pwordbytes = $forgotten_password_minsize + int(rand($diff + 1));

    my $dev = ((defined $random_device) and (open(RANDH, '<', $random_device)));
    while ($pwordbytes > 0) {
        my $idx = int((($dev) ? 62.0 * ord(getc(RANDH)) / 256.0 : rand(62)));
	$retval .= (0..9, 'A'..'Z', 'a'..'z')[$idx];
        $pwordbytes--;
    }

    close(RANDH) if ($dev);
    return($retval);
}


# The actual commands the daemon responds to...

$commands{'AUTH'} = sub {
    my $args = shift;
    my $authuser;

    if ((defined $args) and ($args eq '-')) {
        $authuser = "anonymous account";
        $auth_uid = 0;
        $current_queue_rights = 0;
        $current_global_rights = $anonymous_user_rights;
    } else {
        my ($user, $pass) = (undef, undef);
        if (defined $args) {
            ($user, $pass) = $args =~ /\A\"(.+?)\"\s*\"(.+?)\"\Z/
        }

        if ((not defined $user) or (not defined $pass)) {
            report_error('USAGE: AUTH <"user"> <"password">');
            return(1);
        }

        $authuser = "\"$user\"";

        my $link = get_database_link();

        my $u = $link->quote($user);
        my $sql = "select id, password, globalrights, defaultqueue" .
                  " from $dbtable_users where name=$u";
        my $sth = $link->prepare($sql);
        $sth->execute() or report_fatal("can't execute query: $sth->errstr");

        my @row = $sth->fetchrow_array();
        $sth->finish();

        if ((not @row) or (crypt($pass, $row[1]) ne $row[1])) {
            sleep($invalid_auth_delay);
            report_error("Authorization for $authuser failed.");
            return(1);
        }

        if ($row[2] & canNotAuthorize) {
            report_error("This account has been disabled.");
            return(1);
        }

        $auth_uid = $row[0];
        $current_global_rights = $row[2];
        my $err = change_queue($row[3]);
        if (defined $err) {
            report_error("Failed to select default queue: $err");
            return(1);
        }
    }

    do_log(syslogAuth, "IcculusNews auth: $authuser (id $auth_uid)");

    report_success("$auth_uid, $queue");
    return(1);
};


$commands{'USERINFO'} = sub {
    report_success("$auth_uid, $queue");
    return 1;
};


$commands{'QUEUEINFO'} = sub {
    my $id = shift;
    if ((not defined $id) or ($id !~ /\A\d+\Z/)) {
        report_error('argument must be number.');
        return 1;
    }

    my $link = get_database_link();
    my $sql = "select q.name, q.description, q.itemarchiveurl," .
              " q.itemviewurl, q.siteurl, q.rdfurl," .
              " q.created, q.owner, u.name, q.flags, r.rights" .
              " from $dbtable_queues as q, $dbtable_users as u" .
              " left outer join $dbtable_queue_rights as r" .
              " on r.qid=q.id and r.uid=$auth_uid" .
              " where q.id=$id and q.owner=u.id";
    my $sth = $link->prepare($sql);
    $sth->execute() or report_fatal("can't execute query: $sth->errstr");
    my @row = $sth->fetchrow_array();
    $sth->finish();

    $row[10] = 0 if ((@row) and (not defined $row[1]));
    if ((not @row) or (queue_is_forbidden($row[9], $row[10], $row[7]))) {
        report_error("Can't select that queue.");
        return 1;
    }

    report_success("Here comes ...");
    print("$row[0]\012");
    print("$row[1]\012");
    print("$row[2]\012");
    print("$row[3]\012");
    print("$row[4]\012");
    print("$row[5]\012");
    print("$row[6]\012");
    print("$row[7]\012");
    print("$row[8]\012");
    return 1;
};


$commands{'QUEUE'} = sub {
    my $args = shift;
    if ((not defined $args) or ($args !~ /\A\d+\Z/)) {
        report_error("Argument must be a number.");
    } else {
        my $err = change_queue($args);
        if (defined $err) {
            report_error($err);
        } else {
            report_success("Queue changed");
        }
    }

    return(1);
};


$commands{'ENUM'} = sub {
    my $args = shift;
    my $type = undef;
    my $link;

    ($type, $args) = $args =~ /\A(queues)\s*(.*)\Z/i if defined $args;
    if (not defined $type) {
        report_error("USAGE: ENUM <queues>");
        return(1);
    }

    $link = get_database_link();

    $type =~ tr/A-Z/a-z/;
    if ($type eq "queues") {
        my $sql = "select q.id, q.name, q.flags, r.rights, q.owner" .
                  " from $dbtable_queues as q" .
                  " left outer join $dbtable_queue_rights as r" .
                  " on r.qid=q.id and r.uid=$auth_uid";

        my $sth = $link->prepare($sql);
        $sth->execute() or report_fatal("can't execute query: $sth->errstr");

        report_success("Here comes...");
        while (my @row = $sth->fetchrow_array()) {
            $row[3] = 0 if (not defined $row[3]);
            next if (queue_is_forbidden($row[2], $row[3], $row[4]));
            print("$row[0]\012");
            print("$row[1]\012");
        }
        print(".\012");
        $sth->finish();
    }

    return(1);
};


$commands{'DIGEST'} = sub {
    my $args = shift;
    my $max = undef;
    my $startIndex = undef;

    if (defined $args) {
        ($startIndex, $max) = ($args =~ /\A(\d+|\-)\s*(\d+)\Z/);
    }

    if ((not defined $max) or (not defined $startIndex)) {
        report_error('USAGE: DIGEST <startIndex|-> <msgCount>');
        return 1;
    }

    report_error('no queue selected.'), return 1 if $queue == 0;

    my $link = get_database_link();
    my $sql = "select i.id, i.title, i.postdate, i.author, u.name, i.ip," .
              " i.approved, i.deleted" .
              " from $dbtable_items as i" .
              " left outer join $dbtable_users as u" .
              " on i.author=u.id" .
              " where i.queueid=$queue";

    if ($startIndex ne '-') {
        $sql = $sql . " and (i.id < $startIndex)";
    }

    if (!($current_queue_rights & canSeeDeleted)) {
        $sql = $sql . ' and (i.deleted=0)';
    }

    if (!($current_queue_rights & canSeeUnapproved)) {
        $sql = $sql . ' and (i.approved=1)';
    }

    $sql .= " order by postdate desc limit $max";

    my $sth = $link->prepare($sql);
    $sth->execute() or die "can't execute the query: $sth->errstr";
    report_success("here comes ...");
    while (my @row = $sth->fetchrow_array()) {
        $row[4] = $anon if (not defined $row[4]);
        $row[5] = long2ip($row[5]);
        foreach (@row) {
            print("$_\012");
        }
    }
    print(".\012");
    $sth->finish();

    return(1);
};


$commands{'GET'} = sub {
    my $id = shift;
    if ((not defined $id) or ($id !~ /\A\d+\Z/)) {
        report_error('argument must be number.');
        return 1;
    }
    report_error('no queue selected.'), return 1 if $queue == 0;

    my $link = get_database_link();
    my $sql = "select i.title, i.postdate, i.author, u.name, i.ip," .
              " i.approved, i.deleted, i.text" .
              " from $dbtable_items as i" .
              " left outer join $dbtable_users as u" .
              " on i.author=u.id" .
              " where i.id=$id and i.queueid=$queue";

    if (!($current_queue_rights & canSeeDeleted)) {
        $sql = $sql . ' and (i.deleted=0)';
    }

    if (!($current_queue_rights & canSeeUnapproved)) {
        $sql = $sql . ' and (i.approved=1)';
    }

    $sql = $sql . ' limit 1';

    #print("sql is [$sql].\n");

    my $sth = $link->prepare($sql);
    $sth->execute() or die "can't execute the query: $sth->errstr";
    my @row = $sth->fetchrow_array();
    if (not @row) {
        report_error('no such news item')
    } else {
        $row[3] = $anon if (not defined $row[3]);
        $row[4] = long2ip($row[4]);
        1 while $row[7] =~ s/^\.$/../m;
        report_success('here comes ...');
        foreach (@row) {
            print("$_\012");
        }
        print(".\012");
    }
    $sth->finish();

    return(1);
};


$commands{'MOVEITEM'} = sub {
    my $args = shift;
    my $id;
    my $newqueue;

    ($id, $newqueue) = $args =~ /\A(\d+)\s+(\d+)\s*\Z/ if (defined $args);
    if ((not defined $id) or (not defined $newqueue)) {
        report_error('USAGE: MOVEITEM <itemid> <newqueueid>');
        return 1;
    }

    report_error('no queue selected.'), return 1 if $queue == 0;

    if (!($current_queue_rights & canMoveItems)) {
        report_error("You don't have permission to do that.");
        return(1);
    }

    my $err = can_change_queue($newqueue);
    report_error($err), return 1 if defined $err;

    my $link = get_database_link();
    my $sql = "update $dbtable_items" .
              " set queueid=$newqueue, deleted=0, approved=0" .
              " where id=$id and queueid=$queue";

    my $rc = $link->do($sql);
    if (not defined $rc) {
        report_error("can't execute: $link->errstr");
    } elsif ($rc > 0) {
        report_success("Moved.");
    } else {
        my $reason = "no idea why";
        $sql = "select id from $dbtable_items" .
               " where id=$id and queueid=$queue";
        my $sth = $link->prepare($sql);
        $sth->execute() or die "can't execute the query: $sth->errstr";
        my @row = $sth->fetchrow_array();
        if (not @row) {
            $reason = "no such item";
        }
        $sth->finish();
        report_error("Failed to move: $reason.");
    }
    return(1);
};


$commands{'POST'} = sub {
    my $title = shift;
    my $bytes_left = $max_news_size;
    my $text = '';
    my $input = '';

    report_error('Usage: POST <title>'), return 1 if not defined $title;
    report_error('no queue selected.'), return 1 if $queue == 0;

    report_success("You've got $bytes_left bytes; Go, hose.");

    while (1) {
        $input = read_from_client($bytes_left);
        last if $input eq '.';
        $input =~ s/\A\.\././;
        $text .= $input . "\012";
        $bytes_left -= (length($input) + 1);
    }

    my $link = get_database_link();
    $title = $link->quote($title);
    $text = $link->quote($text);
    my $sql = "insert into $dbtable_items" .
              " (queueid, ip, title, text, author, postdate)" .
              " values ($queue, $ipaddr, $title, $text, $auth_uid, NOW())";

    #print("sql is [$sql].\n");

    if (not defined $link->do($sql)) {
        report_error("can't execute the query: $link->errstr");
    } else {
        report_success("Posted, and awaiting approval.");
        update_rdf();
    }
    return(1);
};


$commands{'EDIT'} = sub {
    my $args = shift;
    my $bogus = 1;
    my $id;
    my $title;

    if (defined $args) {
        ($id, $title) = ($args =~ /\A\s*?(\d+)\s+(.+)\s*\Z/);
        $bogus = 0 if ((defined $id) and (defined $title));
    }
    report_error('Usage: EDIT <id> <title>'), return 1 if ($bogus);

    report_error('no queue selected.'), return 1 if $queue == 0;

    if (!($current_queue_rights & canEditItems)) {
        report_error('permission denied.');
        return 1;
    }

    my $bytes_left = $max_news_size;
    my $text = '';
    my $input = '';

    report_success("You've got $bytes_left; Go, hose.");

    while (1) {
        $input = read_from_client($bytes_left);
        last if $input eq '.';
        $input =~ s/\A\.\././;
        $text .= $input . "\012";
        $bytes_left -= (length($input) + 1);
    }

    my $link = get_database_link();
    $title = $link->quote($title);
    $text = $link->quote($text);
    my $sql = "update $dbtable_items set title=$title, text=$text" .
              " where id=$id and queueid=$queue";

    if (!($current_queue_rights & canEditItems)) {
        $sql .= " and author=$auth_uid";
    }

    #print("sql is [$sql].\n");

    my $rc = $link->do($sql);
    if (not defined $rc) {
        report_error("can't execute the query: $link->errstr");
    } elsif ($rc > 0) {
        report_success("Edited.");
        update_rdf();
    } else {
        report_error("failed; maybe you don't own that item or it's missing?");
    }
    return(1);
};

$commands{'APPROVE'} = sub {
    my $id = shift;
    return(toggle_approve($id, 1));
};


$commands{'UNAPPROVE'} = sub {
    my $id = shift;
    return(toggle_approve($id, 0));
};


$commands{'DELETE'} = sub {
    my $id = shift;
    return(toggle_delete($id, 1));
};


$commands{'UNDELETE'} = sub {
    my $id = shift;
    return(toggle_delete($id, 0));
};


$commands{'PURGE'} = sub {
    my $id = shift;
    if ((not defined $id) or ($id !~ /\A\d+\Z/)) {
        report_error('argument must be number.');
        return 1;
    }
    report_error('no queue selected.'), return 1 if $queue == 0;

    if (!($current_queue_rights & canPurgeItems)) {
        report_error("You don't have permission to do that.");
        return(1);
    }

    my $link = get_database_link();
    my $sql = "delete from $dbtable_items" .
              " where id=$id and queueid=$queue and deleted=1";

    my $rc = $link->do($sql);
    if (not defined $rc) {
        report_error("can't execute: $link->errstr");
    } elsif ($rc > 0) {
        report_success("Purged.");
    } else {
        my $reason = "no idea why";
        $sql = "select deleted from $dbtable_items" .
               " where id=$id and queueid=$queue";
        my $sth = $link->prepare($sql);
        $sth->execute() or die "can't execute the query: $sth->errstr";
        my @row = $sth->fetchrow_array();
        if (not @row) {
            $reason = "no such item";
        } else {
            if (!$row[0]) {
                $reason = "not flagged for deletion";
            }
        }
        $sth->finish();
        report_error("Failed to purge: $reason.");
    }
    return(1);
};


$commands{'PURGEALL'} = sub {
    report_error('no queue selected.'), return 1 if $queue == 0;

    my $link = get_database_link();
    my $sql = "delete from $dbtable_items where queueid=$queue and deleted=1";
    my $rc = $link->do($sql);
    if (not defined $rc) {
        report_error("can't execute: $link->errstr");
    } else {
        report_success("Purged $rc items.");
    }

    return(1);
};


$commands{'CREATEUSER'} = sub {
    my $args = shift;
    my $uname;
    my $email;
    my $pword;

    if ((defined $args) and ($args =~ /\A\"(.+?)\"\s*\"(.+?)\"\s*\"(.+?)\"\Z/)) {
        $uname = $1;
        $email = $2;
        $pword = $3;
    } else {
        report_error('Usage: <"username"> <"email"> <"password">');
        return(1);
    }

    # !!! FIXME: ?
    #if ($uname =~ /[^a-zA-Z0-9]/) {
    #    report_error('Username may only contain letters and numbers');
    #    return(1);
    #}

    if ($uname =~ /[%_<>&\t#]/) {
        report_error('Username has illegal characters');
        return(1);
    }

    if (($current_global_rights & canCreateUsers) == 0) {
        report_error('You are not permitted to create new user accounts.');
        return(1);
    }

    my $link = get_database_link();

    $uname = $link->quote($uname);
    my $sql = "select id from $dbtable_users where name like $uname";
    my $sth = $link->prepare($sql);
    if (not $sth->execute()) {
        report_error("can't execute the query: $sth->errstr");
        return(1);
    }
    my @row = $sth->fetchrow_array();
    $sth->finish();
    if (@row) {
        report_error("That username is taken.");
        return(1);
    }

    $email = $link->quote($email);
    $pword = $link->quote(crypt($pword, new_crypt_salt()));
    $sql = "insert into $dbtable_users" .
           " (name, password, globalrights, defaultqueue, email, created)" .
           " values ($uname, $pword, $default_user_rights," .
           " $default_queue, $email, NOW())";
    if (not $link->do($sql)) {
        report_error("can't add user: $link->errstr");
    } else {
        report_success("Added user.");
    }

    return(1);
};


$commands{'LOCKUSER'} = sub {
    my $uid = shift;
    if ((not defined $uid) or ($uid !~ /\A\d+\Z/)) {
        report_error('argument must be number.');
        return(1);
    }

    report_error("Can't lock anonymous account"), return 1 if ($uid == 0);

    if ($uid != $auth_uid) {
        if (!($current_global_rights & canLockUsers)) {
            report_error("You don't have permission to do that.");
            return(1);
        }
    }

    my $link = get_database_link();
    my $x = canNotAuthorize;   # needed for interpolation.
    my $sql = "update $dbtable_users" .
              " set globalrights=(globalrights|$x)" .
              " where id=$uid";

    if (not defined $link->do($sql)) {
        report_error("can't lock user: $link->errstr");
    } else {
        report_success("Locked user id $uid.");
        if ($uid == $auth_uid) {
            $auth_uid = 0;
            $current_global_rights = $current_queue_rights = 0;
        }
    }

    return(1);
};


$commands{'NOOP'} = sub {
    report_success('epson the whole astroda');
    return(1);
};


$commands{'CREATEQUEUE'} = sub {
    # !!! FIXME: Implement this.
    my $args = shift;
    report_error('Not yet implemented');
    return(1);
};


$commands{'LOCKQUEUE'} = sub {
    # !!! FIXME: Implement this.
    my $args = shift;
    report_error('Not yet implemented.');
    return(1);
};


$commands{'HIDEQUEUE'} = sub {
    # !!! FIXME: Implement this.
    my $args = shift;
    report_error('Not yet implemented.');
    return(1);
};


$commands{'GETRDF'} = sub {
    my $max = shift;  # it's okay for this to be undef'd.
    report_error('no queue selected.'), return 1 if $queue == 0;
    my ($err, $rdf, $filename) = generate_rdf($queue, $max);
    report_error($err), return 1 if (defined $err);
    report_success('here comes.');
    print("$rdf\012.\012");
    return(1);
};


$commands{'FORGOTPASSWORD'} = sub
{
    my $args = shift;
    my ($user, $email) = (undef, undef);
    if (defined $args) {
        ($user, $email) = $args =~ /\A\"(.+?)\"\s*\"(.+?)\"\Z/
    }

    if ((not defined $user) or (not defined $email)) {
	report_error('USAGE: FORGOTPASSWORD <"user"> <"email">');
        return(1);
    }

    my $pword = generate_pword();
    my $err = undef;
    my $link = get_database_link();
    my $u = $link->quote($user);
    my $e = $link->quote($email);
    my $sql = "select id from $dbtable_users" .
	      " where email like $e and name like $u";
    my $sth = $link->prepare($sql);
    if (not $sth->execute()) {
        report_error("can't execute the query: $sth->errstr");
        return(1);
    }
    my @row = $sth->fetchrow_array();
    $sth->finish();
    $err = "Can't find a matching user/email." if (not @row);
    $err = send_forgotten_password($user, $email, $pword) if (not defined $err);
    if (defined $err) {
        $err .= " Please talk to $admin_email for further assistance.";
        report_error($err);
    } else {
	my $id = $row[0];
	#print("id is [$id], pword is [$pword].\n");
	my $cryptedpw = $link->quote(crypt($pword, new_crypt_salt()));
	$sql = "update $dbtable_users set password=$cryptedpw where id=$id";
	$link->do($sql);  # This has to work; we already sent the email. (*shrug*)
        report_success("Helpful Phriendly info sent to $email ...");
    }

    return(1);
};


$commands{'CHANGEPASSWORD'} = sub {
    my $newpass = shift;
    if (not defined $newpass) {
        report_error("Usage: CHANGEPASSWORD <newpass>");
        return(1);
    }

    report_error("Please log in first"), return 1 if ($auth_uid == 0);

    my $link = get_database_link();
    $newpass = $link->quote(crypt($newpass, new_crypt_salt()));
    my $sql = "update $dbtable_users set password=$newpass where id=$auth_uid";
    if (not defined $link->do($sql)) {
        report_error("can't change password: $link->errstr");
    } else {
        report_success("password changed.");
    }

    return(1);
};


$commands{'SETDEFAULTQUEUE'} = sub {
    my $qid = shift;
    if ((not defined $qid) or ($qid !~ /\A\d+\Z/)) {
        report_error('argument must be number.');
        return(1);
    }

    report_error("Please log in first"), return 1 if ($auth_uid == 0);

    if ($qid != 0) {
        my $sql = "select id from $dbtable_queues where id=$qid";
        my $sth = $link->prepare($sql);
        if (not $sth->execute()) {
            report_error("can't execute the query: $sth->errstr");
            return(1);
        }
        my @row = $sth->fetchrow_array();
        $sth->finish();
        if (not @row) {
            report_error("No such queue.");
            return(1);
        }
    }

    # !!! FIXME: Make sure user can select this queue before changing default!

    my $sql = "update $dbtable_users set defaultqueue=$qid where id=$auth_uid";
    if (not defined $link->do($sql)) {
        report_error("Failed to set default queue: $link->errstr");
    } else {
        report_success("default queue changed.");
    }

    return(1);
};


$commands{'QUIT'} = sub {
    report_success('kthxbye');
    return(0);  # break command processing loop.
};


$commands{'EXIT'} = $commands{'QUIT'};  # just an alias.
$commands{'Q'} = $commands{'QUIT'};     # just another alias.


sub news_mainline {
    autoflush STDOUT, 1;

    if (not defined $ENV{'TCPREMOTEIP'}) {
        report_fatal('TCPREMOTEIP is not set. Daemon is misconfigured. Aborting.');
    }

    $ipaddr = unpack('N', inet_aton($ENV{'TCPREMOTEIP'}));

    report_success("IcculusNews daemon $version");
    process_command('AUTH') while (not defined $auth_uid);
    1 while (process_command());
    $link->disconnect() if defined $link;
    return 0;
}


sub syslog_and_die {
    my $err = shift;
    do_log(syslogError, $err);
    die($err);
}


sub go_to_background {
    use POSIX 'setsid';
    chdir('/') or syslog_and_die("Can't chdir to '/': $!");
    open STDIN,'/dev/null' or syslog_and_die("Can't read '/dev/null': $!");
    open STDOUT,'>/dev/null' or syslog_and_die("Can't write '/dev/null': $!");
    defined(my $pid=fork()) or syslog_and_die("Can't fork: $!");
    exit if $pid;
    setsid or syslog_and_die("Can't start new session: $!");
    open STDERR,'>&STDOUT' or syslog_and_die("Can't duplicate stdout: $!");
    do_log(syslogDaemon, "Daemon process is now detached");
}


sub drop_privileges {
    delete @ENV{qw(IFS CDPATH ENV BASH_ENV)};
    $ENV{'PATH'} = $safe_path;
    $) = $wanted_gid if (defined $wanted_gid);
    $> = $wanted_uid if (defined $wanted_uid);
}

sub signal_catcher {
    my $sig = shift;
    do_log(syslogDaemon, "Got signal $sig. Shutting down.");
    exit 0;
}


my @kids;
use POSIX ":sys_wait_h";
sub reap_kids {
    my $i = 0;
    my $x = scalar(@kids);
    while ($i < scalar(@kids)) {
        my $rc = waitpid($kids[$i], &WNOHANG);
        if ($rc != 0) {  # reaped a zombie.
            splice(@kids, $i, 1); # take it out of the array.
        } else {  # still alive, try next one.
            $i++;
        }
    }

    $SIG{CHLD} = \&reap_kids;  # make sure this works on crappy SysV systems.
}



# Mainline.

if (defined $read_timeout) {
    use IO::Select;
}

foreach (@ARGV) {
    $daemonize = 1, next if $_ eq '--daemonize';
    $daemonize = 1, next if $_ eq '-d';
    $daemonize = 0, next if $_ eq '--no-daemonize';
    die("Unknown command line \"$_\".\n");
}

if ($use_syslog) {
    use Sys::Syslog qw(:DEFAULT setlogsock);
    setlogsock('unix');
    openlog('newsd', 'user') or report_fatal("Couldn't open syslog: $!");
}


my $retval = 0;
if (not $daemonize) {
    drop_privileges();
    $retval = news_mainline();
} else {
    do_log(syslogDaemon, "IcculusNews daemon $version starting up...");
    go_to_background();

    $SIG{CHLD} = \&reap_kids;
    $SIG{TERM} = \&signal_catcher;
    $SIG{INT} = \&signal_catcher;

    use IO::Socket::INET;
    my $listensock = IO::Socket::INET->new(LocalPort => $server_port,
                                           Type => SOCK_STREAM,
                                           ReuseAddr => 1,
                                           Listen => $max_connects);

    syslog_and_die("couldn't create listen socket: $!") if (not $listensock);

    my $selection = new IO::Select( $listensock );
    drop_privileges();

    do_log(syslogDaemon, "Now accepting connections (max $max_connects" .
                         " simultaneous on port $server_port).");

    while (1)
    {
        # prevent connection floods.
        sleep(1) while (scalar(@kids) >= $max_connects);

        # if timed out, do upkeep and try again.
        1 while not $selection->can_read(999999);

        # we've got a connection!
        my $client = $listensock->accept();
        if (not $client) {
            syslog("info", "accept() failed: $!") if ($use_syslog);
            next;
        }

        my $ip = $client->peerhost();
        syslog("info", "connection from $ip") if ($use_syslog);

        $ENV{'TCPREMOTEIP'} = $ip;

        my $kidpid = fork();
        syslog_and_die("fork() failed: $!") if (not defined $kidpid);

        if ($kidpid) {  # this is the parent process.
            close($client);  # parent has no use for client socket.
            push @kids, $kidpid;
        } else {
            close($listensock);   # child has no use for listen socket.
            local *FH = $client;
            open(STDIN, "<&FH") or syslog_and_die("no STDIN reassign: $!");
            open(STDERR, ">&FH") or syslog_and_die("no STDERR reassign: $!");
            open(STDOUT, ">&FH") or syslog_and_die("no STDOUT reassign: $!");
            my $retval = news_mainline();
            close($client);
            exit $retval;  # kill child.
        }
    }

    close($listensock);  # shouldn't ever hit this.
}

exit $retval;

# end of IcculusNews_daemon.pl ...

