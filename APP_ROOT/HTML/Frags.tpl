
@@@Error404
<h1>Not Found</h1>
<div>
    <p>Ten-four, that's a four-oh-four.</p>
</div>


@@@Error500
<h1>Server Error</h1>


@@@Breadcrumb
<ul class="breadcrumb">
<li><a href="<?=$lp('Home')?>">all sites</a></li>

<?php if( $page->ActiveNav === 'Site' ): ?>
<li>
<span class="divider">|</span> <?=$S['Domain']?>
&nbsp;<a target="_blank" href="<?=asm('lp')->Link('Home')?>"><img src="<?=$ls('/img/ext-link.png')?>" /></a>
</li>
<li>
&nbsp;&nbsp;<small><a href="#" class="set-status" data-type="select" data-url="<?=$lr('site_set_status')?>" data-value="<?=$S['Status']?>" data-name="Status"><?=strtolower($S['Status'])?></a></small>
</li>
<?php elseif( $page->ActiveNav === 'Page' ): ?>
<li>
<span class="divider">|</span> <a href="<?=$lp('Site','>'.$S['_id'])?>"><?=$S['Domain']?></a><?=$P['Path']?>
&nbsp;<a target="_blank" href="<?=asm('lp')->Link($P['Name'])?>"><img src="<?=$ls('/img/ext-link.png')?>" /></a>
</li>
<li> <span class="divider">|</span> <?=$P['Name']?></li>
<li>
&nbsp;&nbsp;<span style="font-size: .79em;"><a href="#" class="set-status" data-type="select" data-url="<?=$lr('page_set_status')?>" data-value="<?=$P['Status']?>" data-name="Status"><?=strtolower($P['Status'])?></a></span>
</li>
<li class="pull-right">
    <a title="delete" data-toggle="modal" data-target="#page_delete" href="#">
    <img alt="delete" src="<?=$ls('/img/glyphicons_256_delete.png')?>">
    </a>
</li>
<?php elseif( $page->ActiveNav === 'Template' ): ?>
<li>
<span class="divider">|</span> <a href="<?=$lp('Site','>'.$S['_id'])?>"><?=$S['Domain']?></a>
</li>
<li> <span class="divider">|</span> <?=$T['Name']?></li>
<li class="pull-right">
    <a title="delete" data-toggle="modal" data-target="#template_delete" href="#">
    <img alt="delete" src="<?=$ls('/img/glyphicons_256_delete.png')?>">
    </a>
</li>
<?php endif; ?>
</ul>

@@@Footer
<div class="page-footer">
    <div class="footer-left-side">
        <p>Built by <a href="http://www.stackware.com/">Stackware PHP Website Development</a>.</p>
        <p>asmblr is a trademark of Stackware, LLC.</p>
    </div>
    <div class="footer-right-side">
        <p>Framewire <a href="http://www.framewire.org/">Web Application PHP Framework</a></p>
        <p><?=round((microtime(TRUE)-START_TIME)*1000,1)?>ms</p>
    </div>
</div>

