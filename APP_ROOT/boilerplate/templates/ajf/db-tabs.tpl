

@@@mongo-tab-content
<?php
/*
For each of these templates, conditionally display the connection form or
connection options.

NOTE: For template files with multiple templates, nothing - including
comments - can come before the first template marker.

Each of the forms, buttons and links below are bound to Javascript
events - see examples/db-tabs-js.tpl - which fire requests to either
AjaxFrags::Execute() (/ajf/) or JSON::Execute() (/json/) Pages in
Routines/Ajax.inc.

As with other templates, $page, $this and $lp are used for in-page
variable flags, encoding and URL creation, respectively.
*/?>

<?php if( $page->MongoOnline == TRUE ): $H = $_SESSION['MongoConnect']['Hostname']; $DB = $_SESSION['MongoConnect']['Database']; ?>
    <div class="alert alert-info">
        <p>connected to <?=$H?></p>
        <a role="button" class="btn" href="<?=$lp('AjaxFrags','>mongo-tab-content?mongodisco=1')?>">
            <i class="iconic-o-x"></i> Disconnect
        </a>
    </div>

    <?php if( $page->MongoDBExists === FALSE ): ?>
    <div class="alert alert-error">
        Database <i><?=$DB?></i> doesn't exist -
        <a href="<?=$lp('AjaxFrags','>mongo-tab-content?mongoinitdb=1')?>">Create it now</a> before proceeding.
    </div>
    <?php else: ?>
    <div class="alert alert-info">
        <p>Connected to database <i><?=$DB?></i></p>
        <a role="button" class="btn" href="<?=$lp('AjaxFrags','>mongo-tab-content?mongoinitdb=1')?>">
            <i class="iconic-trash"></i> Recreate
        </a>
    </div>
    <?php endif; ?>

<?php else: ?>
<p>Connect to Mongo to write form data to it.  These parameters will persist in a session until your browser is closed,
or you disconnect.</p>
<p>Typically only the database name is required for Mongo.</p>

<div class="label-container">
    <div class="label label-important"></div>
</div>

<form id="mongoconnectform" class="form-inline" style="margin: 0;" method="post" action="<?=$lp('JSON','>mongo-connect?')?>">
    <input placeholder="hostname" id="Hostname" class="input-small" type="text" name="Hostname">
    <input placeholder="username" id="Username" class="input-small" type="text" name="Username">
    <input placeholder="password" id="Password" class="input-small" type="password" name="Password">
    <input placeholder="database" id="Database" class="input-small" type="text" name="Database">
    <button type="submit" value="MongoConnect" class="btn"><i class="iconic-link"></i> Connect</button>
</form>
<?php endif; ?>



@@@mysql-tab-content
<?php if( $page->MySQLOnline == TRUE ): $H = $_SESSION['MySQLConnect']['Hostname']; $DB = $_SESSION['MySQLConnect']['Database']; ?>
<div class="alert alert-info">
    <p>connected to <?=$H==='.'?'localhost':$H?></p>
    <a role="button" class="btn" href="<?=$lp('AjaxFrags','>mysql-tab-content?mysqldisco=1')?>">
        <i class="iconic-o-x"></i> Disconnect
    </a>
</div>

    <?php if( $page->MySQLDBExists === FALSE ): ?>
    <div class="alert alert-error">
        Database <i><?=$DB?></i> doesn't exist -
        <a href="<?=$lp('AjaxFrags','>mysql-tab-content?mysqlinitdb=1')?>">Create it now</a> before proceeding.
    </div>
    <?php else: ?>
    <div class="alert alert-info">
        <p>Connected to database <i><?=$DB?></i></p>
        <a role="button" class="btn" href="<?=$lp('AjaxFrags','>mysql-tab-content?mysqlinitdb=1')?>">
            <i class="iconic-trash"></i> Recreate
        </a>
    </div>
    <?php endif; ?>

<?php else: ?>
<p>Connect to MySQL to write form data to it.  These parameters will persist in a session until your browser is closed,
or you disconnect.</p>
<p>It's easiest to specify the root user.  If on Windows, use a period for the hostname.</p>

<div class="label-container">
    <div class="label label-important"></div>
</div>

<form id="mysqlconnectform" class="form-inline" style="margin: 0;" method="post" action="<?=$lp('JSON','>mysql-connect?')?>">
    <input placeholder="hostname" id="Hostname" class="input-small" type="text" name="Hostname">
    <input placeholder="username" id="Username" class="input-small" type="text" name="Username">
    <input placeholder="password" id="Password" class="input-small" type="password" name="Password">
    <input placeholder="database" id="Database" class="input-small" type="text" name="Database">
    <button type="submit" value="MySQLConnect" class="btn"><i class="iconic-link"></i> Connect</button>
</form>
<?php endif; ?>



@@@sqlsrv-tab-content
<?php if( $page->SQLSrvOnline == TRUE ): $H = $_SESSION['SQLSrvConnect']['Hostname']; $DB = $_SESSION['SQLSrvConnect']['Database']; ?>
<div class="alert alert-info">
    <p>connected to <?=$H==='.'?'localhost':$H?></p>
    <a role="button" id="sqlsrv-disco" class="btn" href="<?=$lp('AjaxFrags','>sqlsrv-tab-content?sqlsrvdisco=1')?>">
        <i class="iconic-o-x"></i> Disconnect
    </a>
</div>

   <?php if( $page->SQLSrvDBExists === FALSE ): ?>
    <div class="alert alert-error">
        The tables 'Profile' and/or 'FileUpload' don't appear to exist -
        <a href="<?=$lp('AjaxFrags','>sqlsrv-tab-content?sqlsrvinitdb=1')?>">Create them now</a> before proceeding.
    </div>
   <?php else: ?>
    <div class="alert alert-info">
        <p>Connected to database <i><?=$DB?></i></p>
        <a role="button" class="btn" href="<?=$lp('AjaxFrags','>sqlsrv-tab-content?sqlsrvinitdb=1')?>">
            <i class="iconic-trash"></i> Recreate
        </a>
    </div>
   <?php endif; ?>

<?php else: ?>

<p>Connect to SQL Server to write form data to it.  These parameters will persist in a session until your browser is closed,
or you disconnect.</p>
<p>The database specified must already exist.  It's easiest to specify the <code>sa</code> or other admin account
for testing purposes.</p>
<p>For local dev. installs the hostname is likely similar to <code>yourcomputer\sqlexpress</code>.</p>

<div class="label-container">
    <div class="label label-important"></div>
</div>

<form id="sqlsrvconnectform" class="form-inline" style="margin: 0;" method="post" action="<?=$lp('JSON','>sqlsrv-connect')?>">
    <input placeholder="hostname" id="Hostname" class="input-small" type="text" name="Hostname">
    <input placeholder="username" id="Username" class="input-small" type="text" name="Username">
    <input placeholder="password" id="Password" class="input-small" type="password" name="Password">
    <input placeholder="database" id="Database" class="input-small" type="text" name="Database">
    <button type="submit" value="SQLSrvConnect" class="btn"><i class="iconic-link"></i> Connect</button>
</form>
<?php endif; ?>

