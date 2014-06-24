var Settings = {};

$(document).ready(function(){
  Handlebars.registerHelper('setKey', function(value){
      this.key = value;
  });
  
  var source = $('#template').html();
  Settings.template = Handlebars.compile(source);
  
  reloadSettings();
});

function reloadSettings() {
  $.getJSON('describe.json', function(data){
    $('#settings').html(Settings.template(data));
  });
}

$('#settings').on('click', 'button', function(e){  
  // grab a JSON representation of the form via form2js
  var formJSON = form2js('settings-form', ".", false, null, true);
  
  // POST the form JSON to the domain-server settings.json endpoint so the settings are saved
  $.ajax('/settings.json', {
    data: JSON.stringify(formJSON),
    contentType: 'application/json',
    type: 'POST'
  }).done(function(){
    
  }).fail(function(){
    var alertBox = $('.alert');
    alertBox.attr('class', 'alert');
    alertBox.addClass('alert-danger');
    alertBox.html("There was a problem saving domain settings. Please try again!");
    alertBox.fadeIn();
    
    reloadSettings();
  });
  
  return false;
});
