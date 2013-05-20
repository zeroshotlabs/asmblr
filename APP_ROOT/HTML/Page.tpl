
<ul class="breadcrumb">
<li><a href="<?=$lp('Home')?>">Home</a> <span class="divider">/</span></li>
<li><a href="<?=$lp('Site','>'.$S['_id'])?>"><?=$page->Domain?></a><?=$page->Path?></li>
<li>
&nbsp;&nbsp;<span style="font-size: .79em;"><a href="#" class="set-status" data-type="select" data-url="<?=$lr('page_set_status')?>" data-value="<?=$P['Status']?>" data-name="Status"><?=strtolower($P['Status'])?></a></span>
</li>
</ul>


<div class="clearfix"></div>

<div class="row-fluid">
    <div class="span12">
        <h3>Path:</h3>
        <a href="#" class="set-path" data-type="text" data-url="<?=$lr('page_set_path')?>" data-name="Path"><?=$P['Path']?></a>
    </div>
</div>

<div class="row-fluid">
    <div class="span12">
        <h3>Name:</h3>
        <a href="#" class="set-name" data-type="text" data-url="<?=$lr('page_set_name')?>" data-name="Name"><?=$P['Name']?></a>
    </div>
</div>

<div class="row-fluid">
    <div class="span12">
        <h3>Routine:</h3>
        <a href="#" class="set-routine" data-type="textarea" data-url="<?=$lr('page_set_routine')?>" data-emptytext="empty routine" data-name="Routine"><?=\fw\Struct::Get(0,$P['Routine'])?></a>
    </div>
</div>

<div class="row-fluid" id="directives">
    <div class="span12"><h3>Directives:</h3>
        <form id="site_set_directive_form" action="<?=$lr('site_set_directive')?>">
        <?php $this->ajf_directive_table(); ?>
<!--  site_set_directive -->

        </form>
    </div>
</div>


<?php $this->Stack('JSPage','ajax'); ?>
<?php $this->Stack('JSDirective','ajax'); ?>

