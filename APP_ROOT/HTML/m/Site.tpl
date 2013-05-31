
    <div data-theme="c" data-role="header">
    stackware.local
    <div style="float: left;"><a href="#" id="nav-panel-btn" class="ui-btn ui-shadow ui-btn-corner-all ui-btn-icon-notext ui-btn-up-a"
       data-icon="bars" data-iconpos="notext" data-corners="true" data-shadow="true" data-iconshadow="true"
       data-wrapperels="span" data-theme="a" title="Navigation">
        <span class="ui-btn-inner">
        <span class="ui-btn-text">Navigation</span>
        <span class="ui-icon ui-icon-bars ui-icon-shadow">&nbsp;</span>
        </span>
    </a></div>
    </div>

    <div data-role="content">
        <div data-role="navbar" data-iconpos="left">
            <ul>
                <li><a href="#page" data-transition="fade" data-theme="" data-icon="plus">page</a></li>
                <li><a href="#template" data-transition="fade" data-theme="" data-icon="plus">template</a></li>
            </ul>
        </div>

        <div id="aapi_msg"></div>

        <div data-role="fieldcontain">
            <label for="Domain" class="ui-hidden-accessible">Domain:</label>
            <input name="Domain" id="Domain" placeholder="domain" value="<?=$S['Domain']?>" type="text" >
            <a href="#"id="SaveDomain" data-role="button" data-icon="check" data-iconpos="notext" data-theme="c" data-inline="true">save</a>
            <a href="#" id="ResetDomain" data-role="button" data-icon="delete" data-iconpos="notext" data-theme="c" data-inline="true">cancel</a>
        </div>

        <div data-role="fieldcontain">
            <label for="BaseURL" class="ui-hidden-accessible">BaseURL:</label>
            <input name="BaseURL" id="BaseURL" placeholder="base URL" value="<?=$S['BaseURL']?>" type="url">
            <a href="#"id="SaveBaseURL" data-role="button" data-icon="check" data-iconpos="notext" data-theme="c" data-inline="true">save</a>
            <a href="#" id="ResetBaseURL" data-role="button" data-icon="delete" data-iconpos="notext" data-theme="c" data-inline="true">cancel</a>
        </div>

        <ul data-role="listview" data-divider-theme="" data-inset="true">
            <li data-role="list-divider" role="heading">Directives</li>
            <li data-theme="c"><a href="#" data-transition="slide">Button</a></li>
        </ul>

        <div data-role="fieldcontain">
            <label for="Routine" class="ui-hidden-accessible">Routine:</label>
            <?php $this->EditRoutine(array('Method'=>'site_set_routine','Routine'=>\fw\Struct::Get(0,$S['Routine']))); ?>
        </div>
    </div>

