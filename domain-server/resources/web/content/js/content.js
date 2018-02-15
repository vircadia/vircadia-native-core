$(document).ready(function(){

  Settings.afterReloadActions = function() {};

  var frm = $('#upload-form');
  frm.submit(function (ev) {
    $.ajax({
      type: frm.attr('method'),
      url: frm.attr('action'),
      data: new FormData($(this)[0]),
      cache: false,
      contentType: false,
      processData: false,
      success: function (data) {
        swal({
          title: 'Uploaded',
          type: 'success',
          text: 'Your Entity Server is restarting to replace its local content with the uploaded file.',
          confirmButtonText: 'OK'
        })
      },
      error: function (data) {
        swal({
          title: '',
          type: 'error',
          text: 'Your entities file could not be transferred to the Entity Server.</br>Verify that the file is a <i>.json</i> or <i>.json.gz</i> entities file and try again.',
          html: true,
          confirmButtonText: 'OK',
        });
      }
    });

    ev.preventDefault();

    showSpinnerAlert("Uploading Entities File");
  });
});
