
	$('a.set-name').editable({mode:'popup',placement:'right'});
	
	$('#confirm-del').on('click',function( e ) {
		pk = $(e.currentTarget).data('pk');
		$.ajax({ url:'<?=$lr('template_delete',">{$T['_id']}")?>',data:{},
			success: function(data){ window.location = '<?=$lp('Site',">{$T['Site_id']}")?>'; }})
		.fail(function(){ console.log('connection error');})
	});

	$('a.tab_link','#template_cm').click(function (e) {
		e.preventDefault();
		$(this).tab('show');
		cm_init($(e.currentTarget).data('taid'));
	});
