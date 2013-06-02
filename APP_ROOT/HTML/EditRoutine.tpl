
<textarea data-method="template_set_routine" data-role="none" id="Routine" name="Routine"><?=nl2br($this(\fw\Struct::Get(0,$T['Routine'])))?></textarea>
<textarea data-method="template_set_body" data-role="none" id="Body" name="Body"><?=nl2br($this($T['Body']))?></textarea>

<div style="margin-left: 22px;">
<a href="#" id="SaveRoutine" data-role="button" data-icon="check" data-iconpos="notext" data-theme="c" data-inline="true">save</a>
<a href="#" id="ResetRoutine" data-role="button" data-icon="delete" data-iconpos="notext" data-theme="c" data-inline="true">cancel</a>
</div>



