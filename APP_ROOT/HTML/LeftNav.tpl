<?php
if( !empty($P) )
{
    $sts = $P['Status'];
    $type = 'page';
}
else if( !empty($S) )
{
    $sts = $S['Status'];
    $type = 'site';
}

if( !empty($sts) )
{
    if( $sts === 'Active' )
    {
        $page->sts_html = "{$type} is on";
        $page->sts_title = 'click to turn off';
        $page->sts_class = 'btn-success';
    }
    else
    {
        $page->sts_html = "{$type} is off";
        $page->sts_title = 'click to turn on';
        $page->sts_class = 'btn-danger';
    }
}
?>
<div class="well">
<div class="btn-group pull-right" style="padding: 0; margin: -20px -20px 2px 0;">
    <div class="btn-group">
       <?php if( $page->ActiveNav === 'Site' ): ?>
        <button class="btn btn-mini nav-btn site-status <?=$page->sts_class?>" title="<?=$page->sts_title?>" data-toggle="button" data-status="<?=$S['Status']?>"><?=$page->sts_html?></button>
       <?php elseif( $page->ActiveNav === 'Page' ): ?>
        <button class="btn btn-mini nav-btn page-status <?=$page->sts_class?>" title="<?=$page->sts_title?>"data-toggle="button" data-status="<?=$P['Status']?>"><?=$page->sts_html?></button>
        <button class="btn btn-mini nav-btn delete_page" data-toggle="modal" data-target="#page_delete"><i title="delete page" class="icon-remove"></i></button>
       <?php elseif( $page->ActiveNav === 'Template' ): ?>
        <button class="btn btn-mini nav-btn delete_template" data-toggle="modal" data-target="#template_delete"><i title="delete template" class="icon-remove"></i></button>
       <?php endif; ?>
    </div>
    <div class="btn-group">
        <button class="btn btn-mini nav-btn new-page" data-type="page" data-url="<?=$lr('page_create')?>"><i title="new page" class="icon-list-alt"></i></button>
        <button class="btn btn-mini nav-btn new-template" data-type="text" data-url="<?=$lr('template_create')?>" data-placeholder="TemplateName" data-name="Name"><i title="new template" class="icon-edit"></i></button>
        <button class="btn btn-mini nav-btn new-content" data-type="text" data-url="<?=$lr('content_create')?>" data-placeholder="ContentName" data-name="Name"><i title="new content" class="icon-file"></i></button>
    </div>
</div>

<div class="clearfix"></div>

<?php if( $page->ActiveNav !== 'Site' ): ?>
<div class="pull-right" style="margin: 0 -15px 0 0; padding: 0;">
<small><a href="<?=$lp('Site','>'.$S['_id'])?>"><?=$S['Domain']?></a></small>
</div>
<?php endif; ?>

<div style="min-height: 65px;">
   <?php if( $page->ActiveNav === 'Site' ): ?>
    <p class="nav-header">site</p>
    <h3><a href="#" class="set-domain" data-type="text" data-url="<?=$lr('site_set_domain')?>" data-name="Domain"><?=$S['Domain']?></a>
        <a target="_blank" href="<?=asm('lp')->Link('Home')?>"><img src="<?=$ls('/img/ext-link.png')?>" /></a></h3>
    <small class="mini-header2">
        <a href="#" class="set-baseurl" data-type="url" data-url="<?=$lr('site_set_baseurl')?>" data-emptytext="default base URL" data-name="BaseURL"><?=$S['BaseURL']?></a>
    </small>
   <?php elseif( $page->ActiveNav === 'Page' ): ?>
    <p class="nav-header">page</p>
    <h3><a href="#" class="set-name" data-type="text" data-url="<?=$lr('page_set_name')?>" data-name="Name"><?=$P['Name']?></a>
        <a target="_blank" href="<?=asm('lp')->Link($P['Name'])?>"><img src="<?=$ls('/img/ext-link.png')?>" /></a></h3>
    <small class="mini-header2">
        <a href="#" class="set-path" data-type="text" data-url="<?=$lr('page_set_path')?>" data-name="Path"><?=$P['Path']?></a>
    </small>
   <?php elseif( $page->ActiveNav === 'Template' ): ?>
    <p class="nav-header">template</p>
    <h3><a href="#" class="set-name" data-type="text" data-url="<?=$lr('template_set_name')?>" data-name="Name"><?=$T['Name']?></a></h3>
   <?php endif; ?>
</div>
</div>

<div class="well">
    <form class="form-search">
    <div  style="text-align: center; padding: 0; margin: -10px 0 20px 0;">
        <input style="width: 75%;" placeholder="search" type="text" class="search-query">
    </div>
    </form>

    <ul class="nav nav-list" style=" margin: 0; padding: 0 0 0 0;">
        <li class="nav-header">recent</li>
       <?php foreach( $PL as $P ): ?>
        <li>
            <a href="<?=$lp('Page','>'.(string)$P['_id'])?>"><i class="icon-list-alt"></i> <?=$P['Name']?></a>
            <div style="margin: -9px 0 0 22px; font-size: .93em;">
            <small><a href="<?=$lp('Page','>'.(string)$P['_id'])?>#directives_tab">directives</a>&nbsp;|&nbsp;<a href="<?=$lp('Page','>'.(string)$P['_id'])?>#routine_tab">routine</a></small>
            </div>
        </li>
       <?php endforeach; ?>
       <?php foreach( $TL as $T ): ?>
        <li>
            <a href="<?=$lp('Template','>'.(string)$T['_id'])?>"><i class="icon-edit"></i> <?=$T['Name']?></a>
            <div style="margin: -9px 0 0 22px; font-size: .93em;">
            <small><a href="<?=$lp('Template','>'.(string)$T['_id'])?>#routine_tab">routine</a>&nbsp;|&nbsp;<a href="<?=$lp('Template','>'.(string)$T['_id'])?>#body_tab">body</a></small>
            </div>
        </li>
       <?php endforeach; ?>
    </ul>
</div>
