
$(document).ready(function()
{
    $('a.set-path').editable({mode:'popup',placement:'right',inputclass:'input-xlarge'});
	$('a.set-name').editable({mode:'popup',placement:'right'});

	$('#confirm-del').on('click',function( e ) {
		pk = $(e.currentTarget).data('pk');
		$.ajax({ url:'<?=$lr('page_delete',">{$P['_id']}")?>',data:{},
			success: function(data){ window.location = '<?=$lp('Site',">{$P['Site_id']}")?>'; }})
		.fail(function(){ console.log('connection error');})
	});

	ajf_directive_grid('page');
});

