<?php

require('./IcculusNews.php');

$news = new IcculusNews();

?>

<html>
  <head>
    <title> Submit news for <?php echo $news->sitename?> </title>
  </head>

  <body>
    <?php
        $news->output_site_header();

        // make sure editid isn't set, so that you can't modify existing
        //  database entries through this page...
        if (isset($editid))
            unset($GLOBALS['editid']);

        $news->handle_news_edit_commands();
    ?>

    <p>
      Submit <?php if ($form_submit) { echo "more"; } ?> news:<br>
      <?php $news->output_news_edit_widgets(); ?>
    </p>

    <?php $news->output_site_footer(); ?>
  </body>
</html>


<!-- end of submit.php ... -->

