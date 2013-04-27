

<div class="row span7">
<ul class="breadcrumb">
   <?php foreach( $SS as $S ): ?>
    <li><a href="<?=$lp('Site',">".(string)$S['_id'])?>"><?=$S['Domain']?></a> <span class="divider">/</span></li>
   <?php endforeach; ?>
</ul>
</div>
