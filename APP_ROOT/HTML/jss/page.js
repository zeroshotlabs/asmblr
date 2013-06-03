
$('a.set-path').editable({mode:'popup',placement:'right',inputclass:'input-xlarge'});
$('a.set-name').editable({mode:'popup',placement:'right'});

$('#confirm-del').on('click',function( e ) {
	pk = $(e.currentTarget).data('pk');
	$.ajax({ url:'<?=$lr('page_delete',">{$P['_id']}")?>',data:{},
		success: function(data){ window.location = '<?=$lp('Site',">{$P['Site_id']}")?>'; }})
	.fail(function(){ console.log('connection error');})
});

$('a.cm_tab_link','#page_tabs').click(function (e) {
	e.preventDefault();
	$(this).tab('show');
	cm_init($(e.currentTarget).data('taid'));
});

$('a.tab_link','#page_tabs').click(function (e) {
	e.preventDefault();
	$(this).tab('show');
	ajf_directive_grid('site');
});


checkhash(window.location.hash);

$(window).on('hashchange',function(e){
	checkhash(window.location.hash);
});

function checkhash( hash )
{
	if( hash === '#directives_tab' )
		$('a.tab_link','#page_tabs').click();
	else if( hash === '#routine_tab' )
		$('a.cm_tab_link','#page_tabs').click();
}


