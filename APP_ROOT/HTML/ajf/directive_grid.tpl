
<div class="sortable" id="directives-sortable">
<?php foreach( $DS as $K => $D ): ?>
<div id="<?=$K?>">
    <div class="input-prepend">
        <select name="Name" class="selectpicker dir-part" data-width="auto" data-pk="<?=$K?>" >
           <?php foreach( $DirectiveNames as $V ): ?>
            <option value="<?=$V?>" <?=$this('Name',$D,$V,'selected')?> ><?=$V?></option>
           <?php endforeach; ?>
            <option data-divider="true"></option>
            <option data-icon="icon-tags" value="copy">copy</option>
            <option data-divider="true"></option>
            <option data-icon="icon-remove" value="delete">delete</option>
        </select>
        <input class="input-medium dir-part" name="Key" data-pk="<?=$K?>" type="text" value="<?=$D['Key']?>" />
        <div class="handle"><i class="icon-resize-vertical"></i></div>
        <input class="input-xxlarge dir-part" name="Value" data-pk="<?=$K?>" type="text" value="<?=$this($D['Value'])?>" />
    </div>
</div>
<?php endforeach; ?>
<div id="new-directive">
    <div class="input-prepend">
        <select name="Name"  class="selectpicker dir-part" data-width="auto" style="min-width: 70px;" data-pk="" >
            <option value="">pick one</option>
           <?php foreach( $DirectiveNames as $V ): ?>
            <option value="<?=$V?>" ><?=$V?></option>
           <?php endforeach; ?>
        </select>
        <input class="input-medium dir-part" name="Key" data-pk="" type="text" value="" placeholder="key" />
        <input class="input-xxlarge dir-part" name="Value" data-pk="" type="text" value="" placeholder="value" />
    </div>
</div>
</div>



<script>
$(document).ready(function()
{
	$('.selectpicker').selectpicker();

    $('#directives-sortable').sortable({forceHelperSize:true,opacity:.9,handle:'div.handle',placeholder:'ui-state-highlight',axis:'y',
        helper:function(e,ui) {
            ui.children().each(function() {
                $(this).width($(this).width());
            });
            return ui;
        },
        update:function(e,ui) {
            itemid = ui.item.attr('id');
            nextid = ui.item.next().attr('id');
            $.ajax({ url:url,
                    data:{D_id:itemid,NextD_id:nextid}}).done(function(){ajfDirectives();});
        }});

});
</script>

