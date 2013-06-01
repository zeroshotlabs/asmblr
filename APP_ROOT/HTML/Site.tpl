
<div class="row-fluid" id="directives">
    <div class="span12">
        <h3>Directives:</h3>
        <div id="dir-table-container"></div>

        <div id="diralert" class="alert alert-error"></div>
        <form id="set_directive_form" action="<?=$lr('site_set_directive')?>" class="form-inline">
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

<?php /*         <a title="edit" data-toggle="modal" data-target="#routine_modal" href="#">edit</a> */ ?>

<?php $this->EditRoutine(array('Method'=>'site_set_routine','Routine'=>\fw\Struct::Get(0,$S['Routine']))); ?>

<?php $this->Stack('JSSite','ajax'); ?>

