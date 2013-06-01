
<div class="pull-right">
<ul class="nav nav-pills">
    <li class="active">
        <a class="new-page" data-type="page" data-showbuttons="bottom" data-url="<?=$lr('page_create')?>">new page</a>
    </li>
    <li class="dropdown">
        <a class="dropdown-toggle" role="button" data-toggle="dropdown" href="#">Pages <b class="caret"></b></a>
        <ul id="menu1" class="dropdown-menu" role="menu" aria-labelledby="pages">
           <?php foreach( $PL as $P ): ?>
            <li role="presentation">
                <a role="menuitem" href="<?=$lp('Page','>'.(string)$P['_id'])?>">
                    <?=$this($P['Name'])?><small> - <?=$this($P['Path'])?></small>
                </a>
            </li>
           <?php endforeach; ?>
        </ul>
    </li>
    <li class="active">
        <a class="new-template" data-type="text" data-showbuttons="bottom" data-url="<?=$lr('template_create')?>" data-placeholder="TemplateName" data-name="Name" href="#">new template</a>
    </li>
    <li class="dropdown">
        <a class="dropdown-toggle" role="button" data-toggle="dropdown" href="#">Templates <b class="caret"></b></a>
        <ul id="menu2" class="dropdown-menu" role="menu" aria-labelledby="templates">
           <?php foreach( $TL as $T ): ?>
            <li role="presentation"><a role="menuitem" href="<?=$lp('Template','>'.(string)$T['_id'])?>"><?=$this($T['Name'])?></a></li>
           <?php endforeach; ?>
        </ul>
    </li>
</ul>
</div>

<div class="clearfix"></div>
