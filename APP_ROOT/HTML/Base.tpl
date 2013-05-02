<!DOCTYPE html>
<!--[if lt IE 7]>      <html class="no-js lt-ie9 lt-ie8 lt-ie7"> <![endif]-->
<!--[if IE 7]>         <html class="no-js lt-ie9 lt-ie8"> <![endif]-->
<!--[if IE 8]>         <html class="no-js lt-ie9"> <![endif]-->
<!--[if gt IE 8]><!--> <html class="no-js"> <!--<![endif]-->
    <head>
        <meta http-equiv="X-UA-Compatible" content="IE=edge,chrome=1">
        <title><?=$this($page->Title)?></title>
        <meta name="description" content="<?=$this($page->Description)?>">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <script src="<?=$lp('JS','>modernizr-2.6.2-respond-1.1.0.min.js?')?>"></script>
        <link rel="stylesheet" href="<?=$lp('CSS','>bootstrap.min.css?')?>">
        <link rel="stylesheet" href="<?=$lp('CSS','>bootstrap-responsive.min.css?')?>">
        <link rel="stylesheet" href="<?=$lp('CSS','>bootstrap-editable.css?')?>">
        <link rel="stylesheet" href="//code.jquery.com/ui/1.10.2/themes/smoothness/jquery-ui.css">
        <link rel="stylesheet" href="<?=$lp('CSS','>fw.css?')?>">
    </head>

    <body>
    <!--[if lt IE 7]>
        <p class="chromeframe">You are using an <strong>outdated</strong> browser. Please <a href="http://browsehappy.com/">upgrade your browser</a> or <a href="http://www.google.com/chromeframe/?redirect=true">activate Google Chrome Frame</a> to improve your experience.</p>
    <![endif]-->

    <div class="container-fluid">
        <?php $this->Header(); ?>
    </div>

	<div id="spinner" class="spinner">
        <img id="img-spinner" src="<?=$ls('/img/ajax-loader.gif')?>" alt="Loading"/>
    </div>

    <section class="container-fluid">
        <div class="row-fluid">
            <article class="span9">
                <?php $this->Article(); ?>
            </article>
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

    <script src="//code.jquery.com/jquery-latest.js"></script>
    <script src="//code.jquery.com/ui/1.10.2/jquery-ui.js"></script>
    <script src="<?=$lp('JS','>jquery.cookie.js?')?>"></script>
    <script src="<?=$lp('JS','>bootstrap.min.js?')?>"></script>
    <script src="<?=$lp('JS','>bootstrap-editable.min.js?')?>"></script>


<script>
// post a form and process returned ValidationReport and reload fragments
function submit_form( e )
{
	$form = $(e.target);

	e.preventDefault();

    $.post($form.attr('action'),$form.serialize(),function(data){
    	$('.label-important',e.delegateTarget).html(data.Msg);

        if( data.Status == false )
        {
            tid = e.target.id;

            $.each( jQuery.parseJSON(data.Data), function(k,v) {
                if( v )
                    $('#'+tid+' input[name="'+k+'"]').removeClass('error');
                else
                	$('#'+tid+' input[name="'+k+'"]').addClass('error');
            });
        }
        else
        {
            reload_tab(e.delegateTarget.id);
            reload_aside();
            reload_buttons();
        }
    },'json');
}

function NormParams( p )
{
	n = {};
	lastK = '';
	$.each(p,function(k,v) {
		if( k == 'name' )
			lastK = v;
		else if( k == 'value' )
		{
		    n[lastK] = v;
		}
		else
			n[k] = v;
	});

   <?php if( $lp->Current('Site',TRUE) ): ?>
    n['Site_id'] = '<?=\asm\Request::Bottom()?>';
   <?php endif;?>

    return n;
}

function DirectiveParams( p )
{
	p2 = {};
	if( p.name === 'Name' )
	{
	    p2['Name'] = p.value;
	    p2['Key'] = $('#'+p.pk+' a.set-directive-key').text();
	    p2['Value'] = $('#'+p.pk+' a.set-directive-value').text();
	}
	else if( p.name === 'Key' )
	{
	    p2['Name'] = $('#'+p.pk+' a.set-directive-name').text();
	    p2['Key'] = p.value;
	    p2['Value'] = $('#'+p.pk+' a.set-directive-value').text();
	}
	else if( p.name === 'Value' )
	{
	    p2['Name'] = $('#'+p.pk+' a.set-directive-name').text();
	    p2['Key'] = $('#'+p.pk+' a.set-directive-key').text();
	    p2['Value'] = p.value;
	}

    p2['D_id'] = p['pk'];

   <?php if( $lp->Current('Site',TRUE) ): ?>
    p2['Site_id'] = '<?=\asm\Request::Bottom()?>';
   <?php endif;?>

    return p2;
}


$(document).ready(function()
{
	if( location.hash )
        window.scrollTo(0, 0);

    $(document).ajaxStart(function() { $('#spinner').show(); });
    $(document).ajaxComplete(function() { window.setTimeout(function() { $('#spinner').hide(); },200)});

    $.ajaxSetup({headers: {'X-ASMBLR-USER': $.cookie('aaid'),'X-ASMBLR-PW': $.cookie('token')}});

	// setup editable defaults
	$.fn.editable.defaults.mode = 'inline';
	$.fn.editable.defaults.onblur = 'ignore';
	$.fn.editable.defaults.send = 'always';
	$.fn.editable.defaults.success = function(r,nv){ if( r.Status === false ) return r.Msg; };
	$.fn.editable.defaults.validate = function(v){if($.trim(v)=='') return 'Required.';};
	$.fn.editable.defaults.params = function(p){return NormParams(p)};

    $('aside').on('click','a.iconlink',function(e) {
        $('#rightaside').load($(e.currentTarget).attr('href'),reload_aside);
    	e.preventDefault();
    });

    // handle form submits  (JSON - /json/)
    $('#login').on('submit','#login',submit_form);
});

</script>



	<?php $this->Unstack('ajax'); ?>

	</body>
</html>

