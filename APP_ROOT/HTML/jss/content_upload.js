

$('a.set-prefix').editable({mode:'inline',placement:'right',send:'never',inputclass:'input-xlarge',emptytext:'optional prefix not set',
						   validate:null,success: function(r,nv){return true;}});


$('#content_upload').fileupload({
	 url: '<?=$lp('aj_content_upload')?>',autoUpload: false,formData:asmblr_ids,dropZone:$('.upload_dropzone')});
//	 .bind('fileuploadadd',function(e,data){console.log(data);});

