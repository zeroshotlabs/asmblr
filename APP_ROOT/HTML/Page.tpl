
<ul class="nav nav-tabs" id="page_tabs">
    <li><a class="tab_link" href="#directives_tab" data-toggle="tab">Directives</a></li>
    <li><a class="routine_tab_link" href="#routine_tab" data-toggle="tab" data-taid="page_routine">Routine</a></li>
    <li><a class="raw_tab_link" href="#raw_tab" data-toggle="tab" data-taid="page_raw">Raw</a></li>
</ul>

<div class="tab-content">
    <div class="tab-pane" id="directives_tab">
        <div id="dir-container"></div>
    </div>
    <div class="tab-pane" id="routine_tab">
        <textarea id="page_routine" name="Routine" data-method="page_set_routine" data-mode="text/x-php"><?=$this(\fw\Struct::Get(0,$P['Routine']))?></textarea>
        <div>
            <a href="#" id="page_routine_save" class="btn">save</a>
            <a href="#" id="page_routine_reset" class="btn">cancel</a>
        </div>
    </div>
    <div class="tab-pane" id="raw_tab">
        <textarea id="page_raw" name="Raw" data-method="page_set_raw" data-mode="text/x-php"><?=\asm\Page::ToPHP($P)?></textarea>
    </div>
</div>

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

