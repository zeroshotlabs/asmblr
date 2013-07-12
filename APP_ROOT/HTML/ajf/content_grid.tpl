
<div id="content_grid">
   <?php foreach( $CL as $K => $C ): ?>
<?php
$Class = 'thumbicon';
if( strpos($C['Type'],'image/') === 0 )
{
    $Src = asm('lp')->Link('ContentSrv').$C['Path'];
    $Class = 'thumbimg';
}
elseif( strpos($C['Type'],'css') !== FALSE )
    $Src = $ls('/img/css-icon.png');
elseif( strpos($C['Type'],'html') !== FALSE )
    $Src = $ls('/img/html-icon.png');
else if( strpos($C['Type'],'text/') === 0 || strpos($C['Type'],'php') !== FALSE )
    $Src = $ls('/img/text-icon.png');
elseif( strpos($C['Type'],'word') !== FALSE )
    $Src = $ls('/img/docx-win-icon.png');
elseif( strpos($C['Type'],'pdf') !== FALSE )
    $Src = $ls('/img/pdf-icon.png');
elseif( strpos($C['Type'],'presentation') !== FALSE )
    $Src = $ls('/img/pptx-win-icon.png');
elseif( strpos($C['Type'],'spreadsheet') !== FALSE )
    $Src = $ls('/img/xlsx-win-icon.png');
else
    $Src = $ls('/img/text-icon.png');
?>

    <div class="pull-left item">
        <a href="<?=$lp('Content',">{$C['_id']}")?>">
            <img title="<?=$Src?>" data-src="holder.js/160x120" src="<?=$Src?>" class="media-object <?=$Class?> replace_dropzone" data-contentid="<?=$C['_id']?>" alt="">
        </a>
    </div>
       <?php endforeach; ?>
</div>

<?php /*



            <small><?=$C['Path']?></small>

            <textarea id="content_body" name="Body" data-method="content_set_body" data-mode="<?=$C['Type']?>" data-hasShown=""><?=$this($C['Body'])?></textarea>

            <h2><a target="_blank" href="<?=asm('lp')->Link('ContentSrv')?><?=$C['Path']?>" >download</a></h2>
            <?=$C['Length']?> bytes <?=$C['Type']?>

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
*/ ?>
