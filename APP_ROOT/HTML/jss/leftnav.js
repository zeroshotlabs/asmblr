
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

