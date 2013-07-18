
<ul class="nav nav-tabs" id="site_tabs">
    <li><a class="tab_link" href="#manage_tab" data-toggle="tab">Manage</a></li>
    <li><a class="tab_link" href="#directives_tab" data-toggle="tab">Directives</a></li>
    <li><a class="routine_tab_link" href="#routine_tab" data-toggle="tab" data-taid="site_routine">Routine</a></li>
    <li><a class="raw_tab_link" href="#raw_tab" data-toggle="tab" data-taid="site_raw">Raw</a></li>
</ul>

<div class="tab-content">
    <div class="tab-pane" id="manage_tab">
        <div class="row-fluid">
        <div class="span3">
            <a href="<?=$lp('aj_site_export',array('sid'=>(string)$S['_id']))?>" class="btn btn-block btn-success">Export Site</a>
        </div>
        </div>
        <div class="row-fluid">
        <div class="span3">
            <input type="text" name="SitePullURL" data-type="url" placeholder="http://site-to.import.com" />
        </div>
        </div>
    </div>
    <div class="tab-pane" id="directives_tab">
        <div id="dir-container"></div>
    </div>
    <div class="tab-pane" id="routine_tab">
        <textarea id="site_routine" name="Routine" data-method="site_set_routine" data-mode="text/x-php" data-hasShown=""><?=$this(\fw\Struct::Get(0,$S['Routine']))?></textarea>
        <div>
            <a href="#" id="site_routine_save" class="btn">save</a>
            <a href="#" id="site_routine_reset" class="btn">cancel</a>
        </div>
    </div>
    <div class="tab-pane" id="raw_tab">
        <textarea id="site_raw" name="Raw" data-method="site_set_raw" data-mode="text/x-php"><?=\asm\Site::ToPHP($S)?></textarea>
    </div>
</div>

<div id="site_delete" class="modal hide">
    <div class="modal-header">
        <h3>Delete ENTIRE site?</h3>
    </div>
    <div class="modal-body">
        <p>This wipes out all Pages, Templates and Content!</p>
        <p>It cannot be undone.</p>
        <button class="btn" data-dismiss="modal" aria-hidden="true">Cancel</button>
        <button id="confirm-del" class="btn btn-danger" data-pk="<?=$S['_id']?>">Delete</button>
    </div>
</div>

