
<ul class="breadcrumb">
<li><a href="<?=$lp('Home')?>">Home</a> <span class="divider">|</span></li>
<li><a href="<?=$lp('Site','>'.$S['_id'])?>"><?=$page->Domain?></a><span class="divider">|</span><?=$page->Name?></li>

<li class="pull-right">
    <a title="delete" data-toggle="modal" data-target="#template_delete" href="#">
    <img alt="delete" src="<?=$ls('/img/glyphicons_256_delete.png')?>">
    </a>
</li>

</ul>


<div class="clearfix"></div>


<div class="row-fluid">
    <div class="span12">
        <h3>Name:</h3>
        <a href="#" class="set-name" data-type="text" data-url="<?=$lr('template_set_name')?>" data-emptytext="no name" data-name="Name"><?=$T['Name']?></a>
    </div>
</div>

<div class="row-fluid">
    <div class="span12">
        <h3>Routine:</h3>
        <a href="#" class="set-routine" data-type="textarea" data-url="<?=$lr('template_set_routine')?>" data-emptytext="no routine" data-name="Routine"><?=\fw\Struct::Get(0,$T['Routine'])?></a>
    </div>
</div>

<div class="row-fluid">
    <div class="span12">
        <h3>Body:</h3>
        <a href="#" class="set-body" data-type="textarea" data-url="<?=$lr('template_set_body')?>" data-emptytext="no body" data-name="Body"><?=$T['Body']?></a>
    </div>
</div>


<div id="template_delete" class="modal hide">
    <div class="modal-header">
        <h3>Delete template?</h3>
    </div>
    <div class="modal-body">
        <p>This cannot be undone.</p>
        <button class="btn" data-dismiss="modal" aria-hidden="true">Cancel</button>
        <button id="confirm-del" class="btn btn-danger" data-pk="<?=$T['_id']?>">Delete</button>
    </div>
</div>


<?php $this->Stack('JSTemplate','ajax'); ?>
