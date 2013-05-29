
<?php $this->Breadcrumb(); ?>

<div class="clearfix"></div>

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

<div class="row-fluid">
    <div class="span12">
        <h3>Domain:</h3>
        <a href="#" class="set-domain" data-type="text" data-url="<?=$lr('site_set_domain')?>" data-name="Domain"><?=$S['Domain']?></a>
    </div>
</div>

<div class="row-fluid">
    <div class="span12">
        <h3>Base URL:</h3>
        <a href="#" class="set-baseurl" data-type="url" data-url="<?=$lr('site_set_baseurl')?>" data-emptytext="default" data-name="BaseURL"><?=$S['BaseURL']?></a>
    </div>
</div>

<div class="row-fluid">
    <div class="span12">
        <h3>Routine:</h3>
        <a href="#" class="set-routine" data-type="textarea" data-url="<?=$lr('site_set_routine')?>" data-emptytext="no routine" data-name="Routine"><?=\fw\Struct::Get(0,$S['Routine'])?></a>
        <a title="edit" data-toggle="modal" data-target="#routine_modal" href="#">edit</a>
    </div>
</div>

<div class="row-fluid" id="directives">
    <div class="span12">
        <h3>Directives:</h3>
        <div id="dir-table-container"></div>

        <div id="diralert" class="alert alert-error"></div>
        <form id="set_directive_form" action="<?=$lr('site_set_directive')?>" class="form-inline">
        <select class="input-small" name="Name">
            <option></option>
           <?php foreach( $DirectiveNames as $V ): ?>
            <option value="<?=$V?>">$<?=$V?></option>
           <?php endforeach; ?>
        </select>
        <input class="input-small" type="text" placeholder="key" value="" name="Key" >
        <textarea class="" placeholder="value" name="Value" style="width: 75%;"></textarea>
        <button type="submit" class="btn btn-success">New</button>
        </form>
    </div>
</div>

<?php $this->EditRoutine(array('RoutineBody'=>\fw\Struct::Get(0,$S['Routine']))); ?>

<?php $this->Stack('JSSite','ajax'); ?>

