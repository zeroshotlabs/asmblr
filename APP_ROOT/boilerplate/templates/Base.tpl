<!DOCTYPE html>
<!--[if lt IE 7]>      <html class="no-js lt-ie9 lt-ie8 lt-ie7"> <![endif]-->
<!--[if IE 7]>         <html class="no-js lt-ie9 lt-ie8"> <![endif]-->
<!--[if IE 8]>         <html class="no-js lt-ie9"> <![endif]-->
<!--[if gt IE 8]><!--> <html class="no-js"> <!--<![endif]-->
    <head>
<?php
/*
Firefox performs double requests if output come before the doctype.
Use PHP to set ContentType/charset in the HTTP headers.
<meta charset="utf-8">
*/?>
        <meta http-equiv="X-UA-Compatible" content="IE=edge,chrome=1">

<?php
/*
Since $page has been Connect()'d in fwboot.php::Go(), it's available here
in the templates of the site.

$this is automatically available and is the TemplateSet object that's
rendering the template.  In this case, since we used locale-specific
enUSHTMLSet for HTML, $this provides functionality for English-based
HTML template.

Here we call it as a function to properly HTML encode the strings that
have been set in $page, such as the Title.

Note that this triggers enUSHTMLSet::__invoke().
*/?>
        <title><?=$this($page->Title)?></title>
        <meta name="description" content="<?=$this($page->Description)?>">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">

<?php
/*
$lp is the LinkPage object instantiated and Connect()'d in fwboot::Go().

It creates absolute URLs based on the currently requested URL and the
configured fwApp::$BaseURL.

Here we call it as a function to create URLs for the CSS and JS pages,
which we've defined in fwboot::Go() to handle the two URLs hierachies.
For each call, we use a URL change string:
    - the leading '>' indicates the string should be treated as path a
      segment and appended to the URL of the Page (either /css/ or /js/).
    - the trailing '?' indicates that the query string should be empty.

Note that this triggers LinkPage::__invoke().
*/?>
        <link rel="stylesheet" href="//code.jquery.com/ui/1.10.2/themes/smoothness/jquery-ui.css">
        <link rel="stylesheet" href="//static.cnvyr.io/bootstrap.min.css">
        <link rel="stylesheet" href="//static.cnvyr.io/bootstrap-responsive.min.css">

        <link rel="stylesheet" href="<?=$lp('CSS','>fw.css?')?>">

        <script src="//static.cnvyr.io/modernizr-2.6.2-respond-1.1.0.min.js"></script>
    </head>

    <body>
    <!--[if lt IE 7]>
        <p class="chromeframe">You are using an <strong>outdated</strong> browser. Please <a href="http://browsehappy.com/">upgrade your browser</a> or <a href="http://www.google.com/chromeframe/?redirect=true">activate Google Chrome Frame</a> to improve your experience.</p>
    <![endif]-->

<?php
/*
Templates can be nested to create different parts of a page.

Here we call the Header() method on $this to render the header
template, HTML/Header.tpl.

Note that these are overloaded methods handled by TemplateSet::__call().
*/?>
    <div class="container-fluid">
        <?php $this->Header(); ?>
    </div>

<?php
/*
$ls is the LinkSet object instantiated and Connect()'d in fwboot::Go().

Similar to $lp, it creates absolute URLs, but for generic files, instead
of for Pages.

Here we call it as a function to create a URL for an image.  The URL given
will be appended to the base URL of the application, fwApp::$SiteURL.

Note that this triggers LinkSet::__invoke().
*/?>
	<div id="spinner" class="spinner">
        <img id="img-spinner" src="<?=$limg('ajax-loader.gif')?>" alt="Loading"/>
    </div>

    <section class="container-fluid">
<?php
/*
Utilizing Bootstrap CSS, render and include the main content template,
Article.  Article will be ReMap()'d depending on the Page being requested.
*/?>
        <div class="row-fluid">
            <article class="span9">
                <?php $this->Article(); ?>
            </article>
<?php
/*
A Template is considered unset if it's been ReMap()'d to NULL,
or hasn't been set in the first place.

Here we check if the right aside is set, and if so, render it.
*/?>
           <?php if( isset($this->RightAside) === TRUE ): ?>
            <aside id="rightaside" class="span3">
                <?php $this->RightAside(); ?>
            </aside>
           <?php endif; ?>
        </div>
    </section>

	<footer class="container-fluid">
	    <?php $this->Footer(); ?>
	</footer>
<?php
/*
As a best practice, load Javascript/jQuery at the end of the page
(modernizr must be loaded at the beginning).
*/?>
    <script src="//code.jquery.com/jquery-latest.js"></script>
	<script src="<?=$lp('JS','>bootstrap.min.js?')?>"></script>

<?php
/*
Loading jQuery at the bottom of the page, however, also requires
that the inclusion of our own Javascript be deferred.

The TemplateSet::Stack() and TemplateSet::Unstack() methods allow
templates to be queued - or 'stacked' - for later rendering.

Here we Unstack() the 'ajax' stack, which will cause each template
to be rendered in the order it was Stack()'d.

See examples/Home.tpl for the Stack() call.

    <script>
        <?php $html->jss_lib(); ?>
        $(function(){
        <?php
            $html->jss_global();
            $html->Unstack('jsready');
        ?>
        });
    </script>
*/?>

	<?php $this->Unstack('ajax'); ?>
	</body>
</html>

