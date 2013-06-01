
<div class="row-fluid" id="directives">
    <div class="span12">
        <h3>Directives:</h3>
        <div id="dir-table-container"></div>

        <div id="diralert" class="alert alert-error"></div>
        <form id="set_directive_form" action="<?=$lr('page_set_directive')?>" class="form-inline">
        <select class="input-small" name="Name">
            <option></option>
           <?php foreach( $DirectiveNames as $V ): ?>
            <option value="<?=$V?>">$<?=$V?></option>
           <?php endforeach; ?>
        </select>
        <input class="input-small" type="text" placeholder="key" value="" name="Key" >
        <textarea class="" placeholder="value" name="Value" style="width: 75%;"></textarea>
        <button type="submit" class="btn btn-success">New</button>
        </form>
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



<?php $this->Stack('JSPage','ajax'); ?>
