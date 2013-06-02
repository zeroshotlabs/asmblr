
<div class="row-fluid" id="directives">
    <div class="span12">
        <div id="dir-container"></div>
    </div>
</div>


<?php /*         <a title="edit" data-toggle="modal" data-target="#routine_modal" href="#">edit</a> */ ?>

<?php // $this->EditRoutine(array('Method'=>'site_set_routine','Routine'=>\fw\Struct::Get(0,$S['Routine']))); ?>




<div id="page_delete" class="modal hide">
    <div class="modal-header">
        <h3>Delete page?</h3>
    </div>
    <div class="modal-body">
        <p>This cannot be undone.</p>
        <button class="btn" data-dismiss="modal" aria-hidden="true">Cancel</button>
        <button id="confirm-del" class="btn btn-danger" data-pk="<?=$P['_id']?>">Delete</button>
    </div>
</div>


