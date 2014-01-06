<?php

// Use the flags set in Request.inc::Examples() to determine which
// buttons should be enabled, and their respective labels, based
// on application/request state.

if( !$page->MongoOnline || (!$page->MongoUpdate && $page->Update) )
    $MongoDisable = TRUE;
else
    $MongoDisable = FALSE;
$MongoLabel = $page->MongoUpdate?'Update':'Save';

if( !$page->MySQLOnline || (!$page->MySQLUpdate && $page->Update) )
    $MySQLDisable = TRUE;
else
    $MySQLDisable = FALSE;
$MySQLLabel = $page->MySQLUpdate?'Update':'Save';

if( !$page->SQLSrvOnline || (!$page->SQLSrvUpdate && $page->Update) )
    $SQLSrvDisable = TRUE;
else
    $SQLSrvDisable = FALSE;
$SQLSrvLabel = $page->SQLSrvUpdate?'Update':'Save';
?>

<div class="span10">
    <button type="submit" <?=$MongoDisable?'disabled="disabled"':''?> value="SaveToMongo" name="Submit" class="btn btn-primary"><?=$MongoLabel?> to MongoDB</button>
    <button type="submit" <?=$MySQLDisable?'disabled="disabled"':''?> value="SaveToMySQL" name="Submit" class="btn btn-primary"><?=$MySQLLabel?> to MySQL</button>
    <button type="submit" <?=$SQLSrvDisable?'disabled="disabled"':''?> value="SaveToSQLSrv" name="Submit" class="btn btn-primary"><?=$SQLSrvLabel?> to SQL Server</button>
</div>

