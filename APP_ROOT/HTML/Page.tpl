
<?php $this->Breadcrumb(); ?>

<div class="clearfix"></div>

<div class="row-fluid">
    <div class="span12">
        <h3>Path:</h3>
        <a href="#" class="set-path" data-type="text" data-url="<?=$lr('page_set_path')?>" data-name="Path"><?=$P['Path']?></a>
        <a target="_blank" href="<?=asm('lp')->Link($P['Name'])?>"><img src="<?=$ls('/img/ext-link.png')?>" /></a>
    </div>
</div>

<div class="row-fluid">
    <div class="span12">
        <h3>Name:</h3>
        <a href="#" class="set-name" data-type="text" data-url="<?=$lr('page_set_name')?>" data-emptytext="no name" data-name="Name"><?=$P['Name']?></a>
    </div>
</div>

<div class="row-fluid">
    <div class="span12">
        <h3>Routine:</h3>
        <a href="#" class="set-routine" data-type="textarea" data-url="<?=$lr('page_set_routine')?>" data-emptytext="no routine" data-name="Routine"><?=\fw\Struct::Get(0,$P['Routine'])?></a>
    </div>
</div>


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
