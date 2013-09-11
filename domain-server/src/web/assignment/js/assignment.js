$(document).ready(function(){
  
  // setup the editor
  var editor = ace.edit("editor");
  editor.setTheme("ace/theme/twilight")
  editor.getSession().setMode("ace/mode/javascript");
  editor.getSession().setTabSize(2);
  
  // setup the Ink filepicker
  filepicker.setKey("ARhz9KegMS0ioo5o9bPOcz");
  
  $('#deploy-button').click(function(){
    script = editor.getValue();
    
    // store the script on S3 using filepicker
    filepicker.store(script, {mimetype: 'application/javascript'}, function(blob){
      console.log(JSON.stringify(blob));
      
      s3_filename = blob["key"];
      
      $.post('/assignment', {s3_filename: s3_filename}, function(response){
        // the response is the assignment ID, if successful
        console.log(response);
      }, function(error) {
        console.log(error);
      });
    }, function(FPError){
      console.log(FPError);
    });
  });
});
