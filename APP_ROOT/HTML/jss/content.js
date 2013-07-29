
$('a.set-path').editable({mode:'popup',placement:'right',inputclass:'input-xlarge'});

<?php
$cts = array();
foreach( \fw\HTTP::GetContentTypes(TRUE) as $K => $V )
   $cts[] = array('text'=>$V,'value'=>$V);
?>
cts = <?=json_encode($cts)?>;
$('a.set-type').editable({mode:'popup',placement:'right',source:cts,value:''});

$('#confirm-del').on('click',function( e ) {
	pk = $(e.currentTarget).data('pk');
	$.ajax({ url:'<?=$la('content_delete',">{$C['_id']}")?>',data:{},
		success: function(data){ window.location = '<?=$lp('ContentUpload',">{$C['Site_id']}")?>'; }})
	.fail(function(){ console.log('connection error');})
});

$('a.body_tab_link','#content_tabs').click(function (e) {
	e.preventDefault();
	$(this).tab('show');
	// we might have an image
	if( $('#'+$(e.currentTarget).data('taid')).length > 0 )
		cm_init($(e.currentTarget).data('taid'));
});

$('#content_replace').fileupload({
	 url: '<?=$lp('aj_content_upload')?>',autoUpload: true,formData:asmblr_ids,dropZone:$('.replace_dropzone')
}).bind('fileuploaddrop',function(e,data){
	return confirm('Replace the content body and update type?');
}).bind('fileuploaddone',function(e,data){
	location.reload();
});


checkhash(window.location.hash);

$(window).on('hashchange',function(e){
	checkhash(window.location.hash);
});

function checkhash( hash )
{
	if( hash === '#body_tab' )
		$('a.body_tab_link','#content_tabs').click();
	else if( hash === '#upload_tab' )
		$('a.upload_tab_link','#content_tabs').click();
}

$('a.body_tab_link','#content_tabs').click();
