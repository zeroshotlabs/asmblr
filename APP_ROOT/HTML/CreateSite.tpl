
<div id="createsite" class="span5">
<form id="createsite-form" method="post" action="<?=$lr('site_create')?>">
    <div class="row center">
        <div class="label label-important"></div>
    </div>
    <div class="row center">
        <input class="input-block-level" type="text" placeholder="domain.com" name="Domain">
    </div>
    <div class="row center">
        <button class="btn-block btn btn-primary" type="submit" value="Submit" name="Submit">Create site</button>
    </div>
</form>
</div>


<?php $this->Stack('JSCreateSite','ajax'); ?>

