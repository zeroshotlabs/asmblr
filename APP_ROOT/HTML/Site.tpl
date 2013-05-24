
<ul class="breadcrumb">
<li><a href="<?=$lp('Home')?>">Home</a> <span class="divider">|</span></li>
<li><?=$page->Domain?></li>
<li>
&nbsp;&nbsp;<span style="font-size: .79em;"><a href="#" class="set-status" data-type="select" data-url="<?=$lr('site_set_status')?>" data-value="<?=$S['Status']?>" data-name="Status"><?=strtolower($S['Status'])?></a></span>
</li>
</ul>


<div class="pull-right">
<ul class="nav nav-pills">
    <li class="active">
        <a class="new-page" data-type="text" data-url="<?=$lr('page_create')?>" data-placeholder="/url-path" data-name="Path" href="#">new page</a>
    </li>
    <li class="dropdown">
        <a class="dropdown-toggle" role="button" data-toggle="dropdown" href="#">Pages <b class="caret"></b></a>
        <ul id="menu1" class="dropdown-menu" role="menu" aria-labelledby="pages">
           <?php foreach( $PS as $P ): ?>
            <li role="presentation"><a role="menuitem" href="<?=$lp('Page','>'.(string)$P['_id'])?>"><?=$this($P['Path'])?></a></li>
           <?php endforeach; ?>
        </ul>
    </li>
    <li class="active">
        <a class="new-template" data-type="text" data-url="<?=$lr('template_create')?>" data-placeholder="TemplateName" data-name="Name" href="#">new template</a>
    </li>
    <li class="dropdown">
        <a class="dropdown-toggle" role="button" data-toggle="dropdown" href="#">Templates <b class="caret"></b></a>
        <ul id="menu2" class="dropdown-menu" role="menu" aria-labelledby="templates">
           <?php foreach( $TS as $T ): ?>
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


<?php $this->Stack('JSSite','ajax'); ?>

