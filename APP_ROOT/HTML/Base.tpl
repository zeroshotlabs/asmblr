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
    <script src="//cdnjs.cloudflare.com/ajax/libs/x-editable/1.4.4/bootstrap-editable/js/bootstrap-editable.min.js"></script>

<script>
// because JS confuses itself
var NormParams = function( p )
{
	n = {};
	// from x-editable, simple key/value pairs
	if( typeof p.name !== 'undefined' )
	{
	    lastK = '';
    	$.each(p,function(k,v) {
    		if( k == 'name' )
    			lastK = v;
    		else if( k == 'value' )
    		    n[lastK] = v;
    		else
    			n[k] = v;
    	});
	}
	// from serializeArray forms (i.e. new directive form)
	else
	{
    	$.each(p,function(k,v) {
    	    n[v['name']] = v['value'];
    	});
	}

    return n;
}

var NormDirParams = function( p )
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

    return p2;
}

function ajfDirectives()
{
	$('#dir-table-container').load('<?=$lp('ajfHandler','>directive_table')?>',
	function(r,s,x)
	{
        $('#directives-sortable').sortable({forceHelperSize:true,opacity:.9,handle:'a.handle',placeholder:'ui-state-highlight',axis:'y',
            helper:function(e,ui) {
                ui.children().each(function() {
                    $(this).width($(this).width());
                });
                return ui;
            },
            update:function(e,ui) {
                itemid = ui.item.attr('id');
                nextid = ui.item.next().attr('id');
                $.ajax({ url:'<?=$lr('site_mv_directive')?>',
                        data:{D_id:itemid,NextD_id:nextid}}).done(function(){ajfDirectives();});
            }});
	});
}

// validate: function(v){return '';}
$(document).ready(function()
{
	if( location.hash )
        window.scrollTo(0, 0);

    $(document).ajaxStart(function() { $('#spinner').show(); });
    $(document).ajaxComplete(function() { window.setTimeout(function() { $('#spinner').hide(); },200)});

    headers = {'X-ASMBLR-USER': '<?=$_SESSION['Account']['_id']?>',
 	       'X-ASMBLR-PW': '<?=$_SESSION['Account']['Password']?>'};

    data = {};
   <?php if( $lp->Current('Site',TRUE) ): ?>
    data['Site_id'] = '<?=\fw\Request::Path(-1)?>';
   <?php elseif( $lp->Current('Page',TRUE) ): ?>
    data['Page_id'] = '<?=\fw\Request::Path(-1)?>';
   <?php endif;?>

    $.ajaxSetup({headers:headers,data:data,dataType:'json',type:'POST'});

	// setup editable defaults
	$.fn.editable.defaults.mode = 'popup';
	$.fn.editable.defaults.onblur = 'ignore';
	$.fn.editable.defaults.send = 'always';
	$.fn.editable.defaults.placement = 'top';
	$.fn.editable.defaults.success = function(r,nv){ if( r.Status === false ) return r.Msg; };
	$.fn.editable.defaults.validate = function(v){if($.trim(v)=='') return 'Required.';};
	$.fn.editable.defaults.params = NormParams;

//    $('aside').on('click','a.iconlink',function(e) {
//        $('#rightaside').load($(e.currentTarget).attr('href'),reload_aside);
//    	e.preventDefault();
//    });

    // handle form submits  (JSON - /json/)
//    $('#login').on('submit','#login',submit_form);
});

</script>



	<?php $this->Unstack('ajax'); ?>

	</body>
</html>

