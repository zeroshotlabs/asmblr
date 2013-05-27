
// because JS confuses itself
var NormParams = function( p )
{
	n = {};
	// from x-editable
	if( typeof p.name !== 'undefined'  )
	{
		// simple key/value pairs
		if( p.name !== '' )
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
		// custom x-editable types (Page)
		else
		{
			n = p.value;
		}
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


(function ($) {
    "use strict";
    
    var Page = function (options) {
        this.init('page', options, Page.defaults);
    };

    //inherit from Abstract input
    $.fn.editableutils.inherit(Page, $.fn.editabletypes.abstractinput);

    $.extend(Page.prototype, {
        /**
        Renders input from tpl

        @method render() 
        **/
        render: function() {
           this.$input = this.$tpl.find('input');
        },
        
        /**
        Default method to show value in element. Can be overwritten by display option.
        
        @method value2html(value, element) 
        **/
        value2html: function(value, element) {
            if(!value) {
                $(element).empty();
                return; 
            }
            var html = $('<div>').text(value.Name).html() + '| ' + $('<div>').text(value.Path).html();
            $(element).html(html); 
        },
        
        /**
        Gets value from element's html
        
        @method html2value(html) 
        **/        
        html2value: function(html) {        
          /*
            you may write parsing method to get value by element's html
            e.g. "Moscow, st. Lenina, bld. 15" => {city: "Moscow", street: "Lenina", building: "15"}
            but for complex structures it's not recommended.
            Better set value directly via javascript, e.g. 
            editable({
                value: {
                    city: "Moscow", 
                    street: "Lenina", 
                    building: "15"
                }
            });
          */ 
          return null;  
        },
      
       /**
        Converts value to string. 
        It is used in internal comparing (not for sending to server).
        
        @method value2str(value)  
       **/
       value2str: function(value) {
           var str = '';
           if(value) {
               for(var k in value) {
                   str = str + k + ':' + value[k] + ';';  
               }
           }
           return str;
       }, 
       
       /*
        Converts string to value. Used for reading value from 'data-value' attribute.
        
        @method str2value(str)  
       */
       str2value: function(str) {
           /*
           this is mainly for parsing value defined in data-value attribute. 
           If you will always set value by javascript, no need to overwrite it
           */
           return str;
       },                
       
       /**
        Sets value of input.
        
        @method value2input(value) 
        @param {mixed} value
       **/         
       value2input: function(value) {
           if(!value) {
             return;
           }
           this.$input.filter('[name="Name"]').val(value.Name);
           this.$input.filter('[name="Path"]').val(value.Path);
       },       
       
       /**
        Returns value of input.
        
        @method input2value() 
       **/          
       input2value: function() {
           return {
              Name: this.$input.filter('[name="Name"]').val(), 
              Path: this.$input.filter('[name="Path"]').val()
           };
       },

        /**
        Activates input: sets focus on the first field.
        
        @method activate() 
       **/        
       activate: function() {
            this.$input.filter('[name="Name"]').focus();
       },  
       
       /**
        Attaches handler to submit form in case of 'showbuttons=false' mode
        
        @method autosubmit() 
       **/
       autosubmit: function() {
           this.$input.keydown(function (e) {
                if (e.which === 13) {
                    $(this).closest('form').submit();
                }
           });
       }
    });

    Page.defaults = $.extend({}, $.fn.editabletypes.abstractinput.defaults, {
        tpl: '<div class="editable-page"><input placeholder="Name" type="text" name="Name" class="input"></div>'+
             '<div class="editable-page"><input placeholder="/path" type="text" name="Path" class="input"></div>',
        inputclass: ''
    });

    $.fn.editabletypes.page = Page;

}(window.jQuery));
