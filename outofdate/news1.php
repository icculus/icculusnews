<?php
    require('./IcculusNews.php');
    $news = new IcculusNews();
?>

<html>
  <head>
    <title> icculus.org headlines </title>
  </head>

  <body>
    <?php $news->output_site_header(); ?>

    <p><b>News archive:</b> <font size="-3"><a href="submit.php">(submit news)</a></font><br>
    <ul>
      <?php
        if ( (isset($id)) and ($id != 0) )
        {
            $news->do_news(false, 1, $id);
        } // if
        else
        {
            $news->do_news(false, -1, 0);
        } // else
      ?>
    </ul>

    <?php $news->output_site_footer(); ?>
  </body>
</html>


