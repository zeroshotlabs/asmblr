

$('a.set-prefix').editable({mode:'inline',placement:'right',send:'never',inputclass:'input-xlarge',emptytext:'no prefix set',
						   validate:null,success: function(r,nv){return true;}});
/*
var gutter = 12;
var min_width = 150;
var $container = $("#content_grid");

$("#content_grid").masonry({
    itemSelector : '.box',
    gutterWidth: gutter,
    isAnimated: true,
      columnWidth: function( containerWidth ) {
        var num_of_boxes = (containerWidth/min_width | 0);

        var box_width = (((containerWidth - (num_of_boxes-1)*gutter)/num_of_boxes) | 0) ;

        if (containerWidth < min_width) {
            box_width = containerWidth;
        }

        $('.box').width(box_width);

        return box_width;
      }
});
*/

$('#content_upload').fileupload({
	 url: '<?=$lp('aj_content_upload')?>',autoUpload: true,dropZone:$('.upload_dropzone'),
	 formData:function(f){
		 aid = [{name:'Site_id',value:asmblr_ids['Site_id']},
		        {name:'UploadPrefix',value:$('.set-prefix').editable('getValue').UploadPrefix}
		 	   ];
//		 console.log($('.set-prefix').editable('getValue'));
//		 aid['UploadPrefix'] = $('.set-prefix').editable('getValue').UploadPrefix;
//		 console.log(aid);
		 return aid;
	 }
}).bind('fileuploaddone',function(e,data){
	var ab = $('.alert-body');
	var ah = $('.alert-header');
	var t = $('.alert');
	
	if( data.result.Status === false )
	{
		ah.html('Aborted!');
		ab.html(data.result.Msg);
        t.removeClass('alert-success');
        t.addClass('alert-error');
	}
	else
	{
		ah.html('Success!');
		ab.html(data.result.Data);
        t.removeClass('alert-error');
        t.addClass('alert-success');
		ajf_content_grid();
	}
	
	$('.alert').show();

	window.setTimeout(function() { $('#spinner').hide(); },200);
});

