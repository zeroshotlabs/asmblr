
<div data-role="header">
    <h1><a target="_blank" href="<?=asm('lp')->Link($P['Name'])?>"><?=$S['Domain'].$P['Path']?></a></h1>
    <a href="#" id="nav-panel-btn" data-icon="gear" class="ui-btn-left">Options</a>
    <div class="ui-btn-right">
        <label for="PageStatus" class="ui-hidden-accessible">Status:</label>
        <select name="Status" id="PageStatus" data-role="slider" data-mini="true">
            <option <?=$this('Status',$S,'Disabled','selected')?> value="Disabled">Off</option>
            <option <?=$this('Status',$S,'Active','selected')?> value="Active">On</option>
        </select>
    </div>
    <div data-role="navbar" data-iconpos="left">
        <ul>
            <li><a href="#create_page_panel" data-transition="fade" data-theme="" data-icon="plus">page</a></li>
            <li><a href="#create_template" data-transition="fade" data-theme="" data-icon="plus">template</a></li>
        </ul>
    </div>
</div>

<div data-role="content">
    <div id="aapi_msg"></div>

    <div data-role="fieldcontain">
        <label for="PageName" class="ui-hidden-accessible">Name:</label>
        <input name="Name" id="PageName" placeholder="name" value="<?=$P['Name']?>" type="text" >
        <a href="#" id="SavePageName" data-role="button" data-icon="check" data-iconpos="notext" data-theme="c" data-inline="true">save</a>
        <a href="#" id="ResetPageName" data-role="button" data-icon="delete" data-iconpos="notext" data-theme="c" data-inline="true">cancel</a>
    </div>

    <div data-role="fieldcontain">
        <label for="PagePath" class="ui-hidden-accessible">Path:</label>
        <input name="Path" id="PagePath" placeholder="/path" value="<?=$P['Path']?>" type="text">
        <a href="#" id="SavePagePath" data-role="button" data-icon="check" data-iconpos="notext" data-theme="c" data-inline="true">save</a>
        <a href="#" id="ResetPagePath" data-role="button" data-icon="delete" data-iconpos="notext" data-theme="c" data-inline="true">cancel</a>
    </div>

    <ul data-role="listview" data-divider-theme="" data-inset="true">
        <li data-role="list-divider" role="heading">Directives</li>
        <li data-theme="c"><a href="#" data-transition="slide">Button</a></li>
    </ul>

    <div data-role="fieldcontain">
        <label for="Routine" class="ui-hidden-accessible">Routine:</label>
        <?php $this->EditRoutine(array('Method'=>'page_set_routine','Routine'=>\fw\Struct::Get(0,$P['Routine']))); ?>
    </div>


<div id="page_delete" class="modal hide">
    <div class="modal-header">
        <h3>Delete page?</h3>
    </div>
    <div class="modal-body">
        <p>This cannot be undone.</p>
        <button class="btn" data-dismiss="modal" aria-hidden="true">Cancel</button>
        <button id="confirm-del" class="btn btn-danger" data-pk="<?=$P['_id']?>">Delete</button>
    </div>
</div>

</div>

<script>

$(document).on('pageinit','#jqm_page', function() {

	$('#nav-panel-btn').on( "click",function(e) {
		$("#nav-panel").panel( "open" );
	});

	init_field('PageName','page_set_name');
	init_field('PagePath','page_set_path');

	$("#PageStatus").bind("change",function(e) {
		aapi_set('page_set_status',{Status:$(e.currentTarget).val()});
	});
});

</script>