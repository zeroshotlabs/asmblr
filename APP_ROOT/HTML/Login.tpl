
<div class="center span4" id="login">
    <div class="label-important"></div>
<form id="login-form" method="post" action="<?=$lr('account_auth')?>">
    <div class="controls row-fluid">
        <div class="span12 row control-group">
            <input type="text" placeholder="Username" name="Email" class="span12">
        </div>
    </div>
    <div class="controls row-fluid">
        <div class="span12 row control-group">
            <input type="password" placeholder="Password" name="Password" class="span12">
        </div>
    </div>
    <div class="control row-fluid">
        <button type="submit" id="login" value="Submit" name="Submit" class="btn btn-primary">Login</button>
    </div>
</form>
</div>

<?php $this->Stack('JSLogin','ajax'); ?>

