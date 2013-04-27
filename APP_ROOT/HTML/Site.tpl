
<ul class="breadcrumb">
<li><a href="<?=$lp('Home')?>">Home</a> <span class="divider">/</span></li>
<li><?=$page->Domain?></li>
</ul>

<ul class="nav nav-pills">
    <li class="dropdown">
        <a class="dropdown-toggle" id="drop4" role="button" data-toggle="dropdown" href="#">Pages <b class="caret"></b></a>
        <ul id="menu1" class="dropdown-menu" role="menu" aria-labelledby="drop4">
           <?php foreach( $PS as $P ): ?>
            <li role="presentation"><a role="menuitem" tabindex="-1" href="#"><?=$this($P['Path'])?></a></li>
           <?php endforeach; ?>
        </ul>
    </li>
    <li class="dropdown">
        <a class="dropdown-toggle" id="drop5" role="button" data-toggle="dropdown" href="#">Templates <b class="caret"></b></a>
        <ul id="menu2" class="dropdown-menu" role="menu" aria-labelledby="drop5">
           <?php foreach( $TS as $S ): ?>
            <li role="presentation"><a role="menuitem" tabindex="-1" href="#"><?=$this($T['Name'])?></a></li>
           <?php endforeach; ?>
        </ul>
    </li>
</ul>


<div class="row-fluid">
    <div class="span2">Domain:</div>
    <div class="span8">
        <a href="#" class="set-domain" data-type="text" data-url="<?=$lr('site_set_domain')?>" data-name="Domain"><?=$S['Domain']?></a>
    </div>
</div>

<div class="row-fluid">
    <div class="span2">Status:</div>
    <div class="span8">
        <a href="#" class="set-status" data-type="select" data-url="<?=$lr('site_set_status')?>" data-value="<?=$S['Status']?>" data-name="Status"><?=$S['Status']?></a>
    </div>
</div>

<div class="row-fluid">
    <div class="span2">Base URL:</div>
    <div class="span8">
        <a href="#" class="set-baseurl" data-type="url" data-url="<?=$lr('site_set_baseurl')?>" data-emptytext="default" data-name="BaseURL"><?=$S['BaseURL']?></a>
    </div>
</div>

<div class="row-fluid">
    <div class="span2">Routine:</div>
    <div class="span8">
        <a href="#" class="set-routine" data-type="textarea" data-url="<?=$lr('site_set_routine')?>" data-emptytext="empty routine" data-name="Routine"><?=\fw\Struct::Get(0,$S['Routine'])?></a>
    </div>
</div>

<div class="row-fluid">
    <div class="span2">Directives:</div>
    <div class="span8">
    <form class="form-inline">
       <div id="directives">
           <div id="sortable-dirs">
           <?php foreach( $S['Directives'] as $K => $D ): ?>
            <?php $this->DirectiveForm($D); ?>
           <?php endforeach; ?>
           <?php $this->DirectiveForm(); ?>
            </div>
       </div>

    </form>
    </div>
</div>


<?php $this->Stack('JSSite','ajax'); ?>

