#!/usr/bin/perl -w

use strict;
use warnings;

# The ever important debug-spew-enabler...
my $debug = 0;

# most of these can be altered at runtime with command line options.
my $newshost     = 'localhost';
my $newsauthor   = 'postmaster';
my $postingagent = '/usr/local/bin/IcculusNews_post.pl';
my $queuenum     = 1;
my $newspass     = undef;
my $newspassfile = '/etc/IcculusNews_mailgatewaypass.txt';
my $portnum      = 263;
my $newssubj     = undef;

#-----------------------------------------------------------------------------#
#     The rest is probably okay without you laying yer dirty mits on it.      #
#-----------------------------------------------------------------------------#

# the mainline.

if ((not defined $newspass) && (not defined $ENV{'ICCNEWS_POST_PASS'})) {
    if (defined $newspassfile) {
        open(FH, $newspassfile) or die("failed to open $newspassfile: $!\n");
        $newspass = <FH>;
        chomp($newspass);
        $newspass =~ s/\A\s*//;
        $newspass =~ s/\s*\Z//;
        close(FH);
    }
}


for (my $i = 0; $i < scalar(@ARGV); $i++) {
    my $arg = $ARGV[$i];
    $debug = 1, next if ($arg eq '--debug');
    $newshost = $ARGV[++$i], next if ($arg eq '--host');
    $queuenum = $ARGV[++$i], next if ($arg eq '--queue');
    $portnum = $ARGV[++$i], print("!!! FIXME: --port is ignored right now.\n"), next if ($arg eq '--port');
    $newsauthor = $ARGV[++$i], next if ($arg eq '--user');
    $newssubj = $ARGV[++$i], next if ($arg eq '--subj');
    $postingagent = $ARGV[++$i], next if ($arg eq '--agent');

    #$username = $arg, next if (not defined $username);
    #$subject = $arg, next if (not defined $subject);
    #$text = $arg, next if (not defined $text);
    #etc.

    print("Unknown argument \"$arg\".\n");
}

print("   reading from STDIN...\n") if $debug;
my $t = '';
$t .= $_ while (<STDIN>);

if (not defined $newssubj) {
    if ($t =~ /^Subject:\s?(.*?)$/m) {
        $newssubj = $1;
    }
}

$newssubj = "Posting to IcculusNews mail gateway" if (not defined $newssubj);

print("   posting to IcculusNews's submission queue...\n") if $debug;
$ENV{'ICCNEWS_POST_PASS'} = $newspass if defined $newspass;
my $rc = open(PH, "|'$postingagent' '$newshost' '$newsauthor' '$queuenum' '$newssubj' -");
if (not $rc) {
    print("   No pipe to IcculusNews: $!\n");
} else {
    print PH "<i>$ENV{'SENDER'} sent us the following email:" .
             "</i>\n<p>\n<pre>$t</pre>\n</p>\n";
    close(PH);
    print("   posted to submission queue.\n") if $debug;
}

exit 99;  # prevent further qmail delivery with successful status.

# end of IcculusNews_dotqmail.pl ...

