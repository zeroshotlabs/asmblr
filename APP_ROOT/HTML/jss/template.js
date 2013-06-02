
$(document).ready(function()
{
	$('a.set-name').editable({mode:'popup',placement:'right'});
	
//	$('a.set-routine').editable({mode:'inline',inputclass: 'input-large'});
//	$('a.set-body').editable({mode:'inline',inputclass: 'input-large'});

	$('#confirm-del').on('click',function( e ) {
		pk = $(e.currentTarget).data('pk');
		$.ajax({ url:'<?=$lr('template_delete',">{$T['_id']}")?>',data:{},
			success: function(data){ window.location = '<?=$lp('Site',">{$T['Site_id']}")?>'; }})
		.fail(function(){ console.log('connection error');})
	});
	
	$('#template_edit a:last').tab('show');
	
	
	  <?php $this->jss_editroutine(); ?>	
});

