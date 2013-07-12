
$('a.set-domain').editable({mode:'popup',placement:'right',inputclass:'input-large'});
$('a.set-baseurl').editable({mode:'popup',placement:'right',inputclass:'input-xlarge',validate:function(v){return '';}});

$('#confirm-del').on('click',function( e ) {
	pk = $(e.currentTarget).data('pk');
	$.ajax({ url:'<?=$lr('site_delete',">{$S['_id']}")?>',data:{},
		success: function(data){ window.location = '<?=$lp('Home')?>'; }})
	.fail(function(){ console.log('connection error');})
});

$('a.routine_tab_link','#site_tabs').click(function (e) {
	e.preventDefault();
	$(this).tab('show');
	cm_init($(e.currentTarget).data('taid'));
});

$('a.raw_tab_link','#site_tabs').click(function (e) {
	e.preventDefault();
	$(this).tab('show');
	cm_init($(e.currentTarget).data('taid'));
});

$('a.tab_link','#site_tabs').click(function (e) {
	e.preventDefault();
	$(this).tab('show');
	ajf_directive_grid('site');
});

