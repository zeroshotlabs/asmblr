
<div class="row-fluid" id="directives">
    <div class="span12">
        <div id="routine-container"></div>
    </div>
</div>

<div class="row-fluid" id="directives">
    <div class="span12">
        <div id="body-container"></div>
    </div>
</div>


<ul class="nav nav-tabs" id="template_edit">
    <li class="active"><a href="#routine">Routine</a></li>
    <li><a href="#body">Body</a></li>
</ul>

<div class="tab-content">
  <div class="tab-pane active" id="routine">
     <?php $this->EditRoutine(); ?>
  </div>
  <div class="tab-pane" id="body">
     <?php $this->EditRoutine(); ?>
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


