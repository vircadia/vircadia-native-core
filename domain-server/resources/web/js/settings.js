var Settings = {};

var viewHelpers = {
  getFormGroup: function(groupName, setting, values){
    setting_id = groupName + "." + setting.name
    
    var form_group = "<div class='form-group'>"
    
    
    if (setting.type === 'checkbox') {
      checked_box = _.has(values, groupName) ? values[groupName][setting.name] : setting.default
      
      form_group += "<label class='control-label'>" + setting.label + "</label>"
      form_group += "<div class='checkbox'>"
      form_group += "<label for='" + setting_id + "'>"
      form_group += "<input type='checkbox' id='" + setting_id + "' " + (checked_box ? "checked" : "") + "/>"
      form_group += " " + setting.help + "</label>";
      form_group += "</div>"
    } else {
      form_group += "<label for='" + setting_id + "' class='control-label'>" + setting.label + "</label>";
      form_group += "<input type='text' class='form-control' id='" + setting_id + 
        "' placeholder='" + setting.placeholder + "' value='" + (values[groupName] || {})[setting.name] + "'/>"
      form_group += "<span class='help-block'>" + setting.help + "</span>" 
    }
    
    
    form_group += "</div>"
    return form_group
  }
}

$(document).ready(function(){  
  /*
  * Clamped-width. 
  * Usage:
  *  <div data-clampedwidth=".myParent">This long content will force clamped width</div>
  *
  * Author: LV
  */
  
  $('[data-clampedwidth]').each(function () {
      var elem = $(this);
      var parentPanel = elem.data('clampedwidth');
      var resizeFn = function () {
          var sideBarNavWidth = $(parentPanel).width() - parseInt(elem.css('paddingLeft')) - parseInt(elem.css('paddingRight')) - parseInt(elem.css('marginLeft')) - parseInt(elem.css('marginRight')) - parseInt(elem.css('borderLeftWidth')) - parseInt(elem.css('borderRightWidth'));
          elem.css('width', sideBarNavWidth);
      };

      resizeFn();
      $(window).resize(resizeFn);
  });
  
  
  var panelsSource = $('#panels-template').html()
  Settings.panelsTemplate = _.template(panelsSource)
  
  var sidebarTemplate = $('#list-group-template').html()
  Settings.sidebarTemplate = _.template(sidebarTemplate)
  
  $('body').scrollspy({ target: '#setup-sidebar', offset: 60 })
  
  reloadSettings()
});

function reloadSettings() {
  $.getJSON('/settings.json', function(data){
    _.extend(data, viewHelpers)
    
    $('.nav-stacked').html(Settings.sidebarTemplate(data))
    $('#panels').html(Settings.panelsTemplate(data))
    
    $('.nav-stacked li').each(function(){
      $(this).removeClass('active')
    });
    
    $('.nav-stacked li:first-child').addClass('active');
    $('body').scrollspy('refresh')
  });
}

var SETTINGS_ERROR_MESSAGE = "There was a problem saving domain settings. Please try again!";

$('#settings').on('click', 'button', function(e){
  // disable any inputs not changed
  $("input:not([data-changed])").each(function(){
    $(this).prop('disabled', true);
  });
  
  // grab a JSON representation of the form via form2js
  var formJSON = form2js('settings-form', ".", false, cleanupFormValues, true);
  
  // re-enable all inputs
  $("input").each(function(){
    $(this).prop('disabled', false);
  });
  
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

$('#settings').on('change', 'input', function(){
  // this input was changed, add the changed data attribute to it
  $(this).attr('data-changed', true);
});

function cleanupFormValues(node) {  
  if (node.type && node.type === 'checkbox') {
    return { name: node.id, value: node.checked ? true : false };
  } else {
    return false;
  }
}

function showAlertMessage(message, isSuccess) {
  var alertBox = $('.alert');
  alertBox.attr('class', 'alert');
  alertBox.addClass(isSuccess ? 'alert-success' : 'alert-danger');
  alertBox.html(message);
  alertBox.fadeIn();
}