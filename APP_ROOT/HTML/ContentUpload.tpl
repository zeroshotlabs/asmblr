
<form id="content_upload" action="<?=$la('content_create')?>" method="POST" enctype="multipart/form-data">
<div class="row-fluid text-center">
    <h1>upload content</h1>
    <h3 class="prefix_editable"><a href="#" class="set-prefix" data-type="text" data-name="UploadPrefix"></a></h3>
</div>

<div class="row-fluid">
    <div style="min-height: 40px;" class="span6 offset3">
    <div class="alert" style="display: none; margin: 0;">
        <button type="button" class="close" data-dismiss="alert">&times;</button>
        <strong class="alert-header"></strong>
        <span class="alert-body"></span>
    </div>
    </div>
</div>
<div class="row-fluid text-center">
    <div class="upload_dropzone">
        <h3>drop files here to upload</h3>
    </div>
</div>
<div class="row-fluid text-center">
    <span class="btn btn-success fileinput-button">
        <i class="icon-plus icon-white"></i>
        <span>or click to browse...</span>
        <input type="file" name="files[]" multiple>
    </span>
    <span class="fileupload-loading"></span>
</div>
<div class="row-fluid text-center">
<div class="fileupload-progress fade">
    <div class="progress progress-success progress-striped active" role="progressbar" aria-valuemin="0" aria-valuemax="100">
        <div class="bar" style="width:0%;"></div>
    </div>
    <div class="progress-extended">&nbsp;</div>
</div>
</div>
</form>


<div class="row-fluid">
    <div id="grid_container"><?php $this->ajf_content_grid(); ?></div>
</div>

