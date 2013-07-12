
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

$(".btn-submit").prop("disabled",false);
