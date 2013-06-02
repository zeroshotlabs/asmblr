
$('a.set-domain').editable({mode:'popup',placement:'right',inputclass:'input-large'});
$('a.set-baseurl').editable({mode:'popup',placement:'right',inputclass:'input-xlarge',validate:function(v){return '';}});

$('a.cm_tab_link','#site_tabs').click(function (e) {
	e.preventDefault();
	$(this).tab('show');
	cm_init($(e.currentTarget).data('taid'));
});

$('a.tab_link','#site_tabs').click(function (e) {
	e.preventDefault();
	$(this).tab('show');
	ajf_directive_grid('site');
});

