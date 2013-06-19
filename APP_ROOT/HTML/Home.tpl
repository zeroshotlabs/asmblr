
<div class="row-fluid">
<div class="text-center span6">
    <h1>current sites</h1>
</div>
</div>

<div class="row-fluid">
<div class="text-center span6">
   <?php foreach( $SS as $S ): ?>
   <div style="margin: 15px 0 15px 0;">
    <a href="<?=$lp('Site',">".(string)$S['_id'])?>" class="btn btn-block <?=$S['Status']==='Active'?'btn-success':'btn-warning'?>"><?=$S['Domain']?></a>
    </div>
   <?php endforeach; ?>
</div>
</div>

<div class="row-fluid">
<div class="text-center span6">
</div>
</div>
<div class="row-fluid">
<div class="text-center span6">
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
