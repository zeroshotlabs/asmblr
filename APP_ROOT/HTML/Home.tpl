
<div class="row-fluid">
<div class="text-center span6">
    <h1>current sites</h1>
</div>
</div>

<div class="row-fluid">
<div class="text-center span6">
   <?php if( count($SS) === 0 ) :?>
    <h4 class="text-warning">none</h4>
   <?php else: ?>
   <?php foreach( $SS as $S ): ?>
   <div style="margin: 15px 0 15px 0;">
    <a href="<?=$lp('Site',">".(string)$S['_id'])?>" class="btn btn-block <?=$S['Status']==='Active'?'btn-success':'btn-warning'?>"><?=$S['Domain']?></a>
    </div>
   <?php endforeach; ?>
   <?php endif; ?>
</div>
</div>

<div class="row-fluid">
    <div class="span5" style="margin-top: 20px;">
        <dl>
            <dt class="text-info"><i title="new page" class="icon-list-alt"></i> Page</dt>
            <dd>Ties a URL to some logic, configuration settings, and templates.</dd>
        </dl>
        <dl>
            <dt class="text-info"><i title="new template" class="icon-edit"></i> Template</dt>
            <dd>Store and dynamically generates HTML, CSS and Javascript.</dd>
        </dl>
        <dl>
            <dt class="text-info"><i title="new content" class="icon-file"></i> Content</dt>
            <dd>Store and index multi-lingual blocks of written content or images/video/documents.</dd>
        </dl>
        <dl>
            <dt class="text-info"><i title="upload content" class="icon-upload"></i> Data</dt>
            <dd>Store and tag ordered lists of anything.</dd>
        </dl>
    </div>
</div>


<div class="row-fluid">
<div class="text-center span6">
    <h1>create a new site</h1>
</div>
</div>

<div class="row-fluid">
    <div class="text-center span6">
    <div id="createsite">
        <form id="createsite-form" method="post" action="<?=$lr('site_create')?>">
        <div class=" text-center">
            <div class="label-container"><div class="label label-important"></div></div>
        </div>
        <div class="text-center">
            <input class="input-block-level" type="text" placeholder="some.domain.com" name="Domain">
        </div>
        <div class="text-center">
            <button class="btn-block btn btn-primary" type="submit" value="Submit" name="Submit">Create site</button>
        </div>
        </form>
    </div>
    </div>
</div>
