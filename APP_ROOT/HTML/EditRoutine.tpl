
<textarea data-method="<?=$Method?>" data-role="none" id="Routine" name="Routine"><?=$this($Routine)?></textarea>

<div style="margin-left: 22px;">
<a href="#" id="SaveRoutine" data-role="button" data-icon="check" data-iconpos="notext" data-theme="c" data-inline="true">save</a>
<a href="#" id="ResetRoutine" data-role="button" data-icon="delete" data-iconpos="notext" data-theme="c" data-inline="true">cancel</a>
</div>

<?php $this->Stack('JSEditRoutine','ajax'); ?>

