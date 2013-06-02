
function ajf_directive_grid( Type )
{
	$('#dir-container').load('<?=$lp('ajfHandler','>directive_grid')?>',function(){
		$('.selectpicker').selectpicker();

		$('#directives-sortable').sortable({axis:'y',handle:'div.handle',
	        update:function(e,ui) {
	            itemid = ui.item.attr('id');
	            nextid = ui.item.next().attr('id');
	            $.ajax({ url:aapi_method2url(Type+'_mv_directive'),
	                    data:{D_id:itemid,NextD_id:nextid}}).done(function(){ajf_directive_grid(Type);});
	    }});
	});
}


function aapi_method2url( Method )
{
	switch( Method )
	{
		case 'site_set_baseurl':
			url = '<?=$lr('site_set_baseurl')?>';
			break;
		case 'site_set_domain':
			url = '<?=$lr('site_set_domain')?>';
			break;
		case 'site_set_routine':
			url = '<?=$lr('site_set_routine')?>';
			break;
		case 'site_set_status':
			url = '<?=$lr('site_set_status')?>';
			break;
		case 'site_set_routine':
			url = '<?=$lr('site_set_routine')?>';
			break;

		case 'site_set_directive':
			url = '<?=$lr('site_set_directive')?>';
			break;
		case 'site_mv_directive':
			url = '<?=$lr('site_mv_directive')?>';
			break;
		case 'site_cp_directive':
			url = '<?=$lr('site_cp_directive')?>';
			break;
		case 'site_del_directive':
			url = '<?=$lr('site_del_directive')?>';
			break;
			
		case 'page_create':
			url = '<?=$lr('page_create')?>';
			break;
		case 'page_set_name':
			url = '<?=$lr('page_set_name')?>';
			break;
		case 'page_set_path':
			url = '<?=$lr('page_set_path')?>';
			break;
		case 'page_set_status':
			url = '<?=$lr('page_set_status')?>';
			break;
		case 'page_set_routine':
			url = '<?=$lr('page_set_routine')?>';
			break;

		case 'page_set_directive':
			url = '<?=$lr('page_set_directive')?>';
			break;
		case 'page_mv_directive':
			url = '<?=$lr('page_mv_directive')?>';
			break;
		case 'page_cp_directive':
			url = '<?=$lr('page_cp_directive')?>';
			break;
		case 'page_del_directive':
			url = '<?=$lr('page_del_directive')?>';
			break;

		case 'template_create':
			url = '<?=$lr('template_create')?>';
			break;
		case 'template_set_name':
			url = '<?=$lr('template_set_name')?>';
			break;
		case 'template_set_routine':
			url = '<?=$lr('template_set_routine')?>';
			break;
		case 'template_set_body':
			url = '<?=$lr('template_set_body')?>';
			break;

		default:
			url = 'unknown';
	}

	return url;
}

function aapi_set( Method,FormData )
{
	url = aapi_method2url(Method);

	$.ajax({url:url,data:FormData})
	 .done(function(data){
		 if( data.Status === true )
			 $('#aapi_msg').html('saved');
		 else
			 $('#aapi_msg').html(data.Msg);
    }).fail(function(data) { $('#aapi_msg').html('connection error'); });
}


function aapi_status( Class,Tag,Method )
{
    t = $('button.'+Class);
    if( t.data('status') === 'Active' )
    {
        t.addClass('btn-success');
        t.html(Tag+' is on');
        t.attr('title','click to turn off');
    }
    else
    {
        t.addClass('btn-danger');
        t.html(Tag+' is off');
        t.attr('title','click to turn on');
    }

    url = aapi_method2url(Method);
    
    $('button.'+Class).on('click',function(e){
        t = $(e.currentTarget);
        if( t.data('status') === 'Active' )
        {
        	$.ajax({url:url,data:{Status:'Disabled'}})
        	 .done(function(data){
           		 if( data.Status === true )
           		 {
                     t.removeClass('btn-success');
                     t.addClass('btn-danger');
                     t.html(Tag+' is off');
                     t.data('status','Disabled');
                     t.attr('title','click to turn on');
           		 }
           }).fail(function(data) { $('#aapi_msg').html('connection error'); });
        }
        else
        {
        	$.ajax({url:url,data:{Status:'Active'}})
       	     .done(function(data){
           		 if( data.Status === true )
           		 {
                     t.removeClass('btn-danger');
                     t.addClass('btn-success');
                     t.html(Tag+' is on');
                     t.data('status','Active');
                     t.attr('title','click to turn off');
           		 }
          }).fail(function(data) { $('#aapi_msg').html('connection error'); });
        }
	});
}


function cm_init( TextareaID )
{
	if( typeof cm_init.Editors == 'undefined' )
		cm_init.Editors = [];

	if( $('#'+TextareaID).data('hasShown') === 'true' )
		return;
	else
		$('#'+TextareaID).data('hasShown','true');

	cm_init.Editors[TextareaID] = CodeMirror.fromTextArea(document.getElementById(TextareaID),
	{
	    lineNumbers: true,
	    matchBrackets: true,
	    mode: $('#'+TextareaID).data('mode'),
	    indentUnit: 4,
	    extraKeys: {"Ctrl-S":function(instance){ $('#'+TextareaID+'_save').click(); }}
	});
	
	cm_init.Editors[TextareaID].on('change',function(e){
		$('#'+TextareaID+'_save').addClass('btn-warning');
		$('#'+TextareaID+'_reset').addClass('btn-primary');
	});
	
	$('#'+TextareaID+'_save').on('click',function(e){
		e.preventDefault();
		ta = cm_init.Editors[TextareaID].getTextArea();
		t = {};
		t[ta.name] = ta.defaultValue = cm_init.Editors[TextareaID].getValue();
		aapi_set($(ta).data('method'),t);
		$('#'+TextareaID+'_save').removeClass('btn-warning');
		$('#'+TextareaID+'_reset').removeClass('btn-primary');
	});
	
	$('#'+TextareaID+'_reset').on('click',function(e){
		e.preventDefault();
		cm_init.Editors[TextareaID].setValue(cm_init.Editors[TextareaID].getTextArea().defaultValue);
		$('#'+TextareaID+'_save').removeClass('btn-warning');
		$('#'+TextareaID+'_reset').removeClass('btn-primary');
	});
}


function xedit_norm( p )
{
	n = {};
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

	return n;
}


//page custom x-editable type
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


