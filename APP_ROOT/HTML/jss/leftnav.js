
<?php if( $page->ActiveNav === 'Site' ): ?>
aapi_status('site-status','site','site_set_status');
<?php elseif( $page->ActiveNav === 'Page' ): ?>
aapi_status('page-status','page','page_set_status');
<?php elseif( $page->ActiveNav === 'Content' ): ?>
aapi_status('content-status','content','content_set_status');
<?php endif; ?>


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

$('button.new-content').editable({display:false,showbuttons:'bottom',placement:'bottom',inputclass:'input-large',
    success: function(r,nv){
        if( r.Status === false )
            return r.Msg;
        else
            window.location = '<?=$lp('Content')?>'+r.Data._id;
    }
});
