<?php
    require('./IcculusNews.php');

// !!! FIXME: Move this somewhere.
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



function output_archive($news_queue, $host = 'localhost', $profile = false)
{
    $err = NULL;
    $digestarray = NULL;

    if ($profile)
        $starttime = gettimeofday();

    // log in as Anonymous Hoser.
    if ($err = news_login($sock, $host))
        $err = "Login failed: $err.";
    else
    {
        if ($err = news_change_queue($sock, $news_queue))
            $err = "Failed to select queue: $err.";
        else
        {
            if (!isset($viewItemURL))
            {
                $err = news_queueinfo($sock, $news_queue, $qinfo);
                $viewItemURL = $qinfo['itemviewurl'];
            } // if

            if ($err)
                $err = "Failed to get queue info: $err.";
            else
            {
                if ($err = news_digest($sock, $digestarray, 9999999))
                    $err = "Failed to get digest: $err.";
            } // else
        } // else
    } // else

    // disconnect from the news daemon, since we're done with it ...
    news_logout($sock);

    if ($profile)
    {
        $endtime = gettimeofday();
        $totaltime = ((($endtime['sec'] - $starttime['sec']) * 1000) +
                      (($endtime['usec'] - $starttime['usec']) / 1000));
        print("<li>Time spent talking to daemon: $totaltime milliseconds\n");
    } // if

    if (isset($err))
        print("<li><font color=\"#FF0000\">$err</font>\n");
    else
    {
        // finally, list out the digest.
        foreach ($digestarray as $item)
        {
            $newsurl = str_replace('%id', $item['id'], $viewItemURL);
            print("<li><b><a href=\"$newsurl\">${item['title']}</a></b>");
            print(" <font size=\"-3\">(posted ${item['postdate']}");
            print(" by <i>${item['author']}</i>).</font>\n\n");
        } // foreach
    } // else
} // output_archive


function output_item($news_queue, $id, $host = 'localhost', $profile = false)
{
    $err = NULL;
    $digestarray = NULL;

    if ($profile)
        $starttime = gettimeofday();

    // log in as Anonymous Hoser.
    if ($err = news_login($sock, $host))
        $err = "Login failed: $err";
    else
    {
        if ($err = news_change_queue($sock, $news_queue))
            $err = "Failed to select queue: $err";
        else
        {
            if ($err = news_get($sock, $id, $item))
                $err = "Failed to get news item: $err";
        } // else
    } // else

    // disconnect from the news daemon, since we're done with it ...
    news_logout($sock);

    if ($profile)
    {
        $endtime = gettimeofday();
        $totaltime = ((($endtime['sec'] - $starttime['sec']) * 1000) +
                      (($endtime['usec'] - $starttime['usec']) / 1000));
        print("<li>Time spent talking to daemon: $totaltime milliseconds\n");
    } // if

    if (isset($err))
        print("<li><font color=\"#FF0000\">$err</font>\n");
    else
    {
        print("<li><b>${item['title']}</b>");
        print(" <font size=\"-3\">(posted ${item['postdate']}");
        print(" by <i>${item['author']}</i>)</font>");
        print(":<br>\n${item['text']}<br>\n--${item['author']}.");
    } // else
} // output_item

?>

<html>
  <head>
    <title> icculus.org headlines </title>
  </head>

  <body>
    <?php output_site_header(); ?>

    <p><b>News archive:</b> <font size="-3"><a href="/news/queue.php?action=post">(submit news)</a></font><br>
    <ul>
      <?php
        if ( (isset($id)) and ($id != 0) )
            output_item(1, $id, 'localhost', false);
        else
            output_archive(1, 'localhost', false);
      ?>
    </ul>

    <?php output_site_footer(); ?>
  </body>
</html>

