
$(document).ready(function()
{
    $('a.set-domain').editable({mode:'popup',placement:'right',inputclass:'input-large'});
	$('a.set-baseurl').editable({mode:'popup',placement:'right',inputclass:'input-xlarge',validate:function(v){return '';}});

	ajf_directive_grid('site');
});
