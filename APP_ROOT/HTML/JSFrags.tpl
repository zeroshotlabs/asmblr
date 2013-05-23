
@@@JSLogin
<script>
$(document).ready(function()
{
    $('#login').on('submit','#login-form',function( e ) {
		$form = $(e.target);
		e.preventDefault();
	    $.post($form.attr('action'),$form.serialize(),function(data){
		    if( data.Status === true )
			    window.location.href = '/';
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
			    window.location.href = '/site/'+data.Data._id;
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

	$('a.new-page').editable({value:'',placement:'bottom',inputclass:'input-large',
		success: function(r,nv){
		    if( r.Status === false )
		        return r.Msg;
	        else
		        window.location = '/page/'+r.Data._id;
		}
	});

	ajfDirectives();
});
</script>
<?php $this->JSDirectives(array('prefix'=>'site')); ?>


@@@JSPage
<script>
$(document).ready(function()
{
	$('a.set-status').editable({placement:'bottom',source:'<?=$lr('util_site_statuses')?>'});
    $('a.set-path').editable({mode:'inline',inputclass: 'input-xlarge'});
	$('a.set-name').editable({mode:'inline'});
	$('a.set-routine').editable({mode:'inline',inputclass: 'input-large'});

	$('a.new-page').editable({value:'',placement:'bottom',inputclass:'input-large',
		success: function(r,nv){
		    if( r.Status === false )
		        return r.Msg;
	        else
		        window.location = '/page/'+r.Data._id;
		}
	});

	ajfDirectives();
});
</script>
<?php $this->JSDirectives(array('prefix'=>'page')); ?>



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

