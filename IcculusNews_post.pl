#!/usr/bin/perl -w

use strict;
use Sys::Hostname;
use IO::Socket::INET;

my $version = "v2.0.0beta";

# The ever important debug-spew-enabler...
my $debug = 0;

#----------------------------------------------------------------------------#
# End of setup vars. The rest is probably okay without you touching it.      #
#----------------------------------------------------------------------------#

sub usage {
    print STDERR <<__EOF__;

IcculusNews_post $version
USAGE: $0 <host> <username> <queueid> <subject> <text>
  if <username> is '-', the Anonymous account is used.
   You will be prompted for a password if needed. You can
   avoid the prompt if you set the ICCNEWS_POST_PASS
   environment variable.
  if <text> is '-', news is read from stdin.

__EOF__

    exit 10;
}


sub okay_or_die {
    my ($sock, $sect) = @_;
    print "checking server response for: $sect\n" if $debug;
    my $response = <$sock>;
    chomp($response);
    print "server said: \"$response\".\n" if $debug;
    if ($response !~ /\A\+/) {
        print $sock "QUIT\012";  # just in case.
        close($sock);
        die("Error reported during $sect: \"$response\".\n");
    }
}


# the mainline.

my $user = undef;
my $host = undef;
my $pass = undef;
my $subj = undef;
my $text = undef;
my $qid = undef;

foreach (@ARGV) {
    usage() if ($_ eq '--help');
    $debug = 1, next if ($_ eq '--debug');
    $host = $_, next if (not defined $host);
    $user = $_, next if (not defined $user);
    $qid  = $_, next if (not defined $qid);
    $subj = $_, next if (not defined $subj);
    $text = $_, next if (not defined $text);
}

usage() if ((!defined $user) or (!defined $host) or (!defined $subj) or (!defined $text));

$text =~ s/\A\s*//;
$text =~ s/\s*\Z//;
if ($text eq '-') {
    $text = '';
    while (<STDIN>) {
        $text .= $_;
    }
}

$user =~ s/\A\s*//;
$user =~ s/\s*\Z//;

my $authstr = undef;
if ($user eq '-') {
    $authstr = '-';
} else {
    $pass = $ENV{'ICCNEWS_POST_PASS'};
    if (not defined $pass) {
        print("!!! FIXME: Prompt for a password here. export ICCNEWS_POST_PASS for now.\n");
        exit(42);
    }

    $authstr = "\"$user\" \"$pass\"";
}

1 while ($text =~ s/\015\012/\012/s);
1 while ($text =~ s/\015/\012/s);
$text =~ s/^\.\012/..\012/sm;
1 while ($text =~ s/\012.\012/\012..\012/s);

print "Connecting to news server..." if $debug;
my $sock = IO::Socket::INET->new(PeerAddr => $host,
                                 PeerPort => 263,
                                 Type     => SOCK_STREAM,
                                 Proto    => 'tcp');

die("Failed to connect to news server: $!\n") if not defined $sock;

print "connected.\n" if $debug;

okay_or_die($sock, 'initial welcome');
print $sock "AUTH $authstr\012";
okay_or_die($sock, 'user authorization');
print $sock "QUEUE $qid\012";
okay_or_die($sock, 'queue selection');
print $sock "POST $subj\012";
okay_or_die($sock, 'post command');
print $sock "$text\012.\012";
okay_or_die($sock, 'text posting');
print $sock "QUIT\012";
okay_or_die($sock, 'disconnection');
close($sock);

print("News item successfully posted.\n") if $debug;
exit 0;

# end of IcculusNews_post.pl ...

