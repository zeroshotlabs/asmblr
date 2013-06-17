
<ul class="nav nav-tabs" id="content_tabs">
    <li><a class="body_tab_link" href="#body_tab" data-toggle="tab" data-taid="content_body">Body</a></li>
<?php /*
    <li><a class="upload_tab_link" href="#upload_tab" data-toggle="tab" data-taid="content_upload">Upload</a></li>
*/ ?>
</ul>

<div class="tab-content">
    <div class="tab-pane" id="body_tab">

       <?php if( strpos($C['Type'],'text/') === 0 || strpos($C['Type'],'php') !== FALSE ): ?>
        <textarea id="content_body" name="Body" data-method="content_set_body" data-mode="<?=$C['Type']?>" data-hasShown=""><?=$this($C['Body'])?></textarea>
       <?php elseif( strpos($C['Type'],'image/') === 0 ): ?>
        <img src="<?=asm('lp')->Link('ContentSrv')?><?=$C['Path']?>" class="replace_dropzone"/>
       <?php else: ?>
        <h2><a target="_blank" href="<?=asm('lp')->Link('ContentSrv')?><?=$C['Path']?>" >download</a></h2>
        <?=$C['Length']?> bytes <?=$C['Type']?>
       <?php endif; ?>

        <div style="margin-top: 3px;">
            <a href="#" id="content_body_save" class="btn">save</a>
            <a href="#" id="content_body_reset" class="btn">cancel</a>

            <form id="content_replace" style="display: inline;" action="<?=$lr('content_set_body')?>" method="POST" enctype="multipart/form-data">
                <span title="replaces body and update type" class="btn btn-warning fileinput-button replace_dropzone" >
                    <i class="icon-upload icon-white"></i>
                    <span>replace</span>
                    <input id="content_replace_file" type="file" name="files[]" multiple>
                </span>
                <span class="help-inline">replace body and update type - drag or click</span>
                <div class="fileupload-progress fade">
                    <div class="progress progress-success progress-striped active" role="progressbar" aria-valuemin="0" aria-valuemax="100">
                        <div class="bar" style="width:0%;"></div>
                    </div>
                    <div class="progress-extended">&nbsp;</div>
                </div>
            </form>
<?php /*
            <a href="#" id="content_body_upload" class="btn btn-primary"><i class="icon-upload icon-white"></i> replace</a>
*/ ?>
        </div>
    </div>
    <div class="tab-pane" id="upload_tab">
        <form id="content_upload" action="<?=$lr('content_create')?>" method="POST" enctype="multipart/form-data">
            <div class="row fileupload-buttonbar">
                <div class="span7">
                    <span class="btn btn-success fileinput-button">
                        <i class="icon-plus icon-white"></i>
                        <span>Add files...</span>
                        <input type="file" name="files[]" multiple>
                    </span>
                    <button type="submit" class="btn btn-primary start">
                        <i class="icon-upload icon-white"></i>
                        <span>Start upload</span>
                    </button>
                    <button type="reset" class="btn btn-warning cancel">
                        <i class="icon-ban-circle icon-white"></i>
                        <span>Cancel upload</span>
                    </button>
                    <button type="button" class="btn btn-danger delete">
                        <i class="icon-trash icon-white"></i>
                        <span>Delete</span>
                    </button>
                    <input type="checkbox" class="toggle">
                    <span class="fileupload-loading"></span>
                </div>
                <div class="span5 fileupload-progress fade">
                    <div class="progress progress-success progress-striped active" role="progressbar" aria-valuemin="0" aria-valuemax="100">
                        <div class="bar" style="width:0%;"></div>
                    </div>
                    <div class="progress-extended">&nbsp;</div>
                </div>
            </div>
            <table role="presentation" class="table table-striped"><tbody class="files" data-toggle="modal-gallery" data-target="#modal-gallery"></tbody></table>
        </form>
    </div>
</div>

<div id="content_delete" class="modal hide">
    <div class="modal-header">
        <h3>Delete content?</h3>
    </div>
    <div class="modal-body">
        <p>This cannot be undone.</p>
        <button class="btn" data-dismiss="modal" aria-hidden="true">Cancel</button>
        <button id="confirm-del" class="btn btn-danger" data-pk="<?=$C['_id']?>">Delete</button>
    </div>
</div>

<?php $this->ajf_uploader(); ?>


