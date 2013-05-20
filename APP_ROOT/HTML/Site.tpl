
<ul class="breadcrumb">
<li><a href="<?=$lp('Home')?>">Home</a> <span class="divider">/</span></li>
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
        <ul id="menu1" class="dropdown-menu" role="menu" aria-labelledby="drop4">
           <?php foreach( $PS as $P ): ?>
            <li role="presentation"><a role="menuitem" href="<?=$lp('Page','>'.(string)$P['_id'])?>"><?=$this($P['Path'])?></a></li>
           <?php endforeach; ?>
        </ul>
    </li>
    <li class="active"><a href="<?=$lp('NewTemplate')?>">new template</a></li>
    <li class="dropdown">
        <a class="dropdown-toggle" role="button" data-toggle="dropdown" href="#">Templates <b class="caret"></b></a>
        <ul id="menu2" class="dropdown-menu" role="menu" aria-labelledby="drop5">
           <?php foreach( $TS as $S ): ?>
            <li role="presentation"><a role="menuitem" tabindex="-1" href="#"><?=$this($T['Name'])?></a></li>
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
        <a href="#" class="set-routine" data-type="textarea" data-url="<?=$lr('site_set_routine')?>" data-emptytext="empty routine" data-name="Routine"><?=\fw\Struct::Get(0,$S['Routine'])?></a>
    </div>
</div>

<div class="row-fluid" id="directives">
    <div class="span12">
        <h3>Directives:</h3>
        <form id="set_directive_form" action="<?=$lr('site_set_directive')?>">
        <?php $this->ajf_directive_table(); ?>
        </form>
    </div>
</div>


<?php $this->Stack('JSSite','ajax'); ?>

