
<div class="center span4" id="createsite">
    <div class="label label-important"></div>
<form id="createsite-form" method="post" action="<?=$lr('site_create')?>">
    <div class="controls row-fluid">
        <div class="span12 row control-group">
            <input type="text" placeholder="Domain" name="Domain" class="span12">
        </div>
    </div>
    <div class="control row-fluid">
        <button type="submit" id="createsite" value="Submit" name="Submit" class="btn btn-primary">Create site</button>
    </div>
</form>
</div>

<?php $this->Stack('JSCreateSite','ajax'); ?>

