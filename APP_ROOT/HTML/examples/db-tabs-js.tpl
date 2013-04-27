<script>
/*
This template contains site-specific Javascript.  It is Stack()'d
in examples/Home.tpl and UnStack()'d in Base.tpl.

PHP is used to generate the correct URLs for Ajax calls.

This Javascript code uses the AjaxFrags and JSON routines
in Routines/Ajax.inc.
*/

//reload a tab
function reload_tab( id )
{
	$('#'+id).load('<?=$lp('AjaxFrags','?')?>'+id);
}

// reload the right-aside for examples (/ajf/examples-aside)
function reload_aside()
{
	$('#rightaside').load('<?=$lp('AjaxFrags','>examples-aside')?>');
}

// reload the form's three submit buttons (/ajf/submit-buttons)
function reload_buttons()
{
	$('#submit-buttons').load('<?=$lp('AjaxFrags','>submit-buttons')?>');
}

// post a form and process returned ValidationReport and reload fragments
function submit_form( e )
{
	$form = $(e.target);

	e.preventDefault();

    $.post($form.attr('action'),$form.serialize(),function(data){
    	$('.label-important',e.delegateTarget).html(data.Msg);

        if( data.Status == false )
        {
            tid = e.target.id;

            $.each( jQuery.parseJSON(data.Data), function(k,v) {
                if( v )
                    $('#'+tid+' input[name="'+k+'"]').removeClass('error');
                else
                	$('#'+tid+' input[name="'+k+'"]').addClass('error');
            });
        }
        else
        {
            reload_tab(e.delegateTarget.id);
            reload_aside();
            reload_buttons();
        }
    },'json');
}

$(document).ready(function()
{
    if( location.hash )
        window.scrollTo(0, 0);

    if( location.hash !== '' )
        $('a[href="' + location.hash + '"]').tab('show');

    $('a[data-toggle="tab"]').on('show', function(e) {
        if( location.hash )
            window.scrollTo(0, 0);

        location.hash = $(e.target).attr('href').substr(1);
    });

    $(document).ajaxStart(function() { $('#spinner').show(); });
    $(document).ajaxComplete(function() { window.setTimeout(function() { $('#spinner').hide(); },200)});


    // link/button click handlers  (AjaxFrags - /ajf/)
    $('#mongo-tab-content').on('click','a',function(e) {
        $('#mongo-tab-content').load($(e.currentTarget).attr('href'),function(){ reload_aside(); reload_buttons(); });
    	e.preventDefault();
    });
    $('#mysql-tab-content').on('click','a',function(e) {
        $('#mysql-tab-content').load($(e.currentTarget).attr('href'),function(){ reload_aside(); reload_buttons(); });
    	e.preventDefault();
    });
    $('#sqlsrv-tab-content').on('click','a',function(e) {
        $('#sqlsrv-tab-content').load($(e.currentTarget).attr('href'),function(){ reload_aside(); reload_buttons(); });
    	e.preventDefault();
    });
    $('aside').on('click','a.iconlink',function(e) {
        $('#rightaside').load($(e.currentTarget).attr('href'),reload_aside);
    	e.preventDefault();
    });

    // handle form submits  (JSON - /json/)
    $('#login').on('submit','#login',submit_form);
});

</script>

