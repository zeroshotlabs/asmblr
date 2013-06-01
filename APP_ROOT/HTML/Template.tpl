
<div class="row-fluid">
    <div class="span12">
        <h3>Routine:</h3>
        <a href="#" class="set-routine" data-type="textarea" data-url="<?=$lr('template_set_routine')?>" data-emptytext="no routine" data-name="Routine"><?=\fw\Struct::Get(0,$T['Routine'])?></a>
    </div>
</div>

<div class="row-fluid">
    <div class="span12">
        <h3>Body:</h3>
        <a href="#" class="set-body" data-type="textarea" data-url="<?=$lr('template_set_body')?>" data-emptytext="no body" data-name="Body"><?=nl2br($this($T['Body']))?></a>
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
