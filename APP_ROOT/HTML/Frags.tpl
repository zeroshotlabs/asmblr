
@@@Error404
<h1>Not Found</h1>
<div>
    <p>Ten-four, that's a four-oh-four.</p>
</div>


@@@Error500
<h1>Server Error</h1>


@@@Footer
<div class="page-footer">
    <div class="footer-left-side">
        <p>Built by <a href="http://www.stackware.com/">Stackware PHP Website Development</a>.</p>
        <p>asmblr is a trademark of Stackware, LLC.</p>
    </div>
    <div class="footer-right-side">
        <p>Framewire <a href="http://www.framewire.org/">Web Application PHP Framework</a></p>
        <p><?=round((microtime(TRUE)-START_TIME)*1000,1)?>ms</p>
    </div>
</div>


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

	$('a.new-page').editable({value:'',placement:'bottom',inputclass:'input-large'});
});
</script>
<?php $this->JSDirectives(); ?>


@@@JSDirectives
<script>
$(document).ready(function()
{
	editable_name = {placement:'right',source:'<?=$lr('util_dir_names')?>',params:NormDirParams};
	editable_key = {params:NormDirParams};
	editable_value = {inputclass:'input-xlarge',params:NormDirParams};

	$('#directives-sortable a.set-directive-name').editable(editable_name);
	$('#directives-sortable a.set-directive-key').editable(editable_key);
	$('#directives-sortable a.set-directive-value').editable(editable_value);

	$('#directives-sortable').on('click','tr a.del-directive',function( e ) {
		pk = $(e.currentTarget).data('pk');
		$.ajax({ url:'<?=$lr('site_del_directive')?>',data:{D_id:pk},
			success: function(data){ $('#'+pk).fadeOut(100,function(){ $('#'+pk).remove();})}})
		.fail(function(){ console.log('connection error');})
	});

	$('#directives-sortable').on('click','tr a.cp-directive',function( e ) {
		pk = $(e.currentTarget).data('pk');
		$.ajax({ url:'<?=$lr('site_cp_directive')?>',data:{D_id:pk},
			success: function(data){
				t = $('#'+pk).clone();
				t.attr('id',data.Data._id);
				$('a[data-pk]',t).attr('data-pk',data.Data._id);
				t.insertAfter($('#'+pk));

				$('a.set-directive-name',t).editable(editable_name);
				$('a.set-directive-key',t).editable(editable_key);
				$('a.set-directive-value',t).editable(editable_value);
			}})
		.fail(function(){console.log('connection error');})
	});

    $('#directives-sortable').sortable({forceHelperSize:true,opacity:.9,handle:'a.handle',placeholder:'ui-state-highlight',axis:'y',
        helper:function(e,ui) {
            ui.children().each(function() {
                $(this).width($(this).width());
            });
            return ui;
        },
        update:function(e,ui) {
            itemid = ui.item.attr('id');
            nextid = ui.item.next().attr('id');
            $.ajax({ url:'<?=$lr('site_mv_directive')?>',
                    data:{D_id:itemid,NextD_id:nextid}});
        }});

	$('#directives').on('submit','#set_directive_form',function( e ) {
		$form = $(e.target);
		e.preventDefault();
		$.ajax({ url:$form.attr('action'),data:NormParams($form.serializeArray()),
			success: function(data){
		    if( data.Status === true )
		    {
		    	$('.label-success',e.delegateTarget).html(data.Msg);
		    	$('.label-important',e.delegateTarget).html('');
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


