
function aapi_method2url( Method )
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
		case 'site_set_status':
			url = '<?=$lr('site_set_status')?>';
			break;
		case 'page_create':
			url = '<?=$lr('page_create')?>';
			break;
		case 'page_set_name':
			url = '<?=$lr('page_set_name')?>';
			break;
		case 'page_set_path':
			url = '<?=$lr('page_set_path')?>';
			break;
		default:
			url = 'unknown';
	}

	return url;
}

function aapi_set( Method,FormData )
{
	url = aapi_method2url(Method);

	$.ajax({url:url,data:FormData})
	 .done(function(data){
		 if( data.Status === true )
			 $('#aapi_msg').html('saved');
		 else
			 $('#aapi_msg').html(data.Msg);
    }).fail(function(data) { $('#aapi_msg').html('connection error'); });
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
		t[$('#'+Name)[0].name] = $('#'+Name).val();
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

$(function() {

	headers = {};
   <?php if( $page->LoggedIn ): ?>
    headers = {'X-ASMBLR-USER': '<?=$_SESSION['Account']['_id']?>',
 	             'X-ASMBLR-PW': '<?=$_SESSION['Account']['Password']?>'};
   <?php endif; ?>

   // note: if using the jquery mobile native form plugin, this is ignored so
   // hidden fields would need to be used
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


$(document).on('pageinit','#jqm_site', function() {
	
	$('#nav-panel-btn').on( "click",function(e) {
		$("#nav-panel").panel( "open" );
	});
	
	init_field('BaseURL','site_set_baseurl');
	init_field('Domain','site_set_domain');
	
	$("#SiteStatus").bind("change",function(e) {
		aapi_set('site_set_status',{Status:$(e.currentTarget).val()});
	});
	
	$('#NewPagePath,#NewPageName').on('keyup',function(e){
		if( $('#NewPagePath').val() !== '' && $('#NewPageName').val() !== '' )
		{
			$('#CreatePage').addClass('btn-warning');
			$('#ResetCreatePage').addClass('btn-primary');
			if( event.keyCode == 13 )
				$('#CreatePage').click();
		}
	});
	
	$('#CreatePage').on('click',function(e){
		e.preventDefault();
		t = {Name:$('#NewPageName').val(),Path:$('#NewPagePath').val()};
		
		$.ajax({url:aapi_method2url('page_create'),data:t})
		 .done(function(data){
			 if( data.Status === true )
				 $.mobile.changePage('<?=$lp('Page')?>'+data.Data._id);
			 else
				 $('#aapi_msg').html(data.Msg);
	    }).fail(function(data) { $('#aapi_msg').html('connection error'); });

		$('#CreatePage').removeClass('btn-warning');
		$('#ResetCreatePage').removeClass('btn-primary');
	});

	$('#ResetCreatePage').on('click',function(e){
		e.preventDefault();
		$('#NewPageName').val('');
		$('#NewPagePath').val('');
		$('#create_page_panel').panel('close');
	});		
});



