
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


@@@DirectiveForm
<?php $D = empty($D)?array('Name'=>'','Key'=>'','Value'=>''):$D; ?>
<div class="controls controls-row directive-form">
<?php /*
    <a href="#" data-type="select" data-url="<?=$lr('site_set_status')?>" data-value="<?=$S['Status']?>" data-name="Status"><?=$S['Status']?></a>
*/ ?>
<select class="span2" name="set-">
    <option></option>
   <?php foreach( array('page','html','lp') as $V ): ?>
    <option value="<?=$V?>">$<?=$V?></option>
   <?php endforeach; ?>
</select>
<input class="span2" type="text" placeholder="key" value="<?=$this($D['Key'])?>" >
<textarea class="span8" placeholder="value"><?=$this($D['Value'])?></textarea>
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
	$.ajaxSetup({headers: {'X-ASMBLR-USER': $.cookie('aaid'),'X-ASMBLR-PW': $.cookie('token')}});

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
	$('a.set-status').editable({source:[{value:'Active',text:'Active'},{value:'Disabled',text:'Disabled'}]});
	$('a.set-baseurl').editable({validate: function(v){return '';}});
	$('a.set-routine').editable({inputclass: 'input-large',validate: function(v){return '';}});

	$('div.directive-form').editable({selector: 'a'});
    $("#sortable-dirs").sortable({ placeholder: "ui-state-highlight" });
    $("#sortable-dirs").disableSelection();
});
</script>

