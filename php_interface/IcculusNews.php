<?php

static $GDebugSocket = false;

function _fputs($sock, $str)
{
    global $GDebugSocket;
    if ($GDebugSocket) print("<pre>\nfputs:\n$str\n</pre>");
    return fputs($sock, $str);
}
function _fgets($sock, $len)
{
    global $GDebugSocket;
    $str = fgets($sock, $len);
    if ($GDebugSocket) print("<pre>\nfgets:\n$str\n</pre>");
    return $str;
}

function news_login(&$sock, $host, $port = 263, $uname = NULL,
                    $pass = NULL, $queue = NULL)
{
    $sock = fsockopen($host, $port);
    if ($sock == false)
    {
        $sock = NULL;  // Is there a difference?
        return("failed to connect to daemon at all");
    } // if

    $in = _fgets($sock, 4096);   // get welcome message.
    if ($in{0} != '+')
    {
        fclose($sock);
        $sock = NULL;
        return(substr($in, 2));
    } // if

    $authstr = 'AUTH ' . (isset($uname) ? "\"$uname\" \"$pass\"" : '-');
    _fputs($sock, "$authstr\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
    {
        news_logout($sock);
        return(substr($in, 2));
    } // if

    if (isset($queue))
    {
        $rc = news_change_queue($sock, $queue);
        if (isset($rc))
        {
            news_logout($sock);
            return($rc);
        } // if
    } // if

    return(NULL);  // no error. Socket is usable.
} // news_login


function news_logout(&$sock)
{
    if (isset($sock))
    {
        _fputs($sock, "QUIT\n");
        _fgets($sock, 4096);
        fclose($sock);
        $sock = NULL;
    } // if
} // news_logout


//
// returns an array of names:
//
//   foreach($retval as $queueid => $queuename)
//   {
//       print("queue number $queueid is named $queuename.\n");
//   } // foreach
//
function news_enum_queues($sock, &$queuearray)
{
    if (!isset($sock))
        return('bogus socket');

    _fputs($sock, "ENUM queues\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
        return(substr($in, 2));

    $queuearray = NULL;
    $retval = array();
    while (true)
    {
        $x = rtrim(_fgets($sock, 4096));
        if ($x == '.')
            break;  // done reading.

        $retval[$x] = rtrim(_fgets($sock, 4096));
        if (feof($sock))
            return("Unexpected EOF from news daemon");
    } // while

    $queuearray = $retval;
    return(NULL);  // no error.
} // news_enum_queues


function news_change_queue($sock, $queue)
{
    if (!isset($sock))
        return('bogus socket');
    else if (!isset($queue))
        return('bogus queue');

    _fputs($sock, "QUEUE $queue\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
        return(substr($in, 2));

    return(NULL);    // no error.
} // news_change_queue


function news_get_userinfo($sock, &$uid, &$qid)
{
    if (!isset($sock))
        return('bogus socket');

    $uid = $qid = NULL;

    _fputs($sock, "USERINFO\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
        return(substr($in, 2));

    $sep = strpos($in, ',');
    if ($sep === false)  // not found?!
        return("got unexpected result from daemon");

    $uid = trim(substr($in, 2, $sep - 2));
    $qid = trim(substr($in, $sep + 2));

    return(NULL);  // no error.
} // news_get_userinfo



//
// digestarray returns an array of hashtables:
//
//   foreach($retval as $x)
//   {
//      $id = $x['id'];              // ID number of news item.
//      $title = $x['title'];        // string of item's title.
//      $postdate = $x['postdate'];  // string of time/date of posting.
//      $authid = $x['authid'];      // id number of author.
//      $author = $x['author'];      // string of name of author.
//      $ipaddr = $x['ipaddr'];      // string of ip address of poster.
//      $approved = $x['approved'];  // 0==not approved, 1==approved.
//      $deleted = $x['deleted'];    // 0==not deleted, 1==deleted.
//   } // foreach
//
// Note that you'll get different results depending on who you are logged in
//  as...anonymous logins will only get approved, non-deleted items, for
//  example.
//
function news_digest($sock, &$digestarray, $startpoint = false, $maxitems = 5)
{
    if ($startpoint === false)
        $startpoint = '-';

    _fputs($sock, "DIGEST $startpoint $maxitems\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
        return(substr($in, 2));

    $retval = array();
    while (true)
    {
        $x = rtrim(_fgets($sock, 4096));
        if ($x == '.')
            break;  // done reading.

        $item = array();
        $item['id'] = $x;
        $item['title'] = rtrim(_fgets($sock, 4096));
        $item['postdate'] = rtrim(_fgets($sock, 4096));
        $item['authid'] = rtrim(_fgets($sock, 4096));
        $item['author'] = rtrim(_fgets($sock, 4096));
        $item['ipaddr'] = rtrim(_fgets($sock, 4096));
        $item['approved'] = rtrim(_fgets($sock, 4096));
        $item['deleted'] = rtrim(_fgets($sock, 4096));
        if (feof($sock))
            return("Unexpected EOF from news daemon");
        $retval[] = $item;
    } // while

    $digestarray = $retval;
    return(NULL);  // no error.
} // news_digest


//
// item returns a hashtable:
//
//   $id = $retval['id'];              // ID number of news item.
//   $title = $retval['title'];        // string of item's title.
//   $postdate = $retval['postdate'];  // string of time/date of posting.
//   $author = $retval['authid'];      // id number of author.
//   $name = $retval['author'];        // string of name of author.
//   $name = $retval['ipaddr'];        // string of ip address of poster.
//   $name = $retval['text'];          // string of item's complete text.
//   $name = $retval['approved'];      // 0==not approved, 1==approved.
//   $name = $retval['deleted'];       // 0==not deleted, 1==deleted.
//
function news_get($sock, $id, &$item)
{
    _fputs($sock, "GET $id\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
        return(substr($in, 2));

    $retval = array();
    $retval['id'] = $id;
    $retval['title'] = rtrim(_fgets($sock, 4096));
    $retval['postdate'] = rtrim(_fgets($sock, 4096));
    $retval['authid'] = rtrim(_fgets($sock, 4096));
    $retval['author'] = rtrim(_fgets($sock, 4096));
    $retval['ipaddr'] = rtrim(_fgets($sock, 4096));
    $retval['approved'] = rtrim(_fgets($sock, 4096));
    $retval['deleted'] = rtrim(_fgets($sock, 4096));
    $retval['text'] = '';

    while (true)
    {
        if (feof($sock))
            return("Unexpected EOF from news daemon");

        // !!! FIXME: line can overflow 4096.
        $x = rtrim(_fgets($sock, 4096));
        if ($x == '.')
            break;  // we're done.

        if ($x == '..')  // escaped dot.
            $x = '.';

        $retval['text'] .= "$x\n";
    } // while

    $item = $retval;
    return(NULL);  // no error.
} // news_get


//
// info returns a hashtable:
//
//   $title     = $retval['name'];        // string of queue's name
//   $desc      = $retval['desc'];        // string of queue's description.
//   $url       = $retval['url'];         // URL associated with queue.
//   $rdfurl    = $retval['rdfurl'];      // URL of queue's RDF file.
//   $arcurl    = $retval['itemarchiveurl']; // URL of news archives.
//   $itemurl   = $retval['itemviewurl']; // URL where items are viewed.
//   $created   = $retval['created'];     // string of time/date of creation.
//   $ownerid   = $retval['ownerid'];     // owner's ID number.
//   $ownername = $retval['owner'];       // string of owner's name.
//
function news_queueinfo($sock, $id, &$info)
{
    _fputs($sock, "QUEUEINFO $id\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
        return(substr($in, 2));

    $retval = array();
    $retval['name'] = rtrim(_fgets($sock, 4096));
    $retval['desc'] = rtrim(_fgets($sock, 4096));
    $retval['itemarchiveurl'] = rtrim(_fgets($sock, 4096));
    $retval['itemviewurl'] = rtrim(_fgets($sock, 4096));
    $retval['url'] = rtrim(_fgets($sock, 4096));
    $retval['rdfurl'] = rtrim(_fgets($sock, 4096));
    $retval['created'] = rtrim(_fgets($sock, 4096));
    $retval['ownerid'] = rtrim(_fgets($sock, 4096));
    $retval['owner'] = rtrim(_fgets($sock, 4096));

    $info = $retval;
    return(NULL);  // no error.
} // news_queueinfo


function news_post($sock, $title, $text)
{
    _fputs($sock, "POST $title\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
        return(substr($in, 2));

    _fputs($sock, "$text\n.\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
        return(substr($in, 2));

    return(NULL);  // no error.
} // news_post


function news_edit($sock, $id, $title, $text)
{
    _fputs($sock, "EDIT $id $title\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
        return(substr($in, 2));

    _fputs($sock, "$text\n.\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
        return(substr($in, 2));

    return(NULL);  // no error.
} // news_edit


function news_approve($sock, $id)
{
    _fputs($sock, "APPROVE $id\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
        return(substr($in, 2));

    return(NULL);  // no error.
} // news_approve


function news_unapprove($sock, $id)
{
    _fputs($sock, "UNAPPROVE $id\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
        return(substr($in, 2));

    return(NULL);  // no error.
} // news_unapprove


function news_delete($sock, $id)
{
    _fputs($sock, "DELETE $id\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
        return(substr($in, 2));

    return(NULL);  // no error.
} // news_delete


function news_undelete($sock, $id)
{
    _fputs($sock, "UNDELETE $id\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
        return(substr($in, 2));

    return(NULL);  // no error.
} // news_undelete


function news_purge($sock, $id)
{
    _fputs($sock, "PURGE $id\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
        return(substr($in, 2));

    return(NULL);  // no error.
} // news_purge


function news_purgeall($sock)
{
    _fputs($sock, "PURGEALL\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
        return(substr($in, 2));

    return(NULL);  // no error.
} // news_purgeall


// You need this to prevent the server from hanging up on you if you
//  planning on delaying for any amount of time. (the daemon's default
//  timeout is 15 seconds).
function news_noop($sock)
{
    _fputs($sock, "NOOP\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
        return(substr($in, 2));

    return(NULL);  // no error.
} // news_noop


// Be gentle: this is NOT robust.
//
// digestarray returns an array of hashtables:
//
//   foreach($retval as $x)
//   {
//      $title = $x['title'];        // string of item's title.
//      $link = $x['link'];          // string of name item's URL.
//      $id = $x['id'];              // ID number of news item.
//      $postdate = $x['postdate'];  // string of time/date of posting.
//      $name = $x['authid'];        // id number of author.
//      $name = $x['author'];        // string of name of author.
//      $name = $x['ipaddr'];        // string of ip address of poster.
//      $name = $x['approved'];      // 0==not approved, 1==approved.
//      $name = $x['deleted'];       // 0==not deleted, 1==deleted.
//   } // foreach
//
// 'approved' is always 1, and 'deleted' 0, under normal circumstances.
//
function news_parse_rdf($filename, &$digestarray, $max_items = 5)
{
    $in = fopen($filename, "r");
    if ($in == false)
        return("failed to open $filename");

    $retval = array();

    while ($count < $max_items)
    {
        while (strstr(_fgets($in, 4096), "<item rdf") == false)
        {
            if (feof($in))
                break;
        }

        if (feof($in))
            break;

        $item = array();
        $x = ltrim(rtrim(_fgets($in, 4096)));
        $x = str_replace('<title>', '', $x);
        $x = str_replace('</title>', '', $x);
        $item['title'] = $x;
        $x = ltrim(rtrim(_fgets($in, 4096)));
        $x = str_replace('<link>', '', $x);
        $x = str_replace('</link>', '', $x);
        $item['link'] = $x;
        $x = ltrim(rtrim(_fgets($in, 4096)));
        $x = str_replace('<author>', '', $x);
        $x = str_replace('</author>', '', $x);
        $item['author'] = $x;
        $x = ltrim(rtrim(_fgets($in, 4096)));
        $x = str_replace('<authorid>', '', $x);
        $x = str_replace('</authorid>', '', $x);
        $item['authid'] = $x;
        $x = ltrim(rtrim(_fgets($in, 4096)));
        $x = str_replace('<itemid>', '', $x);
        $x = str_replace('</itemid>', '', $x);
        $item['id'] = $x;
        $x = ltrim(rtrim(_fgets($in, 4096)));
        $x = str_replace('<postdate>', '', $x);
        $x = str_replace('</postdate>', '', $x);
        $item['postdate'] = $x;
        $x = ltrim(rtrim(_fgets($in, 4096)));
        $x = str_replace('<ipaddr>', '', $x);
        $x = str_replace('</ipaddr>', '', $x);
        $item['ipaddr'] = $x;
        $x = ltrim(rtrim(_fgets($in, 4096)));
        $x = str_replace('<approved>', '', $x);
        $x = str_replace('</approved>', '', $x);
        $item['approved'] = $x;
        $x = ltrim(rtrim(_fgets($in, 4096)));
        $x = str_replace('<deleted>', '', $x);
        $x = str_replace('</deleted>', '', $x);
        $item['deleted'] = $x;

        $retval[] = $item;
    }

    fclose($in);

    $digestarray = $retval;
    return(NULL);  // no error.
} // news_parse_rdf


function news_createuser($sock, $uname, $email, $pword)
{
    _fputs($sock, "CREATEUSER \"$uname\" \"$email\" \"$pword\"\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
        return(substr($in, 2));

    return(NULL);  // no error.
} // news_createuser


function news_changepassword($sock, $pword)
{
    _fputs($sock, "CHANGEPASSWORD $pword\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
        return(substr($in, 2));

    return(NULL);  // no error.
} // news_changepassword


function news_forgotpassword($sock, $user, $email)
{
    _fputs($sock, "FORGOTPASSWORD \"$user\" \"$email\"\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
        return(substr($in, 2));

    return(NULL);  // no error.
} // news_forgotpassword


function news_moveitem($sock, $itemid, $newqueueid)
{
    _fputs($sock, "MOVEITEM $itemid $newqueueid\n");
    $in = _fgets($sock, 4096);
    if ($in{0} != '+')
        return(substr($in, 2));

    return(NULL);  // no error.
} // news_moveitem

?>
