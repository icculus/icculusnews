<html>
  <head>
    <title> IcculusNews 2.0 posting example. </title>
  </head>
  <body>

    <?php
      include 'IcculusNews.php';

      $starttime = gettimeofday();

      $err = NULL;
      $queuearray = NULL;

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
              $text = "<font size=\"3\">bitch midget wastin' my digit.</font>";
              // post something.
              if ($err = news_post($sock, "This is my title", $text))
                  $err = "Failed to post: $err.";
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
          print("<center><font color=\"#0000FF\">Text posted</font></center><br>\n");

    ?>

  </body>
</html>


