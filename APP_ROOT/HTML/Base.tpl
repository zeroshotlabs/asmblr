<!DOCTYPE html>
<!--[if lt IE 7]>      <html class="no-js lt-ie9 lt-ie8 lt-ie7"> <![endif]-->
<!--[if IE 7]>         <html class="no-js lt-ie9 lt-ie8"> <![endif]-->
<!--[if IE 8]>         <html class="no-js lt-ie9"> <![endif]-->
<!--[if gt IE 8]><!--> <html class="no-js"> <!--<![endif]-->
    <head>
        <title><?=$this($page->Title)?></title>
        <meta http-equiv="X-UA-Compatible" content="IE=edge,chrome=1">
        <meta name="description" content="<?=$this($page->Description)?>">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">

        <link rel="stylesheet" href="//code.jquery.com/ui/1.10.2/themes/smoothness/jquery-ui.css">
        <link rel="stylesheet" href="//static.cnvyr.io/bootstrap.min.css">
        <link rel="stylesheet" href="//static.cnvyr.io/bootstrap-responsive.min.css">
        <link rel="stylesheet" href="//static.cnvyr.io/bootstrap-select.min.css">
        <link rel="stylesheet" href="//cdnjs.cloudflare.com/ajax/libs/x-editable/1.4.4/bootstrap-editable/css/bootstrap-editable.css">
        <link rel="stylesheet" href="<?=$ls('/jslib/codemirror-3.13/lib/codemirror.css')?>" />
        <link rel="stylesheet" href="<?=$ls('/jslib/jQuery-File-Upload-8.3.2/css/jquery.fileupload-ui.css')?>" />
        <link rel="stylesheet" href="<?=$lp('asmconcss')?>">

        <script src="//static.cnvyr.io/modernizr-2.6.2-respond-1.1.0.min.js"></script>
    </head>

    <body>
    <!--[if lt IE 7]>
        <p class="chromeframe">You are using an <strong>outdated</strong> browser. Please <a href="http://browsehappy.com/">upgrade your browser</a> or <a href="http://www.google.com/chromeframe/?redirect=true">activate Google Chrome Frame</a> to improve your experience.</p>
    <![endif]-->

    <div class="container-fluid">
        <?php $this->Header(); ?>
    </div>

	<div id="spinner" class="spinner">
        <img id="img-spinner" src="<?=$ls('/img/ajax-loader.gif')?>" alt="Loading..." />
    </div>

    <div class="container-fluid">
        <div class="row-fluid">
           <?php if( isset($this->LeftNav) ): ?>
            <div class="span3 LeftNav">
                <?php $this->LeftNav(); ?>
            </div>
           <?php endif; ?>
            <div class="span9">
                <?php $this->Article(); ?>
            </div>
        </div>
    </div>

	<footer class="container-fluid">
	    <?php $this->Footer(); ?>
	</footer>

    <script src="//code.jquery.com/jquery-latest.js"></script>
    <script src="//code.jquery.com/ui/1.10.2/jquery-ui.js"></script>
    <script src="//static.cnvyr.io/bootstrap.min.js"></script>
    <script src="//static.cnvyr.io/bootstrap-select.min.js"></script>
    <script src="//cdnjs.cloudflare.com/ajax/libs/x-editable/1.4.4/bootstrap-editable/js/bootstrap-editable.min.js"></script>

    <script src="<?=$ls('/jslib/codemirror-3.13/lib/codemirror.js')?>"></script>
    <script src="<?=$ls('/jslib/codemirror-3.13/addon/edit/matchbrackets.js')?>"></script>
    <script src="<?=$ls('/jslib/codemirror-3.13/mode/htmlmixed/htmlmixed.js')?>"></script>
    <script src="<?=$ls('/jslib/codemirror-3.13/mode/xml/xml.js')?>"></script>
    <script src="<?=$ls('/jslib/codemirror-3.13/mode/javascript/javascript.js')?>"></script>
    <script src="<?=$ls('/jslib/codemirror-3.13/mode/css/css.js')?>"></script>
    <script src="<?=$ls('/jslib/codemirror-3.13/mode/clike/clike.js')?>"></script>
    <script src="<?=$ls('/jslib/codemirror-3.13/mode/php/php.js')?>"></script>

<?php /*
    <script src="http://blueimp.github.io/cdn/js/bootstrap.min.js"></script>
    <script src="http://blueimp.github.io/JavaScript-Canvas-to-Blob/canvas-to-blob.min.js"></script>
    <script src="http://blueimp.github.io/Bootstrap-Image-Gallery/js/bootstrap-image-gallery.min.js"></script>
    <script src="http://blueimp.github.io/JavaScript-Templates/tmpl.min.js"></script>

*/ ?>

    <script src="http://blueimp.github.io/JavaScript-Load-Image/load-image.min.js"></script>
    <script src="<?=$ls('/jslib/jQuery-File-Upload-8.3.2/js/jquery.iframe-transport.js')?>"></script>
    <script src="<?=$ls('/jslib/jQuery-File-Upload-8.3.2/js/jquery.fileupload.js')?>"></script>
    <script src="<?=$ls('/jslib/jQuery-File-Upload-8.3.2/js/jquery.fileupload-process.js')?>"></script>
    <script src="<?=$ls('/jslib/jQuery-File-Upload-8.3.2/js/jquery.fileupload-image.js')?>"></script>
    <script src="<?=$ls('/jslib/jQuery-File-Upload-8.3.2/js/jquery.fileupload-audio.js')?>"></script>
    <script src="<?=$ls('/jslib/jQuery-File-Upload-8.3.2/js/jquery.fileupload-video.js')?>"></script>
    <script src="<?=$ls('/jslib/jQuery-File-Upload-8.3.2/js/jquery.fileupload-validate.js')?>"></script>
    <script src="<?=$ls('/jslib/jQuery-File-Upload-8.3.2/js/jquery.fileupload-ui.js')?>"></script>
    <!--[if gte IE 8]><script src="<?=$ls('/jslib/jQuery-File-Upload-8.3.2/js/cors/jquery.xdr-transport.js')?>"></script><![endif]-->

    <script src="//static.cnvyr.io/masonry.pkgd.min.js"></script>

    <script>
        <?php $this->jss_lib(); ?>
        $(function(){
        <?php
            $this->jss_global();
            $this->Unstack('jsready');
        ?>
        });
    </script>
	</body>
</html>

