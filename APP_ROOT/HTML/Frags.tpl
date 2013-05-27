
@@@Error404
<h1>Not Found</h1>
<div>
    <p>Ten-four, that's a four-oh-four.</p>
</div>


@@@Error500
<h1>Server Error</h1>


@@Breadcrumb
<ul class="breadcrumb">
<li><a href="<?=$lp('Home')?>">Home</a> <span class="divider">|</span></li>
<li><a href="<?=$lp('Site','>'.$S['_id'])?>"><?=$page->Domain?></a><?=$page->Path?></li>
<?php if( !empty($P['Name']) ): ?>
<li><span class="divider">|</span> <?=$P['Name']?></li>
<?php endif; ?>
<li>
&nbsp;&nbsp;<span style="font-size: .79em;"><a href="#" class="set-status" data-type="select" data-url="<?=$lr('page_set_status')?>" data-value="<?=$P['Status']?>" data-name="Status"><?=strtolower($P['Status'])?></a></span>
</li>
<li class="pull-right">
    <a title="delete" data-toggle="modal" data-target="#page_delete" href="#">
    <img alt="delete" src="<?=$ls('/img/glyphicons_256_delete.png')?>">
    </a>
</li>
</ul>

<ul class="breadcrumb">
<li><a href="<?=$lp('Home')?>">Home</a> <span class="divider">|</span></li>
<li><?=$page->Domain?></li>
<li>
&nbsp;&nbsp;<small><a href="#" class="set-status" data-type="select" data-url="<?=$lr('site_set_status')?>" data-value="<?=$S['Status']?>" data-name="Status"><?=strtolower($S['Status'])?></a></small>
</li>
</ul>



<ul class="breadcrumb">
<li><a href="<?=$lp('Home')?>">Home</a> <span class="divider">|</span></li>
<li><a href="<?=$lp('Site','>'.$S['_id'])?>"><?=$page->Domain?></a><span class="divider">|</span><?=$page->Name?></li>

<li class="pull-right">
    <a title="delete" data-toggle="modal" data-target="#template_delete" href="#">
    <img alt="delete" src="<?=$ls('/img/glyphicons_256_delete.png')?>">
    </a>
</li>

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

