<html>
  <head>
    <title> IcculusNews 2.0 queue enumeration example. </title>
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
          // list out queues.
          if ($err = news_enum_queues($sock, $queuearray))
              $err = "Failed to enum queues: $err.";
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
          // finally, list out the queues...
          print("\n<ul>\n");
          foreach ($queuearray as $queueid => $queuename)
          {
              print("    <li>$queueid: [$queuename]\n");
          } // foreach
          print("</ul>\n");
      } // else
    ?>

  </body>
</html>


