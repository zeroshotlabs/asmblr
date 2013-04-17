
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
        <a href="<?=$lp('Home')?>"><img src="<?=$ls('/img/framewire-logo.gif')?>" title="Framewire PHP Framework" /></a>
    </div>

    <div class="nav-collapse collapse">
    <ul class="nav">
<?php
/*
LinkPage::Current() is used to determine whether a particular Page is
being executed for the current request.

For each menu item, we use it to conditionally set a CSS class to
highlight the item if it's the current page.

$lp is also of course used to generate the URL for each menu item's href.
*/?>
        <li class="<?=$lp->Current('Home','active')?>"><a href="<?=$lp('Home')?>">Home</a></li>
        <li class="<?=$lp->Current('Examples','active')?>"><a href="<?=$lp('Examples')?>">Examples</a></li>
        <li><a title="Framewire PHP Framework Documentation" href="http://www.framewire.org/docs">Docs</a></li>
        <li><a title="About PHP Framework Development" href="http://www.framewire.org/about">About</a></li>
        <li><a href="http://groups.google.com/group/framewire">Discussion</a></li>
    </ul>
    </div>
</div>
</div>
</div>
</nav>
</div>
