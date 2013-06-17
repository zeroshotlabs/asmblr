
<ul class="nav nav-tabs" id="template_tabs">
    <li><a class="tab_link" href="#routine_tab" data-toggle="tab" data-taid="template_routine">Routine</a></li>
    <li><a class="body_tab_link" href="#body_tab" data-toggle="tab" data-taid="template_body">Body</a></li>
</ul>

<div class="tab-content">
    <div class="tab-pane" id="routine_tab">
        <textarea id="template_routine" name="Routine" data-method="template_set_routine" data-mode="text/x-php" data-hasShown=""><?=$this(\fw\Struct::Get(0,$T['Routine']))?></textarea>
        <div>
            <a href="#" id="template_routine_save" class="btn">save</a>
            <a href="#" id="template_routine_reset" class="btn">cancel</a>
        </div>
    </div>
    <div class="tab-pane" id="body_tab">
        <textarea id="template_body" name="Body" data-method="template_set_body" data-mode="application/x-httpd-php" data-hasShown=""><?=$this($T['Body'])?></textarea>
        <div>
            <a href="#" id="template_body_save" class="btn">save</a>
            <a href="#" id="template_body_reset" class="btn">cancel</a>
        </div>
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


