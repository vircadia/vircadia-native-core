var Settings = {};

$(document).ready(function(){
  Handlebars.registerHelper('setGroup', function(value){
    this.group = value;
  });
  
  Handlebars.registerHelper('settingsValue', function(values, key, group){
    return values[group][key];
  });
  
  var source = $('#template').html();
  Settings.template = Handlebars.compile(source);
  
  reloadSettings();
});

function reloadSettings() {
  $.getJSON('/settings.json', function(data){
    $('#settings').html(Settings.template(data));
  });
}

var SETTINGS_ERROR_MESSAGE = "There was a problem saving domain settings. Please try again!";

$('#settings').on('click', 'button', function(e){  
  // grab a JSON representation of the form via form2js
  var formJSON = form2js('settings-form', ".", false, null, true);
  
  // POST the form JSON to the domain-server settings.json endpoint so the settings are saved
  $.ajax('/settings.json', {
    data: JSON.stringify(formJSON),
    contentType: 'application/json',
    type: 'POST'
  }).done(function(data){
    if (data.status == "success") {
      showAlertMessage("Domain settings saved.", true);
    } else {
      showAlertMessage(SETTINGS_ERROR_MESSAGE, false);
    }
    
    reloadSettings();
  }).fail(function(){
    showAlertMessage(SETTINGS_ERROR_MESSAGE, false);
    reloadSettings();
  });
  
  return false;
});

function showAlertMessage(message, isSuccess) {
  var alertBox = $('.alert');
  alertBox.attr('class', 'alert');
  alertBox.addClass(isSuccess ? 'alert-success' : 'alert-danger');
  alertBox.html(message);
  alertBox.fadeIn();
}