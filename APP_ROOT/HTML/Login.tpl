
<div class="row-fluid">
<div class="center span4" id="login">
    <div class="label-container"><div class="label label-important"></div></div>
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
            <button type="submit" value="Submit" name="Submit" class="btn btn-primary">Login</button>
        </div>
    </form>
</div>
</div>

<div class="row-fluid">
<div class="center span4" id="register">
    <div class="label-container"><div class="label label-important form-alert"></div></div>
    <form id="register-form" method="post" action="<?=$lr('account_create')?>">
        <div class="controls row-fluid">
            <div class="span12 row control-group">
                <input type="text" placeholder="Email" name="Email" class="span12">
            </div>
        </div>
        <div class="controls row-fluid">
            <div class="span12 row control-group">
                <input type="password" placeholder="Password" name="Password" class="span12">
            </div>
        </div>
        <div class="controls row-fluid">
            <div class="span12 row control-group">
                <input type="text" placeholder="Name" name="Name" class="span12">
            </div>
        </div>
        <div class="controls row-fluid">
            <div class="span12 row control-group">
                <input type="text" placeholder="Company" name="Company" class="span12">
            </div>
        </div>
        <div class="control row-fluid">
            <button type="submit" value="Submit" name="Submit" class="btn btn-primary">Register</button>
        </div>
    </form>
</div>
</div>

