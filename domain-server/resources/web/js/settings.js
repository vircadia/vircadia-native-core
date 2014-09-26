var Settings = {};

var viewHelpers = {
  getFormGroup: function(groupName, setting, values){
    setting_id = groupName + "_" + setting.name
    
    form_group = "<div class='form-group'>"
    
    if (_.has(values, groupName) && _.has(values[groupName], setting.name)) {
      setting_value = values[groupName][setting.name] 
    } else if (_.has(setting, 'default')) {
      setting_value = setting.default
    } else {
      setting_value = ""
    }
    
    if (setting.type === 'checkbox') {
      form_group += "<label class='control-label'>" + setting.label + "</label>"
      form_group += "<div class='checkbox'>"
      form_group += "<label for='" + setting_id + "'>"
      form_group += "<input type='checkbox' id='" + setting_id + "' " + (setting_value ? "checked" : "") + "/>"
      form_group += " " + setting.help + "</label>";
      form_group += "</div>"
    } else {
      input_type = _.has(setting, 'type') ? setting.type : "text"
      
      form_group += "<label for='" + setting_id + "' class='control-label'>" + setting.label + "</label>";
      form_group += "<input type='" + input_type + "' class='form-control' id='" + setting_id + 
        "' placeholder='" + (_.has(setting, 'placeholder') ? setting.placeholder : "") + 
        "' value='" + setting_value + "'/>"
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
  
  $('#settings-form').on('change', 'input', function(){
    // this input was changed, add the changed data attribute to it
    $(this).attr('data-changed', true)
    
    badgeSidebarForDifferences($(this))
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
    
    Settings.initialValues = form2js('settings-form', "_", false, cleanupFormValues, true);
  });
}

var SETTINGS_ERROR_MESSAGE = "There was a problem saving domain settings. Please try again!";

$('#settings').on('click', 'button', function(e){
  // disable any inputs not changed
  $("input:not([data-changed])").each(function(){
    $(this).prop('disabled', true);
  });
  
  // grab a JSON representation of the form via form2js
  var formJSON = form2js('settings-form', "_", false, cleanupFormValues, true);
  
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

function badgeSidebarForDifferences(changedInput) {
  // figure out which group this input is in
  var panelParentID = changedInput.closest('.panel').attr('id')
  
  // get a JSON representation of that section
  var rootJSON = form2js(panelParentID, "_", false, cleanupFormValues, true);
  var panelJSON = rootJSON[panelParentID]
  
  var badgeValue = 0
  
  for (var setting in panelJSON) {
    if (panelJSON[setting] != Settings.initialValues[panelParentID][ setting]) {
      badgeValue += 1
    }
  }
  
  // update the list-group-item badge to have the new value
  if (badgeValue == 0) {
    badgeValue = ""
  }
  
  $("a[href='#" + panelParentID + "'] .badge").html(badgeValue);
}

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