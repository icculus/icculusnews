<?php
session_name('IcculusNews');
session_start();

require './IcculusNews.php';

$daemon_host = 'localhost';
$daemon_port = 263;

function output_site_header()
{
echo <<< EOF

    <p align="center">
      <font size="+3">icculus.org</font><br>
      <font size="-3">
        <a href="http://cvs.icculus.org/">[cvs]</a>
          -
        <a href="https://bugzilla.icculus.org/">[bugzilla]</a>
          -
        <a href="https://www.icculus.org/">[ssl]</a>
          -
        <a href="https://mail.icculus.org/">[mail]</a>
      </font>
    </p>

    <hr>
EOF;
} // output_site_header


function output_site_footer()
{
echo <<< EOF

    <hr>

    <p align="center">
      <font size="-3">
        [
        <a href="mailto:newsmaster@icculus.org">email newsmaster</a>
        |
        <a href="/news/news_icculus_org.rdf">get news rdf</a>
        ]
      </font>
    </p>

    <p align="center">
      <a href="http://icculus.org/">
        <img src="/icculus-org.png" alt="icculus.org" border="0">
      </a>

      <a href="http://icculus.org/">
        <img src="/icculus-org-now.png" alt="icculus.org NOW!" border="0">
      </a>
    </p>
EOF;
} // output_site_footer



function preview_news_item($postdate, $title, $text, $author)
{
    print("<ul><li><b>$title</b>\n");
    print(" <font size=\"-3\">(posted $postdate\n");
    print(" by <i>$author</i>)</font>");
    print(":<br>\n$text<br>\n--$author.</ul>\n");
} // preview_news_item


function current_sql_datetime()
{
    $t = localtime(time(), true);
    return( "" . substr('0000' . ($t['tm_year'] + 1900), -4) . '-' .
                 substr('00' . ($t['tm_mon'] + 1), -2) . '-' .
                 substr('00' . ($t['tm_mday']), -2) . ' ' .
                 substr('00' . ($t['tm_hour']), -2) . ':' .
                 substr('00' . ($t['tm_min']), -2) . ':' .
                 substr('00' . ($t['tm_sec']), -2) );
} // current_sql_datetime


function is_logged_in(&$uname, &$pass, &$queue)
{
    $uname = $pass = $queue = NULL;

    if (!session_is_registered('iccnews_username'))
        return(false);

    if (!isset($GLOBALS['HTTP_SESSION_VARS']['iccnews_username']))
        return(false);

    if ($GLOBALS['HTTP_SESSION_VARS']['iccnews_username'] == false)
        return(false);

    if (!session_is_registered('iccnews_userpass'))
        return(false);

    if (!isset($GLOBALS['HTTP_SESSION_VARS']['iccnews_userpass']))
        return(false);

    if ($GLOBALS['HTTP_SESSION_VARS']['iccnews_userpass'] == false)
        return(false);

    if (!session_is_registered('iccnews_userqueue'))
        return(false);

    if (!isset($GLOBALS['HTTP_SESSION_VARS']['iccnews_userqueue']))
        return(false);

    if ($GLOBALS['HTTP_SESSION_VARS']['iccnews_userqueue'] == false)
        return(false);

    $uname = $GLOBALS['HTTP_SESSION_VARS']['iccnews_username'];
    $pass = $GLOBALS['HTTP_SESSION_VARS']['iccnews_userpass'];
    $queue = $GLOBALS['HTTP_SESSION_VARS']['iccnews_userqueue'];

    return(true);
} // is_logged_in


function get_connected(&$sock)
{
    global $daemon_host, $daemon_port;

    $err = NULL;
    $sock = NULL;

    if (!is_logged_in($user, $pass, $queue))
        $err = "You aren't logged in.";
    else
    {
        if ($err = news_login($sock, $daemon_host, $daemon_port, $user, $pass))
            $err = "Login failed: $err.";
        else
        {
            if ($err = news_change_queue($sock, $queue))
                $err = "Failed to select queue: $err.";
        } // else
    } // else

    return($err);
} // get_connected


function output_queue_rows($sock, $showall = 0)
{
    global $PHP_SELF;

    // create some local variables.
    $err = $query = NULL;

    if (!isset($sock))
        $err = "Didn't connect to daemon at all";
    else
    {
        // !!! FIXME: Grab these in chunks?
        if ($err = news_digest($sock, $query, 9999999))
            $err = "Failed to get digest: $err.";
    } // else

    if ($err)
    {
        print('<tr><td align="center" colspan="5"><font color="#FF0000">');
        print("$err</font></td></tr>\n");
        return;
    } // if

    $item_count = $approved = $pending = $deleted = 0;

    foreach ($query as $item)
    {
        if ((!$showall) && ($item['approved']))
            continue;

        $item_count++;

        $item['author'] = htmlentities($item['author'], ENT_QUOTES);
        $item['title'] = htmlentities($item['title'], ENT_QUOTES);

        $tags = $endtags = '';
        if ($item['deleted'])
        {
            $tags    .= '<strike>';
            $endtags  = '</strike>' . $endtags;
            $deleted++;
        } // if

        if ($item['approved'])
        {
            $tags    .= '<i>';
            $endtags  = '</i>' . $endtags;
            $approved++;
        } // if
        else
        {
            $pending++;
        } // else

        $ip = $item['ipaddr'];

        print("<tr>\n");
        print('<td align="center"> <input type="checkbox" name="itemid[]"');
        print(" value=\"{$item['id']}\"></td>\n");

        print("<td align=\"center\"> $tags {$item['postdate']} $endtags </td>\n");

        print("<td align=\"center\"> $tags");
        print(" <a href=\"{$PHP_SELF}?action=post&editid={$item['id']}\">");
        print("{$item['title']} $endtags </a> </td>\n");

        print("<td align=\"center\"> $tags {$item['author']} $endtags </td>\n");
        print("<td align=\"center\"> $tags $ip $endtags </td>\n");
        print("</tr>\n");
    } // while

    print('<tr><td align="center" colspan="5"><font color="#0000FF">');
    print("$item_count items listed, $deleted deleted, $approved approved, $pending pending.</font></td></tr>\n");
} // output_queue_rows


function output_news_queue_widgets($showall = 0)
{
    global $PHP_SELF;

    is_logged_in($u, $p, $q);

    if (isset($showall) == false)
        $showall = 0;

    $showallflip = 1;
    $showalltext = "Show all items";
    if ($showall)
    {
        $showallflip = 0;
        $showalltext = "Show only pending items";
    } // if

    $queues = NULL;
    $queuelist = '';
    $err = get_connected($sock);

    if (!isset($err))
        $err = news_queueinfo($sock, $q, $qinfo);

    // !!! FIXME: an "htmlentities_whole_hashtable()" would be nice.
    $qinfo['name'] = htmlentities($qinfo['name'], ENT_QUOTES);
    $qinfo['desc'] = htmlentities($qinfo['desc'], ENT_QUOTES);
    $qinfo['ownername'] = htmlentities($qinfo['ownername'], ENT_QUOTES);

// !!! FIXME: Use some of this?
//   $title     = $qinfo['name'];        // string of queue's name
//   $desc      = $qinfo['desc'];        // string of queue's description.
//   $rdfurl    = $qinfo['rdfurl'];      // URL of queue's RDF file.
//   $created   = $qinfo['created'];     // string of time/date of creation.
//   $ownerid   = $qinfo['ownerid'];     // owner's ID number.
//   $ownername  = $qinfo['owner'];      // string of owner's name.

    if (!isset($err))
        $err = news_enum_queues($sock, $queues);

    if (!isset($err))
    {
        if (count($queues) == 1)
        {
            foreach ($queues as $qid => $qname)
            {
                $qname = htmlentities($qname, ENT_QUOTES);
                $queuelist = "Queue: <i>$qname</i>";
            } // foreach
        } // if

        else if (count($queues) > 1)
        {
            $queuelist  = '<form method="post"';
            $queuelist .= "action=\"$PHP_SELF?action=view&showall=$showall\">";
            $queuelist .= "\n";
            $queuelist .= 'Queue: <select name="form_qid" size="1">';
            $queuelist .= "\n";

            foreach ($queues as $qid => $qname)
            {
                $sel = (($qid == $q) ? 'selected' : '');
                $qname = htmlentities($qname, ENT_QUOTES);
                $queuelist .= "<option $sel value=\"$qid\">$qname</option>\n";
            } // foreach

            $queuelist .= "</select>\n";
            $queuelist .= '<input type="submit" name="form_chqid" value="Change">';
            $queuelist .= "\n</form>\n";
        } // else if
    } // if

echo <<< EOF

    <center>
      <table border="0" width="100%">
        <tr>
          <td align="left">
            $queuelist
          </td>
          <td align="right">
            [
            <a href="$PHP_SELF?action=view&showall=$showallflip">$showalltext</a>
            |
            <a href="$PHP_SELF?action=logout">Log out</a>
            ]
          </td>
        </tr>
      <table>

      <form method="post" action="$PHP_SELF?action=view&showall=$showall">
      <table border="1" width="100%">
        <tr>

          <script language="javascript">
          <!--
              function selectAll(formObj)
              {
                  var checkval = false;
                  var i;

                  for (i = 0; i < formObj.length; i++)
                  {
                      var fldObj = formObj.elements[i];
                      if ((fldObj.type == 'checkbox') &&
                          (fldObj.name == 'checkeverything'))
                      {
                          checkval = (fldObj.checked) ? true : false;
                          break;
                      }
                  }

                  if (i == formObj.length)  // ???
                      return;

                  for (i = 0; i < formObj.length; i++)
                  {
                      var fldObj = formObj.elements[i];
                      if (fldObj.type == 'checkbox')
                          fldObj.checked = checkval;
                  }
              }
          //-->
          </script>

          <td align="center">
            <script language="javascript">
            <!--
              document.write('<input type="checkbox" name="checkeverything"');
              document.write(' value="0" onClick="selectAll(this.form);">');
            //-->
            </script>
            <noscript>X</noscript>
          </td>

          <td align="center"> date </td>
          <td align="center"> title </td>
          <td align="center"> author </td>
          <td align="center"> ip addr </td>
        </tr>

EOF;

    output_queue_rows($sock, $showall);

echo <<< EOF

        <tr>
          <td align="center" colspan="5">
            <input type="submit" name="form_refresh"   value="Refresh">
            <input type="submit" name="form_delete"    value="Delete">
            <input type="submit" name="form_undelete"  value="Undelete">
            <input type="submit" name="form_approve"   value="Approve">
            <input type="submit" name="form_unapprove" value="Unapprove">
            <input type="submit" name="form_purge"     value="Purge Selected">
            <input type="submit" name="form_purgeall"  value="Purge All">
          </td>
        </tr>
      </table>
      </form>

      <br>

      <table width="75%">
        <tr>
          <td align="center"><a href="$PHP_SELF?action=post">Add new items.</a></td>
          <td align="center"><a href="{$qinfo['itemarchiveurl']}">View news archive.</a></td>
          <td align="center"><a href="{$qinfo['url']}">View front page.</a></td>
        </tr>
      </table>
    </center>
EOF;

    news_logout($sock);
} // output_news_queue_widgets


function handle_news_queue_commands()
{
    global $itemid, $form_delete, $form_undelete, $form_purge, $form_purgeall;
    global $form_approve, $form_unapprove, $form_chqid, $form_qid;
    global $iccnews_userqueue;

    // create some local variables.
    $sock = NULL;
    $err = NULL;

    if ( ($form_chqid) and (isset($form_qid)) )
    {
        if (is_logged_in($u, $p, $q))
            $iccnews_userqueue = $form_qid;
    } // if

    else if ($form_delete)
    {
        $err = get_connected($sock);

        foreach ($itemid as $id)
        {
            if ( (!isset($sock)) || ($err = news_delete($sock, $id)) )
            {
                echo "<font color=\"#FF0000\">failed to delete item $id.";
                echo " $err</font><br>\n";
            } // if
        } // foreach
    } // if

    else if ($form_undelete)
    {
        $err = get_connected($sock);

        foreach ($itemid as $id)
        {
            if ( (!isset($sock)) || ($err = news_undelete($sock, $id)) )
            {
                echo "<font color=\"#FF0000\">failed to undelete item $id.";
                echo " $err</font><br>\n";
            } // if
        } // foreach
    } // if

    else if ($form_purge)
    {
        $err = get_connected($sock);

        foreach ($itemid as $id)
        {
            if ( (!isset($sock)) || ($err = news_purge($sock, $id)) )
            {
                echo "<font color=\"#FF0000\">failed to purge item $id.";
                echo " $err</font><br>\n";
            } // if
        } // for
    } // else if

    else if ($form_purgeall)
    {
        $err = get_connected($sock);
        if ( ($err) || ($err = news_purgeall($sock)) )
        {
            echo "<font color=\"#FF0000\">failed to purge items.";
            echo " $err</font><br>\n";
        } // if
    } // else if

    else if ($form_approve)
    {
        $err = get_connected($sock);

        foreach ($itemid as $id)
        {
            if ( (!isset($sock)) || ($err = news_approve($sock, $id)) )
            {
                echo "<font color=\"#FF0000\">failed to approve item $id.";
                echo " $err</font><br>\n";
            } // if
        } // for
    } // else if

    else if ($form_unapprove)
    {
        $err = get_connected($sock);

        foreach ($itemid as $id)
        {
            if ( (!isset($sock)) || ($err = news_unapprove($sock, $id)) )
            {
                echo "<font color=\"#FF0000\">failed to unapprove item $id.";
                echo " $err</font><br>\n";
            } // if
        } // for
    } // else if

    news_logout($sock);
} // handle_news_queue_commands


function output_login_widgets($next_action = 'view')
{
    global $SERVER_NAME, $REQUEST_URI, $PHP_SELF;
    global $form_user, $form_next;

    if (!isset($GLOBALS['HTTPS']))
    {
         $href = "https://${SERVER_NAME}${REQUEST_URI}";
         print "<p align=\"center\">You need a secure connection to log in.</p>\n";
         print "<p align=\"center\">Try <a href=\"$href\">here</a>.</p>\n";
         return;
    } // if

    echo <<<EOF

    <script language="javascript">
    <!--
        function trim(inputString)
        {
            var retValue = inputString;
            var ch = retValue.substring(0, 1);
            while (ch == " ")
            {
                retValue = retValue.substring(1, retValue.length);
                ch = retValue.substring(0, 1);
            } // while

            ch = retValue.substring(retValue.length-1, retValue.length);
            while (ch == " ")
            {
                retValue = retValue.substring(0, retValue.length-1);
                ch = retValue.substring(retValue.length-1, retValue.length);
            } // while

            return retValue;
        } // trim


        function check_login_fields()
        {
            if (trim(document.loginform.form_user.value) == "")
            {
                alert("Please enter a username.");
                return(false);
            } // if

            if (trim(document.loginform.form_pass.value) == "")
            {
                alert("Please enter a password.");
                return(false);
            } // if

            return(true);
        } // check_login_fields
    // -->
    </script>

    <center>
      <form name="loginform" method="post"
            onsubmit="return check_login_fields();"
            action="$PHP_SELF?action=login">
        <input type="hidden" name="form_next" value="$next_action">
        <table border="1">
          <tr>
            <td>My username:</td>
            <td><input type="text" name="form_user" value="$form_user"></td>
          </tr>
          <tr>
            <td>My password:</td>
            <td><input type="password" name="form_pass" value=""></td>
          </tr>
          <tr>
            <td align="center" colspan="2">
              <table width="100%">
                <tr>
                  <td align="left" width="25%">
                    <font size="-2">
                      <a href="$PHP_SELF?action=newuser">New user</a>
                    </font>
                  </td>
                  <td align="center" width="50%">
                    <input type="submit" name="form_login" value="Login">
                  </td>
                  <td align="right" width="25%">
                    <font size="-2">
                      <a href="$PHP_SELF?action=forgotpw">Forgot password</a>
                    </font>
                  </td>
                </tr>
              </table>
            </td>
          <tr>
        </table>
      </form>
    </center>

EOF;
} // output_login_widgets


function output_news_edit_widgets($item, $queues, $chosen_queue, $allow_submit)
{
    global $PHP_SELF;
    global $form_postdate, $form_postanon;

    $unsafe_text = $item['text'];
    $unsafe_title = $item['title'];

    $item['title'] = htmlentities($item['title'], ENT_QUOTES);
    $item['text'] = htmlentities($item['text'], ENT_QUOTES);
    $item['author'] = htmlentities($item['author'], ENT_QUOTES);

    $idarg = (isset($item['id'])) ? "&editid={$item['id']}" : '';
    $submit_button = '';
    if ($allow_submit)
        $submit_button = '<input type="submit" name="form_submit" value="Submit">';

    $postanon_form = '';
    $cancel_button = '';
    if (is_logged_in($u, $p, $q))
    {
        $checked = (($form_postanon) ? 'checked' : '');
        $postanon_form = (isset($item['id'])) ?
            '' :
            "<tr><td></td><td align=\"left\"><input type=\"checkbox\" $checked name=\"form_postanon\" value=\"1\">Post anonymously</td></tr>";

        $cancel_button = '<input type="submit" name="form_cancel" value="Cancel">';
    } // if

    $newlogin_form = (isset($item['id'])) ?
        '' :
        "<a href=\"$PHP_SELF?action=login\"><font size=\"-2\">(Log in as someone else)</font></a>";

    $queue_form = '';
    if (count($queues) == 1)
    {
        foreach ($queues as $qid => $qname)
        {
            $qname = htmlentities($qname, ENT_QUOTES);
            $queue_form = "$qname <input type=\"hidden\" name=\"form_qid\" value=\"$qid\">";
        } // foreach
    } // if

    else if (count($queues) > 1)
    {
        $queue_form = '<select name="form_qid" size="1">';
        foreach ($queues as $qid => $qname)
        {
            $sel = (($qid == $chosen_queue) ? 'selected' : '');
            $qname = htmlentities($qname, ENT_QUOTES);
            $queue_form .= "<option $sel value=\"$qid\">$qname</option>";
        } // foreach
        $queue_form .= '</select>';
    } // else if

    // if we're editing an existing item, show the rendered version first time.
    if ( (!isset($form_postdate)) and (isset($item['id'])) )
    {
        // !!! FIXME: Generalize anonymous name.
        $u = (($form_postanon) ? 'anonymous hoser' : $item['author']);
        preview_news_item($item['postdate'], $unsafe_title,
                          $unsafe_text, htmlentities($u, ENT_QUOTES));
        printf("\n<hr>\n\n");
    } // if

    echo <<< EOF

    <form method="post" action="$PHP_SELF?action=post$idarg">
      <input type="hidden" name="form_fromuser" value="{$item['author']}">
      <input type="hidden" name="form_postdate" value="{$item['postdate']}">
      <table>
        <tr>
          <td align="left">Posted by:</td>
          <td align="left">
            {$item['author']}
            $newlogin_form
          </td>
        </tr>
        $postanon_form
        <tr>
          <td align="left">Queue:</td>
          <td align="left">$queue_form</td>
        </tr>
        <tr>
          <td>News title:</td>
          <td>
            <input type="text" size="60" name="form_title" value="{$item['title']}">
          </td>
        </tr>

        <tr>
          <td>News text:</td>
          <td>
            <textarea rows="10" name="form_text" cols="60">{$item['text']}</textarea>
          </td>
        </tr>

        <tr>
          <td colspan="2" align="center">
            <input type="submit" name="form_preview" value="Preview">
            $submit_button
            $cancel_button
          </td>
        </tr>
      </table>
    </form>

EOF;
} // output_news_edit_widgets


function handle_news_edit_commands()
{
    global $daemon_host, $daemon_port;
    global $editid, $form_preview, $form_submit, $form_fromuser;
    global $form_title, $form_text, $form_qid, $form_postanon, $form_cancel;
    global $form_postdate;

    if (!isset($form_fromuser))  // hasn't gone through user yet.
        return(true);

    if ($form_cancel)
    {
        output_news_queue_widgets();
        return(false);
    } // if

    $form_text     = ltrim(rtrim($form_text));
    $form_title    = ltrim(rtrim($form_title));

    if (strlen($form_title) > 64)
        $form_title = substr($form_title, 0, 64);

    if ( (($form_preview) or ($form_submit)) and
         (($form_fromuser == '') or ($form_text == '') or ($form_title == '')) )
    {
        print("<center><font color=\"#FF0000\">");
        print("Please enter all fields. kthxbye.");
        print("</font></center>\n");
        print("<hr>\n");
        $form_submit = false;
        $form_preview = false;
        return(true);
    } // if

    if ($form_submit)
    {
        $u = $p = $q = $sock = $err = NULL;
        if (!$form_postanon)
            is_logged_in($u, $p, $q);
        $q = $form_qid;
        if ($err = news_login($sock, $daemon_host, $daemon_port, $u, $p, $q))
            $err = "Login failed: $err";
        else
        {
            if (isset($editid))
                $err = news_edit($sock, $editid, $form_title, $form_text);
            else
                $err = news_post($sock, $form_title, $form_text);
        } // else
        news_logout($sock);

        if ($err)
        {
            $form_submit = false;
            print("\n<center>\n");
            print("<font color=\"#FF0000\">ERROR SUBMITTING NEWS!</font><br>\n");
            print("$err\n<br>\n");
            print("Please try submitting again later. Sorry.\n</center><hr>\n");
        } // if

        else
        {
            // !!! FIXME: "Ryan" should be generalized (use QUEUEINFO?)
            print("\n<center>\n");
            print("<font color=\"#0000FF\">Thanks for the news!</font><br>\n");
            print("It will likely be posted after Ryan has poked, prodded,\n");
            print(" and probably rewritten it.\n</center>\n");
        } // else
    } // if

    if (($form_submit) || ($form_preview))
    {
        $grammar = (($form_submit) ? "looked" : "looks");
        print("<center>\n");
        print("Your submission $grammar like this:\n");
        if ($form_preview)
        {
            print("<br><i>(When satisfied, hit the \"Submit\" button to");
            print(" stick it in the submission queue and go!)</i>\n");
        } // if

        else
        {
            printf("<br><i>(If you don't like it,");
            printf(" you should have used the Preview button!)</i>\n");
        } // else

        // !!! FIXME: Generalize this.
        $u = (($form_postanon) ? 'anonymous hoser' : $form_fromuser);
        print("</center>\n");
        print("<hr>\n\n");
        preview_news_item($form_postdate, $form_title, $form_text, $u);
        print("<hr>\n\n");

        if ($form_submit)
        {
            $wantview = isset($editid);
            $form_text  = '';
            $form_title = '';
            $form_fromuser = $form_preview = $form_submit = NULL;
            $form_postdate = NULL;
            $editid = NULL;

            if ($wantview)
            {
                output_news_queue_widgets();
                return(false);
            } // if
        } // if
    } // if

    return(true);
} // handle_news_edit_commands



function do_login($next_action = 'view')
{
    // !!! FIXME: tweak get_connected() so we don't need these globals here.
    global $daemon_host, $daemon_port;

    global $iccnews_username, $iccnews_userpass, $iccnews_userqueue;
    global $form_user, $form_pass, $form_next;
    global $actions;

    if (!session_is_registered('iccnews_username'))
    {
        session_register('iccnews_username');
        $iccnews_username = false;
    } // if

    if (!session_is_registered('iccnews_userpass'))
    {
        session_register('iccnews_userpass');
        $iccnews_userpass = false;
    } // if

    if (!session_is_registered('iccnews_userqueue'))
    {
        session_register('iccnews_userqueue');
        $iccnews_userqueue = false;
    } // if

    if ( (!isset($form_user)) || (!isset($form_pass)) )
        output_login_widgets($next_action);
    else
    {
        $err = false;
        if ( (trim($form_user) == '') || (trim($form_pass) == '') )
            $err = "Please enter all fields.";
        else
        {
            $sock = NULL;
            // !!! FIXME: tweak get_connected() so we can use it here.
            $err = news_login($sock, $daemon_host, $daemon_port,
                              $form_user, $form_pass);

            if (!$err)
                $err = news_get_userinfo($sock, $uid, $qid);

            news_logout($sock);
        } // else

        if ($err)
        {
            echo "<p align=\"center\"><font color=\"#FF0000\">Login failed: $err</font></p>\n";
            output_login_widgets($next_action);
        } // if
        else
        {
            $iccnews_username = $form_user;
            $iccnews_userpass = $form_pass;
            $iccnews_userqueue = $qid;
            if ( (isset($form_next)) && (isset($actions[$form_next])) )
                $actions[$form_next]();
        } // else
    } // else
} // do_login


function do_logout()
{
    global $PHP_SELF;
    global $iccnews_username, $iccnews_userpass, $iccnews_userqueue;

    $iccnews_username = $iccnews_userpass = $iccnews_userqueue = NULL;
    session_unset();
    session_destroy();

    echo <<<EOF

    <center>
      <p>You have been logged out.</p>
      <p>You can log back in <a href="$PHP_SELF?action=login">here</a>.
    </center>

EOF;
} // do_logout


function do_whoami()
{
    if (is_logged_in($u, $p))
    {
        $username = "<font color=\"#0000FF\">$u</font>";
    } // if
    else
    {
        $username = "<font color=\"#FF0000\">Not logged in</font>";
    } // else

   echo "<p align=\"center\">You are: $username.</p>\n";
} // do_whoami


function do_view()
{
    global $showall;

    if (!is_logged_in($u, $p, $q))
    {
        do_login('view');
        return;
    } // if

    handle_news_queue_commands();
    output_news_queue_widgets($showall);
} // do_view


function do_post()
{
    global $editid, $form_preview, $form_fromuser, $form_title;
    global $form_text, $form_postdate, $form_qid, $form_postanon;
    global $daemon_host, $daemon_port;

    if (handle_news_edit_commands() == false)
        return; // function specified alternate output; we're done.

    $u = $p = $q = $item = $sock = $err = NULL;
    $allow_submit = true;

    $logged_in = is_logged_in($u, $p, $q);
    $chosen_queue = (isset($form_qid)) ? $form_qid : $q;

    if (isset($form_fromuser))  // edited; pull from posted input.
    {
        $item = array();
        $item['author'] = $form_fromuser;
        $item['title'] = $form_title;
        $item['text'] = $form_text;
        $item['postdate'] = $form_postdate;

        if (isset($editid))
            $item['id'] = $editid;
    } // if

    else  // not edited; pull from the daemon, or use a blank slate.
    {
        if (isset($editid))  // this is really an edit command.
        {
            $err = get_connected($sock);
            if (!isset($err))
            {
                if ($err = news_get($sock, $editid, $item))
                    $err = "Failed to get item for editing: $err.";
            } // if
        } // if

        else  // blank slate.
        {
            $allow_submit = false;  // no submit without at least one preview.
            $item = array();
            $item['author'] = ($logged_in) ? $u : "anonymous hoser";   // !!! FIXME: Generalize this.
            $item['title'] = '';
            $item['text'] = '';
            $item['postdate'] = current_sql_datetime();

            // (the above 'postdate' isn't respected by the daemon;
            //  we just use it for user interface (pre-submit previews)).
        } // else
    } // else

    if (!isset($sock))  // might be connected to daemon already.
        $err = news_login($sock, $daemon_host, $daemon_port, $u, $p);

    $queues = NULL;
    if (!isset($err))
        $err = news_enum_queues($sock, $queues);

    // if this item is in a queue already, only list that specific queue.
    if (isset($item['id']))
    {
        $x = array();
        $x[$q] = $queues[$q];
        $queues = $x;
    } // if

    news_logout($sock);  // disconnect from daemon, if we used it.

    if ($err)
        echo "<center><font color=\"#FF0000\">Problem: $err</font></center>\n";
    else
    {
        output_news_edit_widgets($item, $queues, $chosen_queue,
                                 $allow_submit, isset($form_postanon));
    } // else
} // do_post


function do_forgotpw()
{
    echo <<<EOF

<center>
  <p><font color="#FF0000">Not implemented yet!</font></p>
  <p>Please <a href="mailto:icculus@clutteredmind.org">email Ryan</a> and
     he'll reset your password for you. Hopefully this will be automated
     soon.</p>
</center>

EOF;
} // do_forgotpw


function do_newuser()
{
    echo <<<EOF

<center>
  <p><font color="#FF0000">Not implemented yet!</font></p>
  <p>Please <a href="mailto:icculus@clutteredmind.org">email Ryan</a> and
     he'll create a new account for you. Hopefully this will be automated
     soon.</p>
</center>

EOF;
} // do_newuser


function do_createqueue()
{
    echo <<<EOF

<center>
  <p><font color="#FF0000">Not implemented yet!</font></p>
  <p>Please <a href="mailto:icculus@clutteredmind.org">email Ryan</a> and
     he'll create a new queue for you. Hopefully this will be automated
     soon.</p>
</center>

EOF;
} // do_createqueue


function do_unknown()
{
    echo "<p align=\"center\"><font color=\"#FF0000\">Unknown action.</font></p>\n";
    do_view();  // will go to login screen if not logged in.
} // do_unknown


// mainline/setup.


$actions = array();
$actions['login'] = do_login;
$actions['logout'] = do_logout;
$actions['whoami'] = do_whoami;
$actions['view'] = do_view;
$actions['post'] = do_post;
$actions['newuser'] = do_newuser;
$actions['forgotpw'] = do_forgotpw;
$actions['createqueue'] = do_createqueue;
$actions['unknown'] = do_unknown;

if (!isset($action))
{
    if (!is_logged_in($u, $p, $q))
        $action = 'login';
    else
        $action = 'view';
} // if

else if (!isset($actions[$action]))
{
    $action = 'unknown';
} // else if

?>


<html>
  <head>
    <title>IcculusNews</title>
  </head>

  <body>
    <?php output_site_header(); ?>
    <?php $actions[$action]();  ?>
    <?php output_site_footer(); ?>
  </body>
</html>

