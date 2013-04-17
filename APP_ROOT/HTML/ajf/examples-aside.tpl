<?php
/*
 * This template is used in two ways:
 *  - embedded in Base.tpl during a full web request (non-ajax).
 *  - as an HTML fragment for AjaxFrags request (/ajf/examples-aside)
 *
 * Note also that it has a Routine associated with it (fwboot.php::Go())
 */?>
<div style="text-align: right;" class="well">
    <div class="center">
        <div class="label label-important">Don't leave this page public.</div>
    </div>

    <h3>Debug Toggles</h3>
<?php // Render the examples/DebugMenu.tpl template ?>
    <div><?=$this->examples_DebugMenu()?></div>
</div>

<div style="text-align: right;" class="well">

    <h3>MongoDB Profiles</h3>
   <?php if( empty($page->MongoOnline) ): ?>
    <i>not connected</i>
   <?php elseif( empty($MongoProfiles) ): ?>
    <i>none</i>
   <?php else: ?>
    <dl>
<?php
/*
If the Mongo is online and contains records, generate a listing
of them, including edit, copy and delete links.

Here, $lp and $this are used to automatically generate the correct
URLs using change strings, and escape output, respectively.
*/?>
       <?php foreach( $MongoProfiles as $P ): $id = (string) $P['_id']; ?>
        <dt>
            <a href="<?=$lp('Examples',"?mongoedit={$id}#fileform")?>"><?=$this($P['FirstName'])?> <?=$this($P['LastName'])?></a>
            <a class="iconlink" href="<?=$lp('AjaxFrags',">examples-aside?mongodelete={$id}")?>"><i class="iconic-x"></i></a>
            <a class="iconlink" href="<?=$lp('AjaxFrags',">examples-aside?mongocopy={$id}")?>"><i class="iconic-new-window"></i></a>
        </dt>
        <dt><small class="muted"><?=date('r',$P['InsertDT']->sec)?></small></dt>
       <?php endforeach; ?>
    </dl>
   <?php endif; ?>

<?php
/*
Similarly, generate listings for MySQL and SQL server.
*/?>

    <h3>MySQL Profiles</h3>
   <?php if( empty($page->MySQLOnline) ): ?>
    <i>not connected</i>
   <?php elseif( empty($MySQLProfiles) ): ?>
    <i>none</i>
   <?php else: ?>
    <dl>
       <?php foreach( $MySQLProfiles as $P ): $PID = $P['ProfileID']; ?>
        <dt>
            <a href="<?=$lp('Examples',"?mysqledit={$PID}#fileform")?>"><?=$this($P['FirstName'])?> <?=$this($P['LastName'])?></a>
            <a class="iconlink" href="<?=$lp('AjaxFrags',">examples-aside?mysqldelete={$PID}")?>"><i class="iconic-x"></i></a>
            <a class="iconlink" href="<?=$lp('AjaxFrags',">examples-aside?mysqlcopy={$PID}")?>"><i class="iconic-new-window"></i></a>
        </dt>
        <dt><small class="muted"><?=$P['InsertDT']?></small></dt>
       <?php endforeach; ?>
    </dl>
   <?php endif; ?>


    <h3>SQL Server Profiles</h3>
   <?php if( empty($page->SQLSrvOnline) ): ?>
    <i>not connected</i>
   <?php elseif( empty($SQLSrvProfiles) ):?>
    <i>none</i>
   <?php else: ?>
    <dl>
       <?php foreach( $SQLSrvProfiles as $P ): $PID = $P['ProfileID']; ?>
        <dt>
            <a href="<?=$lp('Examples',"?sqlsrvedit={$PID}#fileform")?>"><?=$this($P['FirstName'])?> <?=$this($P['LastName'])?></a>
            <a class="iconlink" href="<?=$lp('AjaxFrags',">examples-aside?sqlsrvdelete={$PID}")?>"><i class="iconic-x"></i></a>
            <a class="iconlink" href="<?=$lp('AjaxFrags',">examples-aside?sqlsrvcopy={$PID}")?>"><i class="iconic-new-window"></i></a>
        </dt>
        <dt><small class="muted"><?=$P['InsertDT']->format('Y-m-d H:m:s')?></small></dt>
       <?php endforeach; ?>
    </dl>
   <?php endif; ?>
</div>

