
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
</div>

<div id="new-directive">
    <div class="input-prepend">
        <select name="Name" class="selectpicker dir-part inverse" data-width="103"  data-style="btn-warning" data-pk="" >
            <option data-divider="true">create</option>
           <?php foreach( $DirectiveNames as $V ): ?>
            <option value="<?=$V?>" ><?=$V?></option>
           <?php endforeach; ?>
        </select>
        <input class="input-medium dir-part" name="Key" data-pk="" type="text" value="" placeholder="key" />
        <div class="" style="display: inline-block; width: 18px;"></div>
        <input class="input-xxlarge dir-part" name="Value" data-pk="" type="text" value="" placeholder="value" />
    </div>
</div>

