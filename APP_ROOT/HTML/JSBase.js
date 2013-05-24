
// because JS confuses itself
var NormParams = function( p )
{
	n = {};
	// from x-editable, simple key/value pairs
	if( typeof p.name !== 'undefined' )
	{
	    lastK = '';
    	$.each(p,function(k,v) {
    		if( k == 'name' )
    			lastK = v;
    		else if( k == 'value' )
    		    n[lastK] = v;
    		else
    			n[k] = v;
    	});
	}
	// from serializeArray forms (i.e. new directive form)
	else
	{
    	$.each(p,function(k,v) {
    	    n[v['name']] = v['value'];
    	});
	}

    return n;
}

var NormDirParams = function( p )
{
	p2 = {};
	if( p.name === 'Name' )
	{
	    p2['Name'] = p.value;
	    p2['Key'] = $('#'+p.pk+' a.set-directive-key').text();
	    p2['Value'] = $('#'+p.pk+' a.set-directive-value').text();
	}
	else if( p.name === 'Key' )
	{
	    p2['Name'] = $('#'+p.pk+' a.set-directive-name').text();
	    p2['Key'] = p.value;
	    p2['Value'] = $('#'+p.pk+' a.set-directive-value').text();
	}
	else if( p.name === 'Value' )
	{
	    p2['Name'] = $('#'+p.pk+' a.set-directive-name').text();
	    p2['Key'] = $('#'+p.pk+' a.set-directive-key').text();
	    p2['Value'] = p.value;
	}

    p2['D_id'] = p['pk'];

    return p2;
}

function ajfDirectives()
{
	$('#dir-table-container').load('<?=$lp('ajfHandler','>directive_table')?>',
	function(r,s,x)
	{
        $('#directives-sortable').sortable({forceHelperSize:true,opacity:.9,handle:'a.handle',placeholder:'ui-state-highlight',axis:'y',
            helper:function(e,ui) {
                ui.children().each(function() {
                    $(this).width($(this).width());
                });
                return ui;
            },
            update:function(e,ui) {
                itemid = ui.item.attr('id');
                nextid = ui.item.next().attr('id');
                $.ajax({ url:'<?=$lr('site_mv_directive')?>',
                        data:{D_id:itemid,NextD_id:nextid}}).done(function(){ajfDirectives();});
            }});
	});
}

// validate: function(v){return '';}
$(document).ready(function()
{
	if( location.hash )
        window.scrollTo(0, 0);

    $(document).ajaxStart(function() { $('#spinner').show(); });
    $(document).ajaxComplete(function() { window.setTimeout(function() { $('#spinner').hide(); },200)});

    headers = {};
   <?php if( $page->LoggedIn ): ?>
    headers = {'X-ASMBLR-USER': '<?=$_SESSION['Account']['_id']?>',
 	       'X-ASMBLR-PW': '<?=$_SESSION['Account']['Password']?>'};
   <?php endif; ?>

    data = {};
   <?php if( $lp->Current('Site',TRUE) ): ?>
    data['Site_id'] = '<?=\fw\Request::Path(-1)?>';
   <?php elseif( $lp->Current('Page',TRUE) ): ?>
    data['Page_id'] = '<?=\fw\Request::Path(-1)?>';
    <?php elseif( $lp->Current('Template',TRUE) ): ?>
    data['Template_id'] = '<?=\fw\Request::Path(-1)?>';
   <?php endif;?>

    $.ajaxSetup({headers:headers,data:data,dataType:'json',type:'POST'});

	// setup editable defaults
    // upon click to edit we could pull the latest value and compare timestamps/values and
    // notify user if it's changed; then push up the latest timestamp we have; of course this
    // ideally means we store a timestamp for every mongo field, not just the whole document
	$.fn.editable.defaults.mode = 'popup';
	$.fn.editable.defaults.onblur = 'ignore';
	$.fn.editable.defaults.send = 'always';
	$.fn.editable.defaults.placement = 'top';
	$.fn.editable.defaults.success = function(r,nv){ if( r.Status === false ) return r.Msg; };
	$.fn.editable.defaults.validate = function(v){if($.trim(v)=='') return 'Required.';};
	$.fn.editable.defaults.params = NormParams;

//    $('aside').on('click','a.iconlink',function(e) {
//        $('#rightaside').load($(e.currentTarget).attr('href'),reload_aside);
//    	e.preventDefault();
//    });

    // handle form submits  (JSON - /json/)
//    $('#login').on('submit','#login',submit_form);
});

