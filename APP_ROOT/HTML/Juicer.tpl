
<div class="row-fluid">
<div class="span9">
    <h2>Content URLs...</h2>
    <ul>
       <?php foreach( $page->ContentURLs as $U ): ?>
        <li><?=$U?></li>
       <?php endforeach; ?>
    </ul>
</div>
</div>


<div class="row-fluid">
<div class="span9">
    <h2>Page URLs...</h2>
    <ul>
       <?php foreach( $page->PageURLs as $U ): ?>
        <li><?=$U?></li>
       <?php endforeach; ?>
    </ul>
</div>
</div>


