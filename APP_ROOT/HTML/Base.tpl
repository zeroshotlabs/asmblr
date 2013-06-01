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
        <link rel="stylesheet" href="<?=$lp('CSS','>bootstrap.min.css?')?>">
        <link rel="stylesheet" href="<?=$lp('CSS','>bootstrap-responsive.min.css?')?>">
        <link rel="stylesheet" href="<?=$lp('CSS','>bootstrap-select.min.css?')?>">
        <link rel="stylesheet" href="//cdnjs.cloudflare.com/ajax/libs/x-editable/1.4.4/bootstrap-editable/css/bootstrap-editable.css">
        <link rel="stylesheet" href="<?=$lp('CSS','>fw.css?')?>">

        <script src="<?=$lp('JS','>modernizr-2.6.2-respond-1.1.0.min.js?')?>"></script>
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
    <script src="<?=$lp('JS','>bootstrap.min.js?')?>"></script>
    <script src="<?=$lp('JS','>bootstrap-select.min.js?')?>"></script>
    <script src="//cdnjs.cloudflare.com/ajax/libs/x-editable/1.4.4/bootstrap-editable/js/bootstrap-editable.min.js"></script>

    <script><?php $this->JSBase(); ?></script>

    <?php $this->Unstack('ajax'); ?>

	</body>
</html>

