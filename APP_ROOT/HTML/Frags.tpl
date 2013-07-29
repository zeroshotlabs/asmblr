
@@@Footer
<div>
    <div class="pull-left">
        <h6>Built by <a href="http://www.stackware.com/">Stackware Web Development</a>.</h6>
        <h6>asmblr is a trademark of Stackware, LLC.</h6>
    </div>
    <div class="pull-right">
        <h6>Framewire <a href="http://www.framewire.org/">Web Application PHP Framework</a></h6>
        <h6><?=round((microtime(TRUE)-START_TIME)*1000,1)?>ms</h6>
    </div>
</div>



@@@Error404
<h1>Not Found</h1>
<div>
    <p>Ten-four, that's a four-oh-four.</p>
</div>


@@@Error500
<h1>Server Error</h1>



@@@Breadcrumb
<ul class="breadcrumb">

<?php if( $page->ActiveNav === 'Site' ): ?>
<li><?=$S['Domain']?>
&nbsp;<a target="_blank" href="<?=asm('lp')->Link('Home')?>"><img src="<?=$ls('/img/ext-link.png')?>" /></a>
</li>
<li>
    <a class="new-page" data-type="page" data-showbuttons="bottom" data-url="<?=$la('page_create')?>">new page</a>
</li>
<li>
    <a class="new-template" data-type="text" data-showbuttons="bottom" data-url="<?=$la('template_create')?>" data-placeholder="TemplateName" data-name="Name" href="#">new template</a>
</li>
<li>
&nbsp;&nbsp;<small><a href="#" class="set-status" data-type="select" data-url="<?=$la('site_set_status')?>" data-value="<?=$S['Status']?>" data-name="Status"><?=strtolower($S['Status'])?></a></small>
</li>
<?php elseif( $page->ActiveNav === 'Page' ): ?>
<li><a href="<?=$lp('Site','>'.$S['_id'])?>"><?=$S['Domain']?></a><?=$P['Path']?>
&nbsp;<a target="_blank" href="<?=asm('lp')->Link($P['Name'])?>"><img src="<?=$ls('/img/ext-link.png')?>" /></a>
</li>
<li> <span class="divider">|</span> <?=$P['Name']?></li>
<li>
&nbsp;&nbsp;<span style="font-size: .79em;"><a href="#" class="set-status" data-type="select" data-url="<?=$la('page_set_status')?>" data-value="<?=$P['Status']?>" data-name="Status"><?=strtolower($P['Status'])?></a></span>
</li>
<li class="pull-right">
    <a title="delete" data-toggle="modal" data-target="#page_delete" href="#">
    <img alt="delete" src="<?=$ls('/img/glyphicons_256_delete.png')?>">
    </a>
</li>
<?php elseif( $page->ActiveNav === 'Template' ): ?>
<li><a href="<?=$lp('Site','>'.$S['_id'])?>"><?=$S['Domain']?></a>
</li>
<li> <span class="divider">|</span> <?=$T['Name']?></li>
<li class="pull-right">
    <a title="delete" data-toggle="modal" data-target="#template_delete" href="#">
    <img alt="delete" src="<?=$ls('/img/glyphicons_256_delete.png')?>">
    </a>
</li>
<?php endif; ?>
</ul>

