
<ul class="breadcrumb">
<li><a href="<?=$lp('Home')?>">Home</a> <span class="divider">/</span></li>
<li><?=$page->Domain?></li>
<li>
| <span style="font-size: .89em;"><a href="#" class="set-status" data-type="select" data-url="<?=$lr('site_set_status')?>" data-value="<?=$S['Status']?>" data-name="Status"><?=strtolower($S['Status'])?></a></span>
</li>
</ul>

<div class="pull-right">
<ul class="nav nav-pills">
    <li class="active"><a href="<?=$lp('NewPage')?>">new page</a></li>
    <li class="dropdown">
        <a class="dropdown-toggle" id="drop4" role="button" data-toggle="dropdown" href="#">Pages <b class="caret"></b></a>
        <ul id="menu1" class="dropdown-menu" role="menu" aria-labelledby="drop4">
           <?php foreach( $PS as $P ): ?>
            <li role="presentation"><a role="menuitem" tabindex="-1" href="#"><?=$this($P['Path'])?></a></li>
           <?php endforeach; ?>
        </ul>
    </li>
    <li class="active"><a href="<?=$lp('NewTemplate')?>">new template</a></li>
    <li class="dropdown">
        <a class="dropdown-toggle" id="drop5" role="button" data-toggle="dropdown" href="#">Templates <b class="caret"></b></a>
        <ul id="menu2" class="dropdown-menu" role="menu" aria-labelledby="drop5">
           <?php foreach( $TS as $S ): ?>
            <li role="presentation"><a role="menuitem" tabindex="-1" href="#"><?=$this($T['Name'])?></a></li>
           <?php endforeach; ?>
        </ul>
    </li>
</ul>
</div>

<div class="clearfix"></div>

<div class="row-fluid">
    <div class="span12">
    <h3>Domain:</h3>
        <a href="#" class="set-domain" data-type="text" data-url="<?=$lr('site_set_domain')?>" data-name="Domain"><?=$S['Domain']?></a>
    </div>
</div>


<div class="row-fluid">
    <div class="span12">
    <h3>Base URL:</h3>
        <a href="#" class="set-baseurl" data-type="url" data-url="<?=$lr('site_set_baseurl')?>" data-emptytext="default" data-name="BaseURL"><?=$S['BaseURL']?></a>
    </div>
</div>

<div class="row-fluid">
    <div class="span12"><h3>Routine:</h3>
        <a href="#" class="set-routine" data-type="textarea" data-url="<?=$lr('site_set_routine')?>" data-emptytext="empty routine" data-name="Routine"><?=\fw\Struct::Get(0,$S['Routine'])?></a>
    </div>
</div>

<div class="row-fluid" id="directives">
    <div class="span12"><h3>Directives:</h3>
        <form id="site_set_directive_form" action="<?=$lr('site_set_directive')?>">
        <input type="hidden" name="Site_id" value="<?=\asm\Request::Bottom()?>">
        <table class="table table-bordered table-striped" style="width: 100%;">
        <tbody class="sortable" id="directives-sortable">
           <?php foreach( $DS as $K => $D ): ?>
            <tr id="<?=$K?>">
                <td class="text-center span1">
                    <a href="#" class="pull-left handle" data-pk="<?=$K?>"><i class="icon-th-list"></i></a>
                    <a href="#" class="set-directive-name editable-click" data-pk="<?=$K?>" data-type="select" data-url="<?=$lr('site_set_directive')?>" data-value="<?=$D['Name']?>" data-name="Name"><?=$D['Name']?></a>
                </td>
                <td class="text-center span2">
                    <a href="#" class="set-directive-key editable-click" data-pk="<?=$K?>" data-type="text" data-url="<?=$lr('site_set_directive')?>" data-name="Key"><?=$D['Key']?></a>
                </td>
                <td>
                    <a href="#" class="set-directive-value editable-click" data-pk="<?=$K?>" data-type="text" data-url="<?=$lr('site_set_directive')?>" data-name="Value"><?=$D['Value']?></a>
                    <a href="#" class="pull-right cp-directive" data-pk="<?=$K?>"><i class="icon-tags"></i></a>
                    <a href="#" class="pull-right del-directive" data-pk="<?=$K?>"><i class="icon-remove"></i></a>
                </td>
            </tr>
           <?php endforeach; ?>
        </tbody>

        <tr>
            <td colspan="3">
            <div id="diralert" class="alert alert-error"></div>
            </td>
        </tr>

        <tr class="new-directive">
            <td class="text-center">
            <select class="input-small" name="Name">
                <option></option>
               <?php foreach( $DirectiveNames as $V ): ?>
                <option value="<?=$V?>">$<?=$V?></option>
               <?php endforeach; ?>
            </select>
            </td>
            <td class="text-center">
                <input class="input-small" type="text" placeholder="key" value="" name="Key" >
            </td>
            <td style="vertical-align: middle;">
                <textarea class="" placeholder="value" name="Value" style="width: 75%;"></textarea>
                <button type="submit" class="btn btn-success">New</button>
            </td>
        </tr>
    </table>
    </form>
    </div>
</div>


<?php $this->Stack('JSSite','ajax'); ?>

