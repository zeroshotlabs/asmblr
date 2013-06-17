
<form id="content_upload" action="<?=$lr('content_create')?>" method="POST" enctype="multipart/form-data">
<div class="row-fluid center">
    <h1 style="font-size: 40px;">Upload Content</h1>
</div>
<div class="row-fluid center">
<h3 class="prefix_editable"><a href="#" class="set-prefix" data-type="text" data-name="UploadPrefix"></a></h3>
</div>
<div class="row-fluid center">
    <div class="upload_dropzone">
        <h3>drop files to upload</h3>
    </div>

    <table role="presentation" class="table table-striped"><tbody class="files" data-toggle="modal-gallery" data-target="#modal-gallery"></tbody></table>
</div>
<div class="row-fluid">
<div class="fileupload-progress fade">
    <div class="progress progress-success progress-striped active" role="progressbar" aria-valuemin="0" aria-valuemax="100">
        <div class="bar" style="width:0%;"></div>
    </div>
    <div class="progress-extended">&nbsp;</div>
</div>
</div>

<div class="row-fluid center">
    <div>
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
</div>

<?php $this->ajf_uploader(); ?>
</form>


