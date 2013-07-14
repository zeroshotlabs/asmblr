
$('#createsite').on('submit','#createsite-form',function( e ) {
	$form = $(e.target);
	e.preventDefault();
	$.ajax({
		type: "POST",url: $form.attr('action'),dataType: 'json',data: $form.serialize(),
		success: function(data){
	    if( data.Status === true )
		    window.location.href = '<?=$lp('Site')?>'+data.Data._id;
	    else
	    	$('.label-important',e.delegateTarget).html(data.Msg);
		}})
	.fail(function(){ $('.label-important',e.delegateTarget).html('Please complete the form.'); });
});


$('#site_import').fileupload({
	 url: '<?=$lr('site_import')?>',autoUpload: true,dropZone:$('.import_dropzone')
}).bind('fileuploaddone',function(e,data){
	if( data.result.Status === false )
		$('.label-important',e.delegateTarget).html(data.result.Msg);
	else
		window.location.href = '<?=$lp('Site')?>'+data.result.Data._id;

	window.setTimeout(function() { $('#spinner').hide(); },200);
}).bind('fileuploadfail',function(e,data){
	$('.label-important',e.delegateTarget).html('Please ensure a domain is specified.');
	window.setTimeout(function() { $('#spinner').hide(); },200);
});

