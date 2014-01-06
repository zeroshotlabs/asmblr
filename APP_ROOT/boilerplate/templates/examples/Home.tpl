
<p>These examples, as well as this entire site in general, show how to perform
typical web development tasks using Framewire.</p>

<p>This page demonstrates:</p>

<ul>
    <li>Dynamic HTML Template swapping based on the request environment.</li>
    <li>MySQL and Mongo connection, querying and storage of form data.</li>
    <li>Multi-form handling, including population, file upload, multi-button submit and validation.</li>
    <li>URL and form field creation, error handling, and messaging in HTML templates.</li>
    <li>Site-wide debug toggling of request (<code>$ps</code>), HTML templating (<code>$html</code>),
        MySQL CRUDC (<code>$mysql</code>), MySQLSet statement templating (<code>$mysqls</code>),
        SQLSrv CRUDC (<code>$sqlsrv</code>), SQLSrvSet (<code>$sqlsrvs</code>) and MongoDB
        (<code>$mongo</code>) components.</li>
    <li>General request processing logic, based on multiple forms and conditions.</li>
</ul>

<p>The relevant files are:</p>
<ul>
    <li><code>APP_ROOT/Routines/General.inc</code></li>
    <li><code>APP_ROOT/SQL/</code></li>
    <li><code>APP_ROOT/HTML/examples/</code></li>
    <li><code>APP_ROOT/HTML/Base.tpl</code></li>
</ul>

<p>The Framewire Boot File (FBF), <code>DOC_ROOT/fwboot.php</code>, should be
reviewed first.</p>

<p style="height: 35px;"></p>


<div style="min-height: 315px;">
    <h3>Database Connections</h3>
    <ul id="db-tab-nav" class="nav nav-tabs">
        <li class="active"><a id="mongo-tab" href="#mongo-tab-content" data-toggle="tab">Mongo</a></li>
        <li class=""><a id="mysql-tab" href="#mysql-tab-content" data-toggle="tab">MySQL</a></li>
        <li class=""><a id="sqlsrv-tab" href="#sqlsrv-tab-content" data-toggle="tab">SQL Server</a></li>
    </ul>
    <div id="db-tab-content" class="tab-content">
<?php
/*
Utilize AjaxFrags to populate each of our tabs.  Note that the id of the
tab corresponds to the Template name in HTML/ajf/
*/?>
        <div class="tab-pane fade in active" id="mongo-tab-content"><?php $this->{'ajf_mongo-tab-content'}(); ?></div>
        <div class="tab-pane fade in" id="mysql-tab-content"><?php $this->{'ajf_mysql-tab-content'}(); ?></div>
        <div class="tab-pane fade in" id="sqlsrv-tab-content"><?php $this->{'ajf_sqlsrv-tab-content'}(); ?></div>
    </div>
</div>

<?php
/*
Add the HTML/examples/db-tabs-js.tpl template to the 'ajax' stack for
later rendering.
See Base.tpl for the corresponding Unstack() call.
*/?>
<?php $this->Stack('examples_db-tabs-js','ajax'); ?>


<p style="height: 35px;"></p>
<h2 id="fileform">Form Test</h2>
<p>This form tests every validation type from the <code>enUS</code> class.</p>

<?php
/*
Display the one-time FormMsg, set in Routines/Request.inc::Examples()
*/?>
<div class="alert-container">
    <?=$msg->Error('FormMsg')?>
</div>

<?php
/*
The form's action URL is formed by the $lp LinkPage object.

Here we use a defined change string - see Routines/Request.inc::Examples() -
and append a fragment so the form is scrolled to upon submittal.
*/?>
<form method="post" action="<?=$lp('',$page->FormChangeString.'#fileform')?>" enctype="multipart/form-data">

    <label class="control-label" for="FirstName">Name</label>
    <div class="controls row-fluid">
<?php
/*
$vr, the ValidationReport object, is used to highlight which fields are
invalid.

Below, we call it as a function with the field name as a parameter to
return a CSS class if the field didn't validate.  The returned value is
set during object instaniation - see fwboot.php::Go().
*/
?>
        <div class="span2 row control-group <?=$vr('Prefix')?>">
        <select name="Prefix" class="span12">
            <option value="">-prefix-</option>
<?php
/*
Use a pre-defined locale-specific listing of name prefixes to populate
the select options.

Again, $this is used to HTML encode output.  It's also used here to
determine whether an option is selected, without having to first check
the $_POST array - see enUSHTMLSet::__invoke().
*/?>
           <?php foreach( \asm\enUS::PrefixListing() as $V ): ?>
            <option <?=$this('Prefix',$_POST,$V,'selected')?> value="<?=$this($V)?>"><?=$this($V)?></option>
           <?php endforeach; ?>
        </select>
        </div>
        <div class="span3 row control-group <?=$vr('FirstName')?>">
<?php
/*
Here $this is used to populate a form field from $_POST, without first
having to check if it's set.  The field's value will also be correctly
HTML encoded.
*/?>
            <input type="text" placeholder="First Name" class="span12" name="FirstName" value="<?=$this('FirstName',$_POST)?>">
        </div>
        <div class="span3 row control-group <?=$vr('LastName')?>">
            <input type="text" placeholder="Last Name" class="span12" name="LastName" value="<?=$this('LastName',$_POST)?>">
        </div>
        <div class="span2 row control-group <?=$vr('Suffix')?>">
        <select name="Suffix" class="span12">
            <option value="">-suffix-</option>
           <?php foreach( \asm\enUS::SuffixListing() as $V ): ?>
            <option <?=$this('Suffix',$_POST,$V,'selected')?> value="<?=$this($V)?>"><?=$this($V)?></option>
           <?php endforeach; ?>
        </select>
        </div>
    </div>

    <label class="control-label" for="Address1">Address</label>
    <div class="controls row-fluid">
        <div class="span10 row control-group <?=$vr('Address1')?>">
            <input type="text" placeholder="Address 1" name="Address1" class="span12" value="<?=$this('Address1',$_POST)?>">
        </div>
    </div>
    <div class="controls row-fluid">
        <div class="span10 row control-group <?=$vr('Address2')?>">
            <input type="text" placeholder="Address 2" name="Address2" class="span12" value="<?=$this('Address2',$_POST)?>">
        </div>
    </div>
    <div class="controls row-fluid">
        <div class="span4 row control-group <?=$vr('City')?>">
            <input type="text" placeholder="City" name="City" class="span12" value="<?=$this('City',$_POST)?>">
        </div>
        <div class="span4 row control-group <?=$vr('State')?>">
            <select name="State" class="<?=$vr('State')?>">
                <option value="">- state -</option>
               <?php foreach( \asm\enUS::StateListing() as $K => $V ): ?>
                <option <?=$this('State',$_POST,$K,'selected')?> value="<?=$this($K)?>"><?=$this($V)?></option>
               <?php endforeach; ?>
            </select>
        </div>
        <div class="span2 row control-group <?=$vr('ZipCode')?>">
            <input type="text" placeholder="Zip Code" name="ZipCode" class="span12" value="<?=$this('ZipCode',$_POST)?>">
        </div>
    </div>

    <div class="controls row-fluid">
        <div class="span4 row">
        <label class="control-label" for="Email">Email</label>
        </div>
        <div class="span2 row">
        <label class="control-label" for="Age">Age</label>
        </div>
        <div class="span4 row">
        <label class="control-label" for="PhoneNumber">Phone</label>
        </div>
    </div>
    <div class="controls row-fluid">
        <div class="span4 row control-group <?=$vr('Email')?>">
            <input type="text" placeholder="Email" name="Email" class="span12" value="<?=$this('Email',$_POST)?>">
        </div>
        <div class="span2 row control-group <?=$vr('Age')?>">
            <input type="text" placeholder="Age" name="Age" class="span12" value="<?=$this('Age',$_POST)?>">
        </div>
        <div class="span4 row control-group <?=$vr('PhoneNumber')?>">
            <input type="text" placeholder="Phone Number" name="PhoneNumber" class="span12" value="<?=$this('PhoneNumber',$_POST)?>">
        </div>
    </div>

    <div class="controls row-fluid">
        <div class="span4 row">
        <label class="control-label" for="CCN">CC Number</label>
        </div>
        <div class="span2 row">
        <label class="control-label" for="IP">IP</label>
        </div>
        <div class="span4 row">
        <label class="control-label" for="SSN">SSN</label>
        </div>
    </div>
    <div class="controls row-fluid">
        <div class="span4 row control-group <?=$vr('CCN')?>">
            <input type="text" placeholder="CC Number" name="CCN" class="span12" value="<?=$this('CCN',$_POST)?>">
             <span class="help-block">5105105105105100 will work</span>
        </div>
        <div class="span2 row control-group <?=$vr('IP')?>">
            <input type="text" placeholder="xxx.xxx.xxx.xxx" name="IP" class="span12" value="<?=$this('IP',$_POST)?>">
        </div>
        <div class="span4 row control-group <?=$vr('SSN')?>">
            <input type="text" placeholder="SSN" name="SSN" class="span12" value="<?=$this('SSN',$_POST)?>">
        </div>
    </div>

    <label class="control-label" for="Desc">Description</label>
    <div class="controls row-fluid">
        <div class="span10 row control-group <?=$vr('Desc')?>">
            <textarea name="Desc" rows="4" class="span12"><?=$this('Desc',$_POST)?></textarea>
        </div>
    </div>

    <div class="controls row-fluid">
        <div class="span4 row">
        <label class="control-label" for="Username">Username</label>
        </div>
        <div class="span3 row">
        <label class="control-label" for="Password">Password</label>
        </div>
        <div class="span3 row">
        </div>
    </div>
    <div class="controls row-fluid">
        <div class="span4 row control-group <?=$vr('Username')?>">
            <input type="text" placeholder="Username" name="Username" class="span12" value="<?=$this('Username',$_POST)?>">
        </div>
        <div class="span3 row control-group <?=$vr('Password')?>">
            <input type="password" placeholder="Password" name="Password" class="span12" value="<?=$this('Password',$_POST)?>">
        </div>
        <div class="span3 row control-group <?=$vr('Password')?>">
            <input type="password" placeholder="Confirm" name="Password2" class="span12" value="<?=$this('Password2',$_POST)?>">
        </div>
    </div>

    <div class="controls row-fluid">
        <div class="span3 row">
        <label class="control-label" for="HAU[]">How did you hear about us?</label>
        </div>
        <div class="span3 row">
        <label class="control-label" for="FileUp[]">File Upload</label>
        </div>
        <div class="span4 row">
        <label class="control-label" for="SendEmail">Email yourself a copy?</label>
        </div>
    </div>

    <div class="controls row-fluid">
        <div class="span3 row control-group <?=$vr('HAU')?>">
            <span class="help-block">Check at least <b>two</b>.</span>
<?php
/*
Here we use a custom listing of values to generate a set of checkboxes.

$this is also used to determine if an checkbox is checked.
*/?>
           <?php foreach( $HAU as $K => $V ): ?>
            <label class="checkbox">
                <input type="checkbox" name="HAU[]" <?=$this('HAU',$_POST,$K,'checked')?> value="<?=$this($K)?>">
                <?=$this($V)?>
            </label>
           <?php endforeach; ?>
        </div>
        <div class="span3 row control-group <?=$vr('FileUp')?>">
           <?php for( $i = 2; $i > 0; --$i ): ?>
            <div class="fileupload fileupload-new" data-provides="fileupload">
                <span class="btn btn-file">
                    <span class="fileupload-new">Select file</span>
                    <span class="fileupload-exists">Change</span>
                    <input type="file" name="FileUp[]" />
                </span> <span class="fileupload-preview"></span> <a href="#" class="close fileupload-exists" data-dismiss="fileupload" style="float: none">x</a>
            </div>
           <?php endfor;?>
        </div>
        <div class="span4 row control-group <?=$vr('EmailACopy')?>">
            <input type="text" placeholder="email address" name="EmailACopy" class="span12" value="<?=$this('EmailACopy',$_POST)?>">
            <span class="help-block"><small>WINDOWS Users: This probably won't work for you.</small></span>
        </div>
    </div>
<?php
/*
The submit buttons are kept as a separate template because they require
additional logic to be correctly displayed.
*/?>
    <div id="submit-buttons" class="control row-fluid center"><?php $this->{'ajf_submit-buttons'}(); ?></div>
</form>

