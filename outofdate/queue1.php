<?php

require('./IcculusNews.php');

$news = new IcculusNews();

function output_queue_rows($showall = false)
{
    global $news;
    global $PHP_SELF;

    $sql = "";
    if ($showall == false)
        $sql = "where approved=0 ";
    $sql .= "order by postdate desc";

    $query = $news->fetch_news_items($sql);
    if ($query == false)
    {
        print('<tr><td align="center" colspan="5"><font color="#FF0000">');
        print("{$news->err}</font></td></tr>\n");
        return;
    } // if

    $row_count = mysql_num_rows($query);
    $pending = 0;
    $deleted = 0;

    while ( ($row = mysql_fetch_array($query)) != false )
    {
        $tags = $endtags = '';
        if ($row['deleted'])
        {
            $tags     = "$tags<strike>";
            $endtags .= '</strike>';
            $deleted++;
        } // if

        if ($row['approved'])
        {
            $tags     = "$tags<i>";
            $endtags .= '</i>';
        } // if
        else
        {
            $pending++;
        } // else

        $ip = long2ip($row['ip']);

        print("<tr>\n");
        print('<td align="center"> <input type="checkbox" name="itemid[]"');
        print(" value=\"{$row['id']}\"></td>\n");

        print("<td align=\"center\"> $tags {$row['postdate']} $endtags </td>\n");

        print("<td align=\"center\"> $tags");
        print(" <a href=\"{$PHP_SELF}?editid={$row['id']}\">");
        print("{$row['title']} $endtags </a> </td>\n");

        print("<td align=\"center\"> $tags {$row['author']} $endtags </td>\n");
        print("<td align=\"center\"> $tags $ip $endtags </td>\n");
        print("</tr>\n");
    } // while

    $approved = $row_count - $pending;
    print('<tr><td align="center" colspan="5"><font color="#0000FF">');
    print("$row_count items listed, $deleted deleted, $approved approved, $pending pending.</font></td></tr>\n");
} // output_queue_rows


function output_news_queue_widgets()
{
    global $PHP_SELF;
    global $news;
    global $showall;

    if (isset($showall) == false)
        $showall = 0;

    $showallflip = 1;
    $showalltext = "[Show all items]";
    if ($showall)
    {
        $showallflip = 0;
        $showalltext = "[Show only pending items]";
    } // if

echo <<< EOF

    <center>
      <a href="$PHP_SELF?showall=$showallflip">$showalltext</a>
      <form method="post" action="$PHP_SELF?showall=$showall">
      <table border="1">
        <tr>
          <td align="center"> X </td>
          <td align="center"> date </td>
          <td align="center"> title </td>
          <td align="center"> author </td>
          <td align="center"> ip addr </td>
        </tr>

EOF;

    output_queue_rows($showall);

echo <<< EOF

        <tr>
          <td align="center" colspan="5">
            <input type="submit" name="form_refresh"  value="Refresh">
            <input type="submit" name="form_del"      value="Delete">
            <input type="submit" name="form_undel"    value="Undelete">
            <input type="submit" name="form_approve"  value="Approve">
            <input type="submit" name="form_purge"    value="Purge Selected">
            <input type="submit" name="form_purgeall" value="Purge All">
          </td>
        </tr>
      </table>
      </form>

      <br>

      <table width="75%">
        <tr>
          <td align="center"><a href="submit.php">Add new items.</a></td>
          <td align="center"><a href="news.php">View news page.</a></td>
          <td align="center"><a href="{$news->rdf_siteurl}">View front page.</a></td>
        </tr>
      </table>
    </center>
EOF;

} // output_news_queue_widgets


function handle_news_queue_commands()
{
    global $news;
    global $itemid, $form_del, $form_undel, $form_purge, $form_purgeall;
    global $form_approve;

    $delflag = -1;
    if ($form_del)
        $delflag = 1;
    else if ($form_undel)
        $delflag = 0;

    if ($delflag != -1)
    {
         $max = count($itemid);
         for ($i = 0; $i < $max; $i++)
         {
             if ($news->mark_news_item_deleted($itemid[$i], $delflag) == false)
             {
                printf("<font color=\"#FF0000\">failed to mark item %s." .
                       " %s</font><br>\n", $itemid[$i], $news->err);
             } // if
         } // for
    } // if

    else if ($form_purge)
    {
        $max = count($itemid);
        for ($i = 0; $i < $max; $i++)
        {
            if ($news->purge_news_item($itemid[$i]) == false)
            {
                printf("<font color=\"#FF0000\">failed to purge item %s." .
                       " %s</font><br>\n", $itemid[$i], $news->err);
            } // if
        } // for
    } // else if

    else if ($form_purgeall)
    {
        if ($news->purge_all_news_items() == false)
        {
            printf("<font color=\"#FF0000\">failed to purge items." .
                   " %s</font><br>\n", $itemid[$i], $news->err);
        } // if
    } // else if

    else if ($form_approve)
    {
        $max = count($itemid);
        for ($i = 0; $i < $max; $i++)
        {
            if ($news->mark_news_item_approved($itemid[$i]) == false)
            {
                printf("<font color=\"#FF0000\">failed to approve item %s." .
                       " %s</font><br>\n", $itemid[$i], $news->err);
            } // if
        } // for
    } // else if
} // handle_news_queue_commands

?>


<html>
  <head>
    <title> Submission queue for <?php echo $news->sitename?> </title>
  </head>

  <body>
    <?php
        $news->output_site_header();
        if (isset($editid))
        {
            $news->handle_news_edit_commands();
            if ( (isset($editid)) && ($editid != 0) )
                $news->output_news_edit_widgets();
        } // if

        if ( (isset($editid) == false) || ($editid == 0) )
        {
            handle_news_queue_commands();
            output_news_queue_widgets();
        } // else
        $news->output_site_footer();
    ?>
  </body>
</html>

<!-- end of queue.php ... -->

