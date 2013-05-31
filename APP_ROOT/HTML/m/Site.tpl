
<div data-role="header">
    <h1><a target="_blank" href="<?=asm('lp')->Link('Home')?>"><?=$S['Domain']?></a></h1>
    <a href="#" id="nav-panel-btn" data-icon="gear" class="ui-btn-left">Options</a>
    <div class="ui-btn-right">
        <label for="SiteStatus" class="ui-hidden-accessible">Status:</label>
        <select name="Status" id="SiteStatus" data-role="slider" data-mini="true">
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
        <label for="Domain" class="ui-hidden-accessible">Domain:</label>
        <input name="Domain" id="Domain" placeholder="domain" value="<?=$S['Domain']?>" type="text" >
        <a href="#" id="SaveDomain" data-role="button" data-icon="check" data-iconpos="notext" data-theme="c" data-inline="true">save</a>
        <a href="#" id="ResetDomain" data-role="button" data-icon="delete" data-iconpos="notext" data-theme="c" data-inline="true">cancel</a>
    </div>

    <div data-role="fieldcontain">
        <label for="BaseURL" class="ui-hidden-accessible">BaseURL:</label>
        <input name="BaseURL" id="BaseURL" placeholder="base URL" value="<?=$S['BaseURL']?>" type="url">
        <a href="#" id="SaveBaseURL" data-role="button" data-icon="check" data-iconpos="notext" data-theme="c" data-inline="true">save</a>
        <a href="#" id="ResetBaseURL" data-role="button" data-icon="delete" data-iconpos="notext" data-theme="c" data-inline="true">cancel</a>
    </div>

   <?php foreach( $DS as $D ): var_dump($D); endforeach; ?>
    <ul data-role="listview" data-divider-theme="" data-inset="true">
        <li data-role="list-divider" role="heading">Directives</li>
        <li data-theme="c"><a href="#" data-transition="slide">Button</a></li>
    </ul>

    <div data-role="fieldcontain">
        <label for="Routine" class="ui-hidden-accessible">Routine:</label>
        <?php $this->EditRoutine(array('Method'=>'site_set_routine','Routine'=>\fw\Struct::Get(0,$S['Routine']))); ?>
    </div>
</div>

