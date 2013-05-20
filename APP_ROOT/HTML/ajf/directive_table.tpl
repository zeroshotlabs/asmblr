
<table class="table table-bordered table-striped" style="width: 100%;">
<tbody class="sortable" id="directives-sortable">
   <?php foreach( $DS as $K => $D ): ?>
    <tr id="<?=$K?>">
        <td class="text-center span1">
            <a href="#" class="pull-left handle" data-pk="<?=$K?>"><i class="icon-th-list"></i></a>
            <a href="#" class="set-directive-name editable-click" data-pk="<?=$K?>" data-type="select" data-url="<?=$lr('site_set_directive')?>" data-value="<?=$D['Name']?>" data-name="Name"><?=$D['Name']?></a>
        </td>
        <td class="text-center span2">
            <a href="#" class="set-directive-key editable-click" data-pk="<?=$K?>" data-type="text" data-url="<?=$lr('site_set_directive')?>" data-name="Key"><?=$D['Key']?></a>
        </td>
        <td>
            <a href="#" class="set-directive-value editable-click" data-pk="<?=$K?>" data-type="text" data-url="<?=$lr('site_set_directive')?>" data-name="Value"><?=$D['Value']?></a>
            <a href="#" class="pull-right cp-directive" data-pk="<?=$K?>"><i class="icon-tags"></i></a>
            <a href="#" class="pull-right del-directive" data-pk="<?=$K?>"><i class="icon-remove"></i></a>
        </td>
    </tr>
   <?php endforeach; ?>
</tbody>

<tr>
    <td colspan="3">
    <div id="diralert" class="alert alert-error"></div>
    </td>
</tr>

<tr class="new-directive">
    <td class="text-center">
    <select class="input-small" name="Name">
        <option></option>
       <?php foreach( $DirectiveNames as $V ): ?>
        <option value="<?=$V?>">$<?=$V?></option>
       <?php endforeach; ?>
    </select>
    </td>
    <td class="text-center">
        <input class="input-small" type="text" placeholder="key" value="" name="Key" >
    </td>
    <td style="vertical-align: middle;">
        <textarea class="" placeholder="value" name="Value" style="width: 75%;"></textarea>
        <button type="submit" class="btn btn-success">New</button>
    </td>
</tr>
</table>

