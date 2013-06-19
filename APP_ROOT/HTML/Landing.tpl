<div class="row-fluid">
<div class="hero-unit">
    <a href="<?=$lp('Home')?>"><img src="<?=$ls('/img/asmblr-logo.png')?>" title="Framewire PHP Framework" /></a>
    <p class="text-error">A beautiful site starts with beautiful technology.</p>
</div>
</div>

<div class="row-fluid">
    <div class="span5" style="margin-top: 20px;">
    <h4 style="color: #666;">
        asmblr lets you build a custom CMS for your clients, business and startup.
    </h4>
    <h4>&nbsp;</h4>
    <h4 style="color: #666;">
     It's PHP's first cloud-native web application platform-as-a-service (PaaS).
     </h4>
    </div>
    <div class="span5" id="login">
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
            <div class="controls row-fluid">
                <button type="submit" value="Submit" name="Submit" class="btn btn-primary">Login</button>
            </div>
        </form>
    </div>
</div>

<div class="row-fluid">
    <div class="span5" style="margin-top: 20px;">
        <dl>
            <dt class="text-info">Fully API driven.</dt>
            <dd>Build your own web interface or use ours.</dd>
        </dl>
        <dl>
            <dt class="text-info">SEO peace-at-last.</dt>
            <dd>Custom URLs, lighting-fast delivery, endless meta-tags, PageSpeed optimized.</dd>
        </dl>
        <dl>
            <dt class="text-info">A shiny new front-end.</dt>
            <dd>Easily build sites using the latest responsive design and skip the annoying theme.</dd>
        </dl>
    </div>

    <div class="span5" id="register">
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

