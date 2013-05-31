$(function() {
	$( "#nav-panel-btn" ).on( "click", function(e) {
		$("#nav-panel").panel( "open" );
	});

	headers = {};
   <?php if( $page->LoggedIn ): ?>
    headers = {'X-ASMBLR-USER': '<?=$_SESSION['Account']['_id']?>',
 	             'X-ASMBLR-PW': '<?=$_SESSION['Account']['Password']?>'};
   <?php endif; ?>

   // this doesn't work when using the form plugin, thus it doesn't work
   // with jquery mobile - we set these as hidden fields in the forms themselves
    data = {};
   <?php if( $lp->Current('Site',TRUE) ): ?>
    data['Site_id'] = '<?=\fw\Request::Path(-1)?>';
   <?php elseif( $lp->Current('Page',TRUE) ): ?>
    data['Page_id'] = '<?=\fw\Request::Path(-1)?>';
    <?php elseif( $lp->Current('Template',TRUE) ): ?>
    data['Template_id'] = '<?=\fw\Request::Path(-1)?>';
   <?php endif;?>

    $.ajaxSetup({headers:headers,data:data,dataType:'json',type:'POST'});
});

function aapi_set( Method,FormData )
{
	switch( Method )
	{
		case 'site_set_baseurl':
			url = '<?=$lr('site_set_baseurl')?>';
			break;
		case 'site_set_domain':
			url = '<?=$lr('site_set_domain')?>';
			break;
		case 'site_set_routine':
			url = '<?=$lr('site_set_routine')?>';
			break;
		default:
			url = 'unknown';
	}
	
	t = $.ajax({url:url,data:FormData})
		 .done(function(data){
			 if( data.Status === true )
				 $('#aapi_msg').html = 'saved';
			 else
				 $('#aapi_msg').html = 'error';
		 }).fail(function(data) { $('#aapi_msg').html = 'connection error'; });
}

function init_field( Name,Method )
{
	$('#'+Name).on('keyup',function(e){
		$('#Save'+Name).addClass('btn-warning');
		$('#Reset'+Name).addClass('btn-primary');
		if( event.keyCode == 13 )
			$('#Save'+Name).click();
	});

	$('#Save'+Name).on('click',function(e){
		e.preventDefault();
		t = {};
		t[Name] = $('#'+Name).val();
		aapi_set(Method,t);
		$('#'+Name)[0].defaultValue = $('#'+Name).val();
		$('#Save'+Name).removeClass('btn-warning');
		$('#Reset'+Name).removeClass('btn-primary');
	});

	$('#Reset'+Name).on('click',function(e){
		e.preventDefault();
		$('#'+Name).val($('#'+Name)[0].defaultValue);
		$('#Save'+Name).removeClass('btn-warning');
		$('#Reset'+Name).removeClass('btn-primary');
	});	
}

function init_cm( cmEditor,Name,Method )
{
	cmEditor.on('change',function(e){
		$('#Save'+Name).addClass('btn-warning');
		$('#Reset'+Name).addClass('btn-primary');
	});
	
	$('#Save'+Name).on('click',function(e){
		e.preventDefault();
		t = {};
		t[Name] = cmEditor.getValue();
		aapi_set(Method,t);
		$('#'+Name)[0].defaultValue = cmEditor.getValue();
		$('#Save'+Name).removeClass('btn-warning');
		$('#Reset'+Name).removeClass('btn-primary');
	});
	
	$('#Reset'+Name).on('click',function(e){
		e.preventDefault();
		cmEditor.setValue($('#'+Name)[0].defaultValue);
		$('#Save'+Name).removeClass('btn-warning');
		$('#Reset'+Name).removeClass('btn-primary');
	});		
	
}

$(document).on('pageinit','#jqm_site', function() {
	init_field('BaseURL','site_set_baseurl');
	init_field('Domain','site_set_domain');
});


