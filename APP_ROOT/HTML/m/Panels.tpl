
<div data-role="panel" id="nav-panel" data-position="left" data-display="reveal">
    <ul data-role="listview" data-icon="false" data-global-nav="demos">
        <li data-role="list-divider"><a rel="external" href="<?=$lp('Home')?>">All Sites</a></li>
        <li data-role="list-divider">Pages</li>
       <?php foreach( $PL as $P ): ?>
        <li>
            <a href="<?=$lp('Page','>'.(string)$P['_id'])?>"><?=$this($P['Name'])?><small> - <?=$this($P['Path'])?></small></a>
        </li>
       <?php endforeach; ?>

        <li data-role="list-divider">Templates</li>
       <?php foreach( $TL as $T ): ?>
        <li>
            <a href="<?=$lp('Template','>'.(string)$T['_id'])?>"><?=$this($T['Name'])?></a>
        </li>
       <?php endforeach; ?>

    </ul>
</div>


<div id="create_page_panel" data-role="panel" data-position="right" data-display="overlay">
<form id="create_page_form" action="<?=$lr('page_create')?>" method="post">
    <div data-role="fieldcontain">
        <label for="NewPageName" class="ui-hidden-accessible">Name:</label>
        <input name="Name" id="NewPageName" placeholder="Name" value="" type="text" >
    </div>
    <div data-role="fieldcontain">
        <label for="NewPagePath" class="ui-hidden-accessible">Path:</label>
        <input name="Path" id="NewPagePath" placeholder="/page/path" value="" type="text">
    </div>
    <div>
        <a href="#" id="CreatePage" data-role="button" data-icon="check" data-iconpos="notext" data-theme="c" data-inline="true">create</a>
        <a href="#" id="ResetCreatePage" data-role="button" data-icon="delete" data-iconpos="notext" data-theme="c" data-inline="true">cancel</a>
    </div>
</form>
</div>

