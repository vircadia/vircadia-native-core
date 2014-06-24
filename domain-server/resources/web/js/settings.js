$(document).ready(function(){
  $.getJSON('describe.json', function(data){
    var source = $('#template').html();
    var template = Handlebars.compile(source);
    
    $('#settings').html(template(data));
  });
});