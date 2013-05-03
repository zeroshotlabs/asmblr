
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
		    {
		    	$.cookie('aaid',data.Data._id);
		    	$.cookie('token',data.Data.Password);
			    window.location.href = '/';
		    }
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
	$('a.set-domain').editable();
	$('a.set-status').editable({source:'<?=$lr('util_site_statuses')?>'});
	$('a.set-baseurl').editable({validate: function(v){return '';}});
	$('a.set-routine').editable({inputclass: 'input-large',validate: function(v){return '';}});

	editable_name = {mode:'popup',source:'<?=$lr('util_dir_names')?>',placement: 'right',params:function(p){return DirectiveParams(p)}};
	editable_key = {mode:'popup',params:function(p){return DirectiveParams(p)}};
	editable_value = {mode:'popup',inputclass:'input-xlarge',params:function(p){return DirectiveParams(p)}};

	$('#directives-sortable a.set-directive-name').editable(editable_name);
	$('#directives-sortable a.set-directive-key').editable(editable_key);
	$('#directives-sortable a.set-directive-value').editable(editable_value);

	$('#directives-sortable').on('click','tr a.del-directive',function( e ) {
		pk = $(e.currentTarget).data('pk');
		$.ajax({
			type:'POST',url:'<?=$lr('site_del_directive')?>',dataType:'json',data:{D_id:pk,Site_id:'<?=\asm\Request::Bottom()?>'},
			success: function(data){ $('#'+pk).fadeOut(100,function(){ $('#'+pk).remove();})}})
		.fail(function(){ console.log('connection error');})
	});

	$('#directives-sortable').on('click','tr a.cp-directive',function( e ) {
		pk = $(e.currentTarget).data('pk');
		$.ajax({
			type:'POST',url:'<?=$lr('site_cp_directive')?>',dataType:'json',data:{D_id:pk,Site_id:'<?=\asm\Request::Bottom()?>'},
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

    $('#directives-sortable').sortable({forceHelperSize:true,opacity: .9,handle:'a.handle',placeholder:'ui-state-highlight',axis:'y',
        helper:function(e, ui) {
            ui.children().each(function() {
                $(this).width($(this).width());
            });
            return ui;
        },
        update:function(e,ui) {
            itemid = ui.item.attr('id');
            nextid = ui.item.next().attr('id');
            $.ajax({
                type:'POST',url:'<?=$lr('site_mv_directive')?>',dataType:'json',data:{D_id:itemid,NextD_id:nextid,Site_id:'<?=\asm\Request::Bottom()?>'}});
        }});

	$('#directives').on('submit','#site_set_directive_form',function( e ) {
		$form = $(e.target);
		e.preventDefault();
		$.ajax({
			type: "POST",url: $form.attr('action'),dataType: 'json',data: $form.serialize(),
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

