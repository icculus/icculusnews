<?php

/*
 (This comment is cut-and-pasted from Horde's install docs.
   http://www.horde.org/)

First of all, it is very important that you change the MySQL password for
the user 'root'. If you haven't done so already, type:

    mysqladmin [ -h <host> ] -u root -p password <new-password>

Login to MySQL by typing:

    mysql [ -h <host> ] -u root -p

Enter the password for root.

Now, create a database named "IcculusNews" and switch to it:

    mysql> create database IcculusNews;

    mysql> use IcculusNews;

Then set up the table called "news_items". You can actually name it
anything you like as long as you use the same name when you alter the
IcculusNews.php file.
Type:

    mysql> create table news_items (
             id mediumint not null auto_increment,
             ip int not null,
             approved tinyint default 0,
             deleted tinyint default 0,
             title varchar(64) not null,
             text text not null,
             author mediumint default 0,
             postdate datetime not null,
             primary key (id)
           );

    mysql> create table news_users (
             id mediumint not null auto_increment,
             name varchar(32) not null,
             password varchar(32) not null,
             access int default 0,
             createdate datetime not null,
             createip int not null,
             primary key (id)
           );

    mysql> insert into news_users
             (name, password, access)
             values ('icculus', 'password', 0xFFFFFFFF);

Next, create the MySQL user for the IcculusNews database. You can call this
user any name and give this user any password you want, just make sure that
you use the same name and password when you alter IcculusNews.php. For
this example, I will call the user "newsmgr" and make the password
"newspass". Type:

    mysql> use mysql;

    mysql> replace into user ( host, user, password )
        values ('localhost', 'newsmgr', password('newspass'));

    mysql> replace into db ( host, db, user, select_priv, insert_priv,
        update_priv, delete_priv, create_priv, drop_priv )
        values ('localhost', 'IcculusNews', 'newsmgr', 'Y', 'Y',
        'Y', 'Y', 'Y', 'Y');

    mysql> flush privileges;

Exit MySQL by typing:

    mysql> quit
*/

define("ICCULUSNEWS_ACCESSFLAG_POSTSPUBLIC",  1 << 0);
define("ICCULUSNEWS_ACCESSFLAG_SEEQUEUE",     1 << 1);
define("ICCULUSNEWS_ACCESSFLAG_APPROVEITEMS", 1 << 2);
define("ICCULUSNEWS_ACCESSFLAG_EDITITEMS",    1 << 3);

/*
session_name('IcculusNews');
session_start();
session_register('icculusnews_userid');
session_register('icculusnews_username');
session_register('icculusnews_useraccess');
*/

class IcculusNews
{
    var $sitename       = 'icculus.org';

    var $db_server      = 'localhost';
    var $db_user        = 'newsmgr';
    var $static_db_pass = undef;  // You might use an external file instead.
    var $db_passfile    = '/etc/IcculusNews_dbpass.txt';
    var $db_name        = 'IcculusNews';
    var $db_table_items = 'news_items';
    var $db_table_users = 'news_users';

    var $linktarget     = false;

    var $rdf_file       = '/webspace/icculus.org/news/news_icculus_org.rdf';
    var $rdf_url        = 'http://www.icculus.org/news/news_icculus_org.rdf';
    var $rdf_title      = 'icculus.org news';
    var $rdf_desc       = 'news from icculus.org: the open source incubator';
    var $rdf_siteurl    = 'http://www.icculus.org/';
    var $rdf_newsurl    = 'http://www.icculus.org/news/news.php';
    var $rdf_submiturl  = 'http://www.icculus.org/news/submit.php';
    var $rdf_queueurl   = 'https://www.icculus.org/news/queue.php';
    var $rdf_loginurl   = 'https://www.icculus.org/news/login.php';
    var $rdf_newuserurl = 'https://www.icculus.org/news/newuser.php';

    var $need_ssl_login = true;

    var $link           = false;
    var $err            = '';


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


    function get_database()
    {
        if ($this->link == false)
        {
            $dbpass = "";
            if ($this->static_db_pass == '')
                $dbpass = $this->static_db_pass;
            else
            {
                $fh = fopen($this->db_passfile, 'r');
                if ($fh == false)
                {
                    $this->err = "Couldn't get database password.";
                    return(false);
                } // if
                else
                {
                    $dbpass = fgets($fh, 4096);
                    $dbpass = ltrim(rtrim($dbpass));
                    fclose($fh);
                } // else
            } // else

            $this->link = mysql_connect($this->db_server,
                                        $this->db_user,
                                        $dbpass);
            if ($this->link == false)
            {
                $this->err = mysql_error();
                $this->err = "Error connecting to MySQL server: {$this->err}";
                return(false);
            } // if

            if (mysql_select_db($this->db_name, $this->link) == false)
            {
                $this->err = mysql_error();
                $this->err = "Error selecting news database: {$this->err}";
                mysql_close($this->link);
                $this->link = false;
                return(false);
            } // if
        } // if

        return($this->link);
    } // get_database


    function has_access($flags)
    {
        global $HTTP_SESSION_VARS;

//echo "{$HTTP_SESSION_VARS['icculusnews_useraccess']}<br>\n";
//echo "{$flags}<br>\n";

        return($HTTP_SESSION_VARS['icculusnews_useraccess'] & $flags);
    } // has_access


    function do_login($username, $password)
    {
        global $HTTP_SESSION_VARS;

        $link = $this->get_database();
        if ($link == false)
            return(false);

        $username = mysql_escape_string($username);

        // !!! FIXME : don't return password from db, for safety's sake.
        $sql = "select * from {$this->db_table_users} where name='$username'";
        $query = mysql_query($sql, $link);
        if ($query == false)
        {
            $this->err = mysql_error();
            $this->err = "Error in SELECT statement: {$this->err}";
            return(false);
        } // if

        $count = mysql_num_rows($query);
        if ($count > 1)
        {
            $this->err = "There's more than one user with that name." .
                         " This is VERY bad.";
            return(false);
        } // if

        $retval = false;
        if ($count == 1)
        {
            $row = mysql_fetch_array($query);
            $crypted_pw = $row['password'];
            if ($crypted_pw == crypt($password, $crypted_pw))
            {
                $HTTP_SESSION_VARS['icculusnews_userid'] = $row['id'];
                $HTTP_SESSION_VARS['icculusnews_username'] = $row['name'];
                $HTTP_SESSION_VARS['icculusnews_useraccess'] = $row['access'];

echo "access == {$row['access']}<br>\n";

                $retval = true;
            } // if
        } // if

        mysql_free_result($query);

        if ($retval == false)
        {
            sleep(2);  // discourage crackers.
            $this->err = 'Login failed';
        } // if

        return($retval);
    } // do_login


    function do_logout()
    {
        global $HTTP_SESSION_VARS;

        $HTTP_SESSION_VARS['icculusnews_userid'] = false;
        $HTTP_SESSION_VARS['icculusnews_username'] = false;
        $HTTP_SESSION_VARS['icculusnews_useraccess'] = false;

        session_unregister("icculusnews_userid");
        session_unregister("icculusnews_username");
        session_unregister("icculusnews_useraccess");
    } // do_logout


    function current_sql_datetime()
    {
        $t = localtime(time(), true);
        return( "" . ($t['tm_year'] + 1900) . '-' .
                     ($t['tm_mon'] + 1) . '-' .
                     ($t['tm_mday']) . ' ' .
                     ($t['tm_hour']) . ':' .
                     ($t['tm_min']) . ':' .
                     ($t['tm_sec']) );
    } // current_sql_datetime


    function update_rdf($item_max = 5)
    {
        $query = $this->fetch_news_items(
                  "where approved=1 and deleted=0" .
                  " order by postdate desc limit $item_max");
        if ($query == false)
            return(false);

        $itemlist = '';
        $items = '';

        while ( ($row = mysql_fetch_array($query)) != false )
        {
            $url = "{$this->rdf_newsurl}?id={$row['id']}";
            $itemlist .= "\n        <rdf:li resource=\"$url\" />";
            $items .= "\n" .
                      "  <item rdf:about=\"$url\">\n".
                      "    <title>{$row['title']}</title>\n" .
                      "    <link>$url</link>\n" .
                      //"    <description>\n" .
                      //"      {$row['text']}\n" .
                      //"    </description>\n" .
                      "  </item>";
        } // while

        mysql_free_result($query);

        $fp = fopen($this->rdf_file, "wb");
        if ($fp == false)
        {
            $this->err = "failed to open RDF file for writing.";
            return(false);
        } // if

        $deprecated = '';
        if (false)
        {
            $deprecated =     "<items>\n" .
                          "      <rdf:Seq>{$itemlist}\n" .
                          "      </rdf:Seq>\n" .
                          "    </items>\n";
        } // if

        $rdftext =<<< EOF
<?xml version="1.0" encoding="utf-8"?>

<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns="http://purl.org/rss/1.0/">

  <channel rdf:about="{$this->rdf_url}">
    <title>{$this->rdf_title}</title>
    <link>{$this->rdf_siteurl}</link>
    <description>{$this->rdf_desc}</description>
    $deprecated
  </channel>
{$items}
</rdf:RDF>

EOF;

        fputs($fp, $rdftext, strlen($rdftext));
        fclose($fp);
    } // update_rdf


    function mark_news_item_approved($id, $value = 1)
    {
        $link = $this->get_database();
        if ($link == false)
            return(false);

        $sql = "update {$this->db_table_items} set approved=$value" .
               " where id=$id";

        $query = mysql_query($sql, $link);
        if ($query == false)
        {
            $this->err = mysql_error();
            $this->err = "Error in UPDATE statement: {$this->err}";
            return(false);
        } // if

        if (mysql_affected_rows($link) != 1)
        {
            $this->err = "Didn't mark; make sure item actually exists.";
            return(false);
        } // if

        $this->update_rdf();
        return(true);
    } // mark_news_item_approved


    function mark_news_item_deleted($id, $value)
    {
        $link = $this->get_database();
        if ($link == false)
            return(false);

        $sql = "update {$this->db_table_items} set deleted=$value where id=$id";
        $query = mysql_query($sql, $link);
        if ($query == false)
        {
            $this->err = mysql_error();
            $this->err = "Error in UPDATE statement: {$this->err}";
            return(false);
        } // if

        if (mysql_affected_rows($link) != 1)
        {
            $this->err = "Didn't mark; make sure item actually exists.";
            return(false);
        } // if

        $this->update_rdf();
        return(true);
    } // mark_news_item_deleted


    function purge_news_item($id)
    {
        $link = $this->get_database();
        if ($link == false)
            return(false);

        $sql = "delete from {$this->db_table_items}" .
               " where deleted=1 and id=$id";
        $query = mysql_query($sql, $link);
        if ($query == false)
        {
            $this->err = mysql_error();
            $this->err = "Error in DELETE statement: {$this->err}";
            return(false);
        } // if

        if (mysql_affected_rows($link) != 1)
        {
            $this->err = "Didn't purge; make sure item exists and is" .
                         " flagged for deletion.";
            return(false);
        } // if

        return(true);
    } // purge_news_item


    function purge_all_news_items()
    {
        $link = $this->get_database();
        if ($link == false)
            return(false);

        $sql = "delete from {$this->db_table_items} where deleted=1";
        $query = mysql_query($sql, $link);
        if ($query == false)
        {
            $this->err = mysql_error();
            $this->err = "Error in DELETE statement: {$this->err}";
            return(false);
        } // if

        return(true);
    } // purge_all_news_items


    function update_news_item($posted, $title, $text, $author, $id)
    {
        $link = $this->get_database();
        if ($link == false)
            return(false);

        $sql = "update {$this->db_table_items} set " .
               " postdate='$posted'," .
               " title='$title'," .
               " text='$text'," .
               " author='$author'" .
               " where id=$id";

        $query = mysql_query($sql, $link);
        if ($query == false)
        {
            $this->err = mysql_error();
            $this->err = "Error in UPDATE statement: {$this->err}";
            return(false);
        } // if

        $this->update_rdf();
        return(true);
    } // update_news_item


    function submit_news_item($posted, $title, $text, $author, $id = 0)
    {
        global $REMOTE_ADDR;

        $title = mysql_escape_string($title);
        $text = mysql_escape_string($text);
        $author = mysql_escape_string($author);

        if ($id != 0)
        {
            return($this->update_news_item($posted, $title, $text,
                                           $author, $id));
        } // if

        $link = $this->get_database();
        if ($link == false)
            return(false);

        $ip = ip2long($REMOTE_ADDR);
        $sql = "insert into {$this->db_table_items}" .
               " (ip, title, text, author, postdate)" .
               " values ('$ip', '$title', '$text', '$author', '$posted')";
        $query = mysql_query($sql, $link);
        if ($query == false)
        {
            $this->err = mysql_error();
            $this->err = "Error in INSERT statement: {$this->err}";
            return(false);
        } // if

        if (mysql_affected_rows($link) != 1)
        {
            $this->err = mysql_error();
            $this->err = "Couldn't insert: {$this->err}";
            return(false);
        } // if

        return(true);
    } // submit_news_item


    function do_news_item($digest_mode, $posted, $title, $text, $author, $id = 0, $target = false)
    {
        $href = '';
        $endhref = '';
        if (($digest_mode) and ($id != 0))
        {
            $t = '';
            if ($target != false)
                $t = "target=\"$target\"";
            $href = "<a $t href=\"{$this->rdf_newsurl}?id=$id\">";
            $endhref = '</a>';
        } // if

        printf("<li>%s<b>%s</b>%s", $href, $title, $endhref);
        printf(" <font size=\"-3\">(posted %s by <i>%s</i>)</font>",
                $posted, $author);

        if ($digest_mode)
            printf(".");
        else
            printf(":<br>\n%s<br>\n--%s.", $text, $author);

        printf("\n");
    } // do_news_item


    function fetch_news_items($conditions)
    {
        $link = $this->get_database();
        if ($link == false)
            return(false);

        $sql = "select * from {$this->db_table_items} $conditions";
        $query = mysql_query($sql, $link);
        if ($query == false)
        {
            $this->err = mysql_error();
            $this->err = "Error in SELECT statement: $err";
        } // if

        return($query);
    } // fetch_news_items


    function do_news($digest_mode = true, $item_max = 5, $id = 0, $target = false)
    {
        print("\n");

        $conditions = "where approved=1 and deleted=0";

        if ($id != 0)
            $conditions .= " and id=$id";

        $conditions .= " order by postdate desc";

        if ($item_max >= 0)
            $conditions .= " limit $item_max";

        $query = $this->fetch_news_items($conditions);
        if ($query == false)
        {
            print("<li><font color=\"#FF0000\">{$this->err}</font>\n");
            return;
        } // if

        if (mysql_num_rows($query) == 0)
        {
            print('<li><font color="#FF0000">');
            print(' No news found, which is okay,');
            print(' because no news is good news.');
            print("</font>\n");
        } // if

        else
        {
            while ( ($row = mysql_fetch_array($query)) != false )
            {
                $this->do_news_item($digest_mode, $row['postdate'],
                                    $row['title'], $row['text'],
                                    $row['author'], $row['id'], $target);
            } // while
        } // else

        mysql_free_result($query);
    } // do_news


    function handle_news_edit_commands()
    {
        global $news;
        global $form_submit, $form_preview, $form_name, $form_item;
        global $form_title, $form_postdate;
        global $editid;
        global $GLOBALS;

        // reload from database...
        if ( (isset($editid)) &&
             ($editid != 0) &&
             ($form_submit == false) &&
             ($form_preview == false) )
        {
           $query = $news->fetch_news_items("where id=$editid");
           if ($query == false)
           {
               print("<font color=\"#FF0000\">Failed to grab item $editid. {$news->err}</font><br>\n");
               unset($GLOBALS['editid']);
               return;
           } // if

           $row = mysql_fetch_array($query);
           $form_title = $row['title'];
           $form_name  = $row['author'];
           $form_item  = $row['text'];
           $form_postdate = $row['postdate'];
           $form_preview = 1;
           mysql_free_result($query);
           //return;
        } // if

        $form_name  = ltrim(rtrim($form_name));
        $form_item  = ltrim(rtrim($form_item));
        $form_title = ltrim(rtrim($form_title));

        if (strlen($form_name) > 32)
            $form_name = substr($form_name, 0, 32);

        if (strlen($form_title) > 64)
            $form_title = substr($form_title, 0, 64);

        if ( (($form_preview) or ($form_submit)) and
             (($form_name == "") or ($form_item == "") or ($form_title == "")) )
        {
            printf("<center><font color=\"#FF0000\">");
            printf("Please enter all fields. kthxbye.");
            printf("</font></center>\n");
            printf("<hr>\n");
            $form_submit = false;
            $form_preview = false;
            return;
        } // if

        if ($form_submit)
        {
            $id = (isset($editid)) ? $editid : 0;
            $rc = $news->submit_news_item($form_postdate, $form_title,
                                          $form_item, $form_name, $id);
            if ($rc == false)
            {
                $form_submit = "";
                printf("\n<center>\n");
                printf("<font color=\"#FF0000\">ERROR SUBMITTING NEWS!</font><br>\n");
                printf("%s\n<br>\n", $news->err);
                printf("Please try submitting again later. Sorry.\n</center><hr>\n");
            } // if

            else
            {
                printf("\n<center>\n");
                printf("<font color=\"#0000FF\">Thanks for the news!</font><br>\n");
                printf("It will likely be posted after Ryan has poked, prodded,\n");
                printf(" and probably rewritten it.\n</center>\n");
            } // else
        } // if

        if (($form_submit) or ($form_preview))
        {
            printf("<center>\n");
            printf("Your submission %s like this:\n",
                    ($form_submit) ? "looked" : "looks");

            if ($form_preview)
            {
                printf("<br><i>(When satisfied, hit the \"Submit\" button to");
                printf(" stick it in the submission queue and go!)</i>\n");
            } // if

            else
            {
                printf("<br><i>(If you don't like it,");
                printf(" you should have used the Preview button!)</i>\n");
            } // else

            printf("</center>\n");
            printf("<hr>\n");
            printf("<p><ul>\n");
            $news->do_news_item(false, $form_postdate, $form_title,
                                $form_item, $form_name, false);
            printf("</ul><hr></p>\n");

            if ($form_submit)
            {
                $form_item  = "";
                $form_title = "";
                unset($GLOBALS['form_postdate']);
                $editid = 0;
                unset($GLOBALS['editid']);
            } // if
        } // if
    } // handle_news_edit_commands


    function output_news_edit_widgets()
    {
        global $news;
        global $form_submit, $form_preview, $form_name, $form_item;
        global $form_title, $form_postdate;
        global $editid;
        global $HTTPS;

        if (isset($form_postdate) == false)
            $form_postdate = $news->current_sql_datetime();

echo <<< EOF

<form method="post" action="$PHP_SELF">
  <input type="hidden" name="form_postdate" value="$form_postdate">

EOF;


        if (isset($editid))
        {
           print('<input type="hidden" name="editid"');
           print(" value=\"$editid\">\n");
        } // if

        print("  <table>\n");

        if ($this->has_access(ICCULUSNEWS_ACCESSFLAG_SEEQUEUE))
        {
            echo <<< EOF
    <tr>
      <td colspan="2">
        <font size="-3">
          (<a href="{$news->rdf_queueurl}">View queue instead.</a>)
        </font>
      </td>
    </tr>

EOF;
        } // if

        echo <<< EOF
    <tr>
      <td>My name:</td>
      <td><input type="text" name="form_name" value="$form_name"></td>
    </tr>

    <tr>
      <td>News title:</td>
      <td><input type="text" name="form_title" value="$form_title"></td>
    </tr>

    <tr>
      <td>News text:</td>
      <td>
        <textarea rows="6" name="form_item" cols="60">$form_item</textarea>
      </td>
    </tr>

    <tr>
      <td colspan="2" align="center">
        <input type="submit" name="form_preview" value="Preview">
EOF;


    if ($form_preview)
        print('<input type="submit" name="form_submit" value="Submit">');

echo <<< EOF
      </td>
    </tr>
  </table>
</form>
EOF;

    } // output_news_edit_widgets

} // IcculusNews

?>
