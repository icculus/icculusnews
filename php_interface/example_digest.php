<!--
Note that it's more efficient to parse out the RDF file the daemon spits
out, and doubly-so if you parse the RDF file and write out static HTML
instead. This is useful if you want to do something dynamic (such as the
queue editor does), or you want more items than are listed in the RDF
file, etc.)

In short, use examine news_parse_rdf() unless you need more power, since it
runs on average _TWENTY_ times faster and takes less resources.
-->

<html>
  <head>
    <title> IcculusNews 2.0 digest example. </title>
  </head>
  <body>

    <?php
      include 'IcculusNews.php';

      $err = NULL;
      $digestarray = NULL;

      $starttime = gettimeofday();

      // log in as Anonymous Hoser.
      if ($err = news_login($sock, 'localhost'))
          $err = "Login failed: $err.";
      else
      {
          // select a news queue. '1' is a safe bet.
          if ($err = news_change_queue($sock, 1))
              $err = "Failed to select queue: $err.";
          else
          {
              // get the five newest news items...
              if ($err = news_digest($sock, $digestarray, 5))
                  $err = "Failed to get digest: $err.";
          } // else
      } // else

      // disconnect from the news daemon, since we're done with it ...
      news_logout($sock);

      $endtime = gettimeofday();
      $totaltime = ((($endtime['sec'] - $starttime['sec']) * 1000) +
                    (($endtime['usec'] - $starttime['usec']) / 1000));
      print("Time spent talking to daemon: $totaltime milliseconds<br>\n");

      if (isset($err))
          print("<center><font color=\"#FF0000\">$err</font></center><br>\n");
      else
      {
          // finally, list out the digest.
          print("\n<ul>\n\n");
          foreach ($digestarray as $item)
          {
              $style = $endstyle = '';

              if ($item['deleted'])
              {
                  $style .= '<strike>';
                  $endstyle = '</strike>' . $endstyle;
              } // if

              if (!$item['approved'])
              {
                  $style .= '<b>';
                  $endstyle = '</b>' . $endstyle;
              } // if

              print("<li>$style${item['title']} (item #${item['id']})\n");
              print("<font size=\"-3\">(posted on ${item['postdate']} from ${item['ipaddr']}\n");
              print("by <i>${item['author']} (user #${item['authid']})</i>).</font>$endstyle\n\n");
          } // foreach
          print("</ul>\n");
      } // else
    ?>

  </body>
</html>


