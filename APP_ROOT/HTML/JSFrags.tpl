
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
    $('a.set-domain').editable({mode:'popup',placement:'right',inputclass:'input-large'});
	$('a.set-baseurl').editable({mode:'popup',placement:'right',inputclass:'input-xlarge',validate:function(v){return '';}});

	ajfDirectives();

});
</script>
<?php $this->JSDirectives(array('prefix'=>'site')); ?>



@@@JSPage
<script>
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

	ajfDirectives();
});
</script>
<?php $this->JSDirectives(array('prefix'=>'page')); ?>





@@@JSLeftNav
<script>
$(document).ready(function()
{
	<?php if( $page->ActiveNav === 'Site' ): ?>
	aapi_status('site-status','site','site_set_status');
    <?php elseif( $page->ActiveNav === 'Page' ): ?>
	aapi_status('page-status','page','page_set_status');
    <?php endif; ?>

	$('button.new-page').editable({display:false,showbuttons:'bottom',placement:'bottom',inputclass:'input-large',
		success: function(r,nv){
		    if( r.Status === false )
		        return r.Msg;
	        else
		        window.location = '<?=$lp('Page')?>'+r.Data._id;
		}
	});

	$('button.new-template').editable({display:false,showbuttons:'bottom',placement:'bottom',inputclass:'input-large',
		success: function(r,nv){
		    if( r.Status === false )
		        return r.Msg;
	        else
		        window.location = '<?=$lp('Template')?>'+r.Data._id;
		}
	});

});
</script>



@@@JSEditRoutine
<script>
<?php /*
    	onCursorActivity: function() {
    	    editor.matchHighlight("CodeMirror-matchhighlight",2,'<?=\fw\Struct::Get('SearchTerms',$_GET)?>');
    	},
*/ ?>


$(document).on('pageshow','#jqm_site', function() {
	editor = CodeMirror.fromTextArea(document.getElementById("Routine"),
	{
	    lineNumbers: true,
	    matchBrackets: true,
	    mode: "text/x-php",
	    indentUnit: 4,
	    extraKeys: {"Ctrl-S":function(instance){
		    t = {};
		    t[instance.getTextArea().id] = instance.getValue();
		    aapi_set($(instance.getTextArea()).data('method'),t);
		    instance.getTextArea().defaultValue = instance.getValue();
			$('#Save'+instance.getTextArea().id).removeClass('btn-warning');
			$('#Reset'+instance.getTextArea().id).removeClass('btn-primary');

	    }}
	});
	init_cm(editor,'Routine',$(editor.getTextArea()).data('method'));
});

</script>




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

