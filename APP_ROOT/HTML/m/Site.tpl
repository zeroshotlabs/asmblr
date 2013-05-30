
    <div data-theme="c" data-role="header">
    stackware.local
    <a href="#" id="nav-panel-btn" class="ui-btn-left ui-btn ui-shadow ui-btn-corner-all ui-btn-icon-notext ui-btn-up-a"
       data-icon="bars" data-iconpos="notext" data-corners="true" data-shadow="true" data-iconshadow="true"
       data-wrapperels="span" data-theme="a" title="Navigation">
        <span class="ui-btn-inner">
        <span class="ui-btn-text">Navigation</span>
        <span class="ui-icon ui-icon-bars ui-icon-shadow">&nbsp;</span>
        </span>
    </a>
    </div>

    <div data-role="content">
        <div data-role="navbar" data-iconpos="left">
            <ul>
                <li><a href="#page" data-transition="fade" data-theme="" data-icon="plus">page</a></li>
                <li><a href="#template" data-transition="fade" data-theme="" data-icon="plus">template</a></li>
            </ul>
        </div>

        <form method="post" action="<?=$lr('site_set_domain')?>">
        <input type="hidden" name="Site_id" value="<?=$page->Site_id?>" />
        <div data-role="fieldcontain">
            <label for="Domain" class="ui-hidden-accessible">Domain:</label>
            <input type="text" name="Domain" id="Domain" placeholder="domain" value="<?=$S['Domain']?>">
        </div>
        <input type="submit" name="submit" value="save" data-inline="true" />
        <input type="reset" name="reset" value="reset" data-inline="true" />

<?php /*
class="btn-success ui-btn-hidden" aria-disabled="false"
data-role="button" data-icon="check" data-iconpos="notext" data-theme="c" data-inline="true"

            <a href="#" class="btn-danger" data-role="button" data-icon="delete" data-iconpos="notext" data-theme="c" data-inline="true">cancel</a>
        </div>
*/ ?>

            </form>


        <div data-role="fieldcontain">
            <input name="BaseURL" id="textinput2" placeholder="base URL" value="" type="url">
            <a href="#" class="btn-success" data-role="button" data-icon="check" data-iconpos="notext" data-theme="c" data-inline="true">save</a>
            <a href="#" class="btn-danger" data-role="button" data-icon="delete" data-iconpos="notext" data-theme="c" data-inline="true">cancel</a>
        </div>

        <ul data-role="listview" data-divider-theme="" data-inset="true">
            <li data-role="list-divider" role="heading">Directives</li>
            <li data-theme="c"><a href="#" data-transition="slide">Button</a></li>
        </ul>

        <div data-role="fieldcontain">
            <?php $this->EditRoutine(array('RoutineBody'=>\fw\Struct::Get(0,$S['Routine']))); ?>
        </div>
    </div>
