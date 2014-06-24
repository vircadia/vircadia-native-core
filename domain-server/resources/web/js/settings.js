$(document).ready(function(){
  $.getJSON('describe.json', function(data){
    
    Handlebars.registerHelper('setKey', function(value){
        this.key = value;
    });
    
    var source = $('#template').html();
    var template = Handlebars.compile(source);
    
    $('#settings').html(template(data));
  });
});