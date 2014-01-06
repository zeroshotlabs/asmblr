<?php
/*
Based on the flags set in Internal::DebugHandler(), generate URLs
and display buttons to manage dynamic application debugging.
*/?>
<div class="btn-toolbar center">
    <div class="btn-group">
       <?php if( $page->psdebug ): ?>
        <a class="btn btn-warning" title="PageSet" href="<?=$lp('Examples','?psdebug=')?>">$ps</a>
       <?php else: ?>
        <a class="btn btn-success" title="PageSet" href="<?=$lp('Examples','?psdebug=1')?>">$ps</a>
       <?php endif; ?>

       <?php if( $page->htmldebug ): ?>
        <a class="btn btn-warning" title="enUSHTMLSet" href="<?=$lp('Examples','?htmldebug=')?>">$html</a>
       <?php else: ?>
        <a class="btn btn-success" title="enUSHTMLSet" href="<?=$lp('Examples','?htmldebug=1')?>">$html</a>
       <?php endif; ?>

      <?php if( $page->MongoOnline ): ?>
       <?php if( $page->mongodbdebug ): ?>
        <a class="btn btn-warning" title="MongoDB" href="<?=$lp('Examples','?mongodbdebug=')?>">$mongodb</a>
       <?php else: ?>
        <a class="btn btn-success" title="MongoDB" href="<?=$lp('Examples','?mongodbdebug=1')?>">$mongodb</a>
       <?php endif; ?>
      <?php endif ?>
    </div>
    <div class="btn-group">
      <?php if( $page->MySQLOnline ): ?>
       <?php if( $page->mysqldebug ): ?>
        <a class="btn btn-warning" title="MySQL" href="<?=$lp('Examples','?mysqldebug=')?>">$mysql</a>
       <?php else: ?>
        <a class="btn btn-success" title="MySQL" href="<?=$lp('Examples','?mysqldebug=1')?>">$mysql</a>
       <?php endif; ?>
       <?php if( $page->mysqlsdebug ): ?>
        <a class="btn btn-warning" title="MySQLSet" href="<?=$lp('Examples','?mysqlsdebug=')?>">$mysqls</a>
       <?php else: ?>
        <a class="btn btn-success" title="MySQLSet" href="<?=$lp('Examples','?mysqlsdebug=1')?>">$mysqls</a>
       <?php endif; ?>
      <?php endif; ?>
    </div>
    <div class="btn-group">
      <?php if( $page->SQLSrvOnline ): ?>
       <?php if( $page->sqlsrvdebug ): ?>
        <a class="btn btn-warning" title="SQLSrv" href="<?=$lp('Examples','?sqlsrvdebug=')?>">$sqlsrv</a>
       <?php else: ?>
        <a class="btn btn-success" title="SQLSrv" href="<?=$lp('Examples','?sqlsrvdebug=1')?>">$sqlsrv</a>
       <?php endif; ?>
       <?php if( $page->sqlsrvsdebug ): ?>
        <a class="btn btn-warning" title="SQLSrvSet" href="<?=$lp('Examples','?sqlsrvsdebug=')?>">$sqlsrvs</a>
       <?php else: ?>
        <a class="btn btn-success" title="SQLSrvSet" href="<?=$lp('Examples','?sqlsrvsdebug=1')?>">$sqlsrvs</a>
       <?php endif; ?>
      <?php endif; ?>
    </div>
</div>

