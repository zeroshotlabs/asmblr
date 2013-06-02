
/*
    	onCursorActivity: function() {
    	    editor.matchHighlight("CodeMirror-matchhighlight",2,'<?=\fw\Struct::Get('SearchTerms',$_GET)?>');
    	},
*/

	editor = CodeMirror.fromTextArea(document.getElementById("Routine"),
	{
	    lineNumbers: true,
	    matchBrackets: true,
	    mode: "text/x-php",
	    indentUnit: 4,
	    extraKeys: {"Ctrl-S":function(instance){
		    t = {};
		    t[instance.getTextArea().id] = instance.getValue();
		    aapi_set($(instance.getTextArea()).data('method'),t);
		    instance.getTextArea().defaultValue = instance.getValue();
			$('#Save'+instance.getTextArea().id).removeClass('btn-warning');
			$('#Reset'+instance.getTextArea().id).removeClass('btn-primary');

	    }}
	});
	
	
	init_cm(editor,'Routine',$(editor.getTextArea()).data('method'));


    
    
    