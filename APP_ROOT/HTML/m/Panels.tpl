
<div data-role="panel" id="nav-panel" data-position="left" data-display="reveal">
    <ul data-role="listview" data-icon="false" data-global-nav="demos">
        <li data-role="list-divider">Pages</li>
       <?php foreach( $PL as $P ): ?>
        <li>
            <a href="<?=$lp('Page','>'.(string)$P['_id'])?>"><?=$this($P['Name'])?><small> - <?=$this($P['Path'])?></small></a>
        </li>
       <?php endforeach; ?>

        <li data-role="list-divider">Templates</li>
       <?php foreach( $TL as $T ): ?>
        <li>
            <a href="<?=$lp('Template','>'.(string)$T['_id'])?>"><?=$this($T['Name'])?></a>
        </li>
       <?php endforeach; ?>

    </ul>
</div>

