
<ul class="nav nav-tabs" id="site_tabs">
    <li><a class="tab_link" href="#directives_tab" data-toggle="tab">Directives</a></li>
    <li><a class="cm_tab_link" href="#routine_tab" data-toggle="tab" data-taid="site_routine">Routine</a></li>
</ul>

<div class="tab-content">
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
</div>

