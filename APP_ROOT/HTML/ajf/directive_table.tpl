
<table class="table table-bordered table-striped" style="width: 100%;">
<tbody class="sortable" id="directives-sortable">
   <?php foreach( $DS as $K => $D ): ?>
    <tr id="<?=$K?>">
        <td class="text-center span2">
            <a href="#" class="pull-left handle" data-pk="<?=$K?>"><i class="icon-th-list"></i></a>
            <a href="#" class="editable-click set-directive-name" data-pk="<?=$K?>" data-placement="right" data-type="select" data-value="<?=$D['Name']?>" data-name="Name"><?=$D['Name']?></a>
        </td>
        <td class="text-center span2">
            <a href="#" class="editable-click set-directive-key" data-pk="<?=$K?>" data-type="text"  data-name="Key"><?=$D['Key']?></a>
        </td>
        <td>
            <a href="#" class="editable-click set-directive-value" data-inputclass="input-xlarge" data-pk="<?=$K?>" data-type="text" data-name="Value"><?=$D['Value']?></a>
            <div class="pull-right">
                <span style="padding: 5px;"><a href="#" class="del-directive" data-pk="<?=$K?>"><i class="icon-remove"></i></a></span>
                <a href="#" class="cp-directive" data-pk="<?=$K?>"><i class="icon-tags"></i></a>
            </div>
        </td>
    </tr>
   <?php endforeach; ?>
</tbody>
</table>

