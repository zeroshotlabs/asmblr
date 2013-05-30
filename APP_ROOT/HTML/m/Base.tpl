<!DOCTYPE html>
<!--[if lt IE 7]>      <html class="no-js lt-ie9 lt-ie8 lt-ie7"> <![endif]-->
<!--[if IE 7]>         <html class="no-js lt-ie9 lt-ie8"> <![endif]-->
<!--[if IE 8]>         <html class="no-js lt-ie9"> <![endif]-->
<!--[if gt IE 8]><!--> <html class="no-js"> <!--<![endif]-->
<head>
<meta charset="utf-8">
<meta name="viewport" content="initial-scale=1.0, user-scalable=no">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black">
<title><?=$this($page->Title)?></title>
<meta name="description" content="<?=$this($page->Description)?>">

<link rel="stylesheet" href="https://d10ajoocuyu32n.cloudfront.net/mobile/1.3.1/jquery.mobile-1.3.1.min.css">
<link rel="stylesheet" href="<?=$lp('CSS','>bootstrap.min.css?')?>">
<link rel="stylesheet" href="<?=$lp('CSS','>bootstrap-responsive.min.css?')?>">
</head>


<body>
<div data-role="page" id="#<?=$page->PageID?>">
    <?php $this->Page(); ?>

    <?php $this->m_Panels(); ?>
</div>

<script src="//code.jquery.com/jquery-latest.js"></script>
<script src="//code.jquery.com/ui/1.10.2/jquery-ui.js"></script>
<script src="https://d10ajoocuyu32n.cloudfront.net/mobile/1.3.1/jquery.mobile-1.3.1.min.js"></script>
<script src="<?=$lp('JS','>bootstrap.min.js?')?>"></script>
<script><?php $this->m_JSBase(); ?></script>

<?php $this->Unstack('ajax'); ?>

</body>
</html>

