
@@@JSLogin
<script>
$(document).ready(function()
{
    $('#login').on('submit','#login-form',function( e ) {
		$form = $(e.target);
		e.preventDefault();
	    $.post($form.attr('action'),$form.serialize(),function(data){
		    if( data.Status === true )
			    window.location.href = '<?=$lp('Home')?>';
		    else
		    	$('.label-important',e.delegateTarget).html(data.Msg);
	    },'json').fail(function(){ $('.label-important',e.delegateTarget).html('Please complete the form.'); });
    });

    $('#register').on('submit','#register-form',function( e ) {
		$form = $(e.target);
		e.preventDefault();
	    $.post($form.attr('action'),$form.serialize(),function(data){
		    if( data.Status === true )
			    window.location.href = '<?=$lp('Home')?>';
		    else
		    	$('.label-important',e.delegateTarget).html(data.Msg);
	    },'json').fail(function(){ $('.label-important',e.delegateTarget).html('Please complete the form.'); });
    });

});
</script>


@@@JSCreateSite
<script>
$(document).ready(function()
{
    $('#createsite').on('submit','#createsite-form',function( e ) {
		$form = $(e.target);
		e.preventDefault();
		$.ajax({
			type: "POST",url: $form.attr('action'),dataType: 'json',data: $form.serialize(),
			success: function(data){console.log(data);
		    if( data.Status === true )
			    window.location.href = '<?=$lp('Site')?>'+data.Data._id;
		    else
		    	$('.label-important',e.delegateTarget).html(data.Msg);
			}})
		.fail(function(){ $('.label-important',e.delegateTarget).html('Please complete the form.'); });
    });
});
</script>


@@@JSSite
<script>
$(document).ready(function()
{
	$('a.set-status').editable({placement:'bottom',source:'<?=$lr('util_site_statuses')?>'});
    $('a.set-domain').editable({mode:'inline',inputclass: 'input-xlarge'});
	$('a.set-baseurl').editable({mode:'inline',inputclass: 'input-xlarge'});
	$('a.set-routine').editable({mode:'inline',inputclass: 'input-large'});

	$('a.new-page').editable({value:{Path:'',Name:''},placement:'bottom',inputclass:'input-large',
		success: function(r,nv){
		    if( r.Status === false )
		        return r.Msg;
	        else
		        window.location = '<?=$lp('Page')?>'+r.Data._id;
		}
	});

	$('a.new-template').editable({value:'',placement:'bottom',inputclass:'input-large',
		success: function(r,nv){
		    if( r.Status === false )
		        return r.Msg;
	        else
		        window.location = '<?=$lp('Template')?>'+r.Data._id;
		}
	});

	ajfDirectives();

});
</script>
<?php $this->JSDirectives(array('prefix'=>'site')); ?>

@@@JSEditRoutine
<script src="<?=$ls('/jslib/codemirror-3.13/lib/codemirror.js')?>"></script>
<script src="<?=$ls('/jslib/codemirror-3.13/addon/edit/matchbrackets.js')?>"></script>
<script src="<?=$ls('/jslib/codemirror-3.13/mode/htmlmixed/htmlmixed.js')?>"></script>
<script src="<?=$ls('/jslib/codemirror-3.13/mode/xml/xml.js')?>"></script>
<script src="<?=$ls('/jslib/codemirror-3.13/mode/javascript/javascript.js')?>"></script>
<script src="<?=$ls('/jslib/codemirror-3.13/mode/css/css.js')?>"></script>
<script src="<?=$ls('/jslib/codemirror-3.13/mode/clike/clike.js')?>"></script>
<script src="<?=$ls('/jslib/codemirror-3.13/mode/php/php.js')?>"></script>

<link rel="stylesheet" href="<?=$ls('/jslib/codemirror-3.13/lib/codemirror.css')?>" />
<script>
<?php /*
    	onCursorActivity: function() {
    	    editor.matchHighlight("CodeMirror-matchhighlight",2,'<?=\fw\Struct::Get('SearchTerms',$_GET)?>');
    	},
*/ ?>

var editor = CodeMirror.fromTextArea(document.getElementById("routine_body"),
{
    lineNumbers: true,
    matchBrackets: true,
    mode: "text/x-php",
    indentUnit: 4,
    enterMode: "keep",
    tabMode: "shift"
});

editor.on('change',function(){$('#save_routine_body').removeAttr('disabled');});

$('#save_routine_body').on('click',function(e){
	$.ajax({ url:'<?=$lr('site_set_routine')?>',data:{Routine:editor.getValue()},
		success: function(data){
	    if( data.Status === true )
	    {
	    	$('#routine_msg').html('saved');
	    	$(e.currentTarget).attr('disabled','disabled');
	    }
	    else
	    	$('#routine_msg').html(data.Msg);
		}})
	.fail(function(){ $('#routine_msg').html('save error'); });
});
</script>


@@@JSPage
<script>
$(document).ready(function()
{
	$('a.set-status').editable({placement:'bottom',source:'<?=$lr('util_site_statuses')?>'});
    $('a.set-path').editable({mode:'inline',inputclass: 'input-xlarge'});
	$('a.set-name').editable({mode:'inline'});
	$('a.set-routine').editable({mode:'inline',inputclass: 'input-large'});

	$('#confirm-del').on('click',function( e ) {
		pk = $(e.currentTarget).data('pk');
		$.ajax({ url:'<?=$lr('page_delete',">{$P['_id']}")?>',data:{},
			success: function(data){ window.location = '<?=$lp('Site',">{$P['Site_id']}")?>'; }})
		.fail(function(){ console.log('connection error');})
	});

	ajfDirectives();
});
</script>
<?php $this->JSDirectives(array('prefix'=>'page')); ?>



@@@JSTemplate
<script>
$(document).ready(function()
{
	$('a.set-name').editable({mode:'inline'});
	$('a.set-routine').editable({mode:'inline',inputclass: 'input-large'});
	$('a.set-body').editable({mode:'inline',inputclass: 'input-large'});

	$('#confirm-del').on('click',function( e ) {
		pk = $(e.currentTarget).data('pk');
		$.ajax({ url:'<?=$lr('template_delete',">{$T['_id']}")?>',data:{},
			success: function(data){ window.location = '<?=$lp('Site',">{$T['Site_id']}")?>'; }})
		.fail(function(){ console.log('connection error');})
	});
});
</script>


@@@JSDirectives
<script>
$(document).ready(function()
{
	$('#dir-table-container').editable({source:'<?=$lr('util_dir_names')?>',
		params:NormDirParams,selector:'a.editable-click',
	    url:'<?=$lr("{$prefix}_set_directive")?>'
	});

	$('#dir-table-container').on('click','tr a.del-directive',function( e ) {
		pk = $(e.currentTarget).data('pk');
		$.ajax({ url:'<?=$lr("{$prefix}_del_directive")?>',data:{D_id:pk},
			success: function(data){ $('#'+pk).fadeOut(100,function(){ $('#'+pk).remove();})}})
		.fail(function(){ console.log('connection error');})
	});

	$('#dir-table-container').on('click','tr a.cp-directive',function( e ) {
		pk = $(e.currentTarget).data('pk');
		$.ajax({ url:'<?=$lr("{$prefix}_cp_directive")?>',data:{D_id:pk},
			success: function(data){
				t = $('#'+pk).clone();
				t.attr('id',data.Data._id);
				$('a[data-pk]',t).attr('data-pk',data.Data._id);
				t.insertAfter($('#'+pk));
			}})
		.fail(function(){console.log('connection error');})
	});

	$('#directives').on('submit','#set_directive_form',function( e ) {
		$form = $(e.target);
		e.preventDefault();
		$.ajax({ url:$form.attr('action'),data:NormParams($form.serializeArray()),
			success: function(data){
		    if( data.Status === true )
		    {
		    	$('.label-success',e.delegateTarget).html(data.Msg);
		    	$('.label-important',e.delegateTarget).html('');
		    	ajfDirectives();
		    }
		    else
		    {
		    	$('#diralert').html(data.Msg);
		    }
			}})
		.fail(function(){ $('#diralert').css('display','block').html('Please complete the form.'); });
    });
});
</script>

