$(function() {
	$( "#nav-panel-btn" ).on( "click", function(e) {
		$("#nav-panel").panel( "open" );
	});

	headers = {};
   <?php if( $page->LoggedIn ): ?>
    headers = {'X-ASMBLR-USER': '<?=$_SESSION['Account']['_id']?>',
 	             'X-ASMBLR-PW': '<?=$_SESSION['Account']['Password']?>'};
   <?php endif; ?>

   // this doesn't work when using the form plugin, thus it doesn't work
   // with jquery mobile - we set these as hidden fields in the forms themselves
    data = {};
   <?php if( $lp->Current('Site',TRUE) ): ?>
    data['Site_id'] = '<?=\fw\Request::Path(-1)?>';
   <?php elseif( $lp->Current('Page',TRUE) ): ?>
    data['Page_id'] = '<?=\fw\Request::Path(-1)?>';
    <?php elseif( $lp->Current('Template',TRUE) ): ?>
    data['Template_id'] = '<?=\fw\Request::Path(-1)?>';
   <?php endif;?>

    $.ajaxSetup({headers:headers,data:data,dataType:'json',type:'POST'});
});
