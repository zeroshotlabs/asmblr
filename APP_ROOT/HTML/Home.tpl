
<div class="row-fluid span5">
<ul class="nav nav-list" style="padding: 0; margin: 0;">
    <li class="nav-header">sites</li>
   <?php foreach( $SS as $S ): ?>
    <li><a href="<?=$lp('Site',">".(string)$S['_id'])?>"><?=$S['Domain']?></a></li>
   <?php endforeach; ?>
</ul>
</div>

<div class="clearfix"></div>

<div class="row-fluid span5">
<div id="createsite" style="padding: 0; margin: 0;">
<form id="createsite-form" method="post" action="<?=$lr('site_create')?>">
    <div class="row center">
        <div class="label-container"><div class="label label-important"></div></div>
    </div>
    <div class="row center">
        <input class="input-block-level" type="text" placeholder="domain.com" name="Domain">
    </div>
    <div class="row center">
        <button class="btn-block btn btn-primary" type="submit" value="Submit" name="Submit">Create site</button>
    </div>
</form>
</div>
</div>
