$(document).ready(function(){
  
  // setup the editor
  var editor = ace.edit("editor");
  editor.setTheme("ace/theme/twilight")
  editor.getSession().setMode("ace/mode/javascript");
  editor.getSession().setTabSize(2);
  
  $('#deploy-button').click(function(){
    script = editor.getValue();
    
    // setup our boundary - this is "highfidelity" in hex
    var boundary = "----68696768666964656c697479";
    var body = '--' + boundary + '\r\n'
      // parameter name is "file" and local filename is "temp.txt"
      + 'Content-Disposition:form-data; name="file"; '
      + 'filename="script.js"\r\n'
      // add the javascript mime-type
      + 'Content-type: application/javascript\r\n\r\n'
      // add the script
      + script + '\r\n'
      + '--' + boundary + '--\r\n';
      
    var headers = {};
    
    if ($('#instance-field input').val()) {
      headers['ASSIGNMENT-INSTANCES'] = $('#instance-field input').val();
    }
    
    if ($('#pool-field input').val()) {
      headers['ASSIGNMENT-POOL'] = $('#pool-field input').val();
    }    
            
    // post form to assignment in order to create an assignment
    $.ajax({
        contentType: "multipart/form-data; boundary=" + boundary,
        data: body,
        type: "POST",
        url: "/assignment",
        headers: headers,
        success: function (data, status) {
          console.log(data);
          console.log(status);
        }
    });
  });
});
