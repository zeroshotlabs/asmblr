
<div class="row-fluid">
<nav>
<div class="navbar">
<div class="navbar-inner">
<div class="container">
    <a class="btn btn-navbar" data-toggle="collapse" data-target=".nav-collapse">
        <span class="icon-bar"></span>
        <span class="icon-bar"></span>
        <span class="icon-bar"></span>
    </a>

    <div class="brand">
        <a href="<?=$lp('Home')?>"><img src="<?=$ls('/img/asmblr-logo.png')?>" title="Framewire PHP Framework" /></a>
    </div>

    <div class="nav-collapse collapse">
   <?php if( $page->LoggedIn ): ?>
    <ul class="nav">
        <li class="<?=$lp->Current('Account','active')?>"><a href="<?=$lp('Account')?>">Account</a></li>
        <li><a href="<?=$lp('Logout')?>">Logout</a></li>
    </ul>
   <?php endif; ?>
    </div>
</div>
</div>
</div>
</nav>
</div>



<?php /*
<form class="form-search">
    <div class="input-append">
    <input type="text" class="span6 search-query">
    <button type="submit" class="btn"><i class="icon-search"></i></button>
    </div>
</form>
*/ ?>


