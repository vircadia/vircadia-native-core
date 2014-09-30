var Settings = {
  showAdvanced: false
};

var viewHelpers = {
  getFormGroup: function(groupName, setting, values, isAdvanced, isLocked) {
    setting_name = groupName + "." + setting.name
    
    form_group = "<div class='form-group" + (isAdvanced ? " advanced-setting" : "") + "'>"
    
    if (_.has(values, groupName) && _.has(values[groupName], setting.name)) {
      setting_value = values[groupName][setting.name] 
    } else if (_.has(setting, 'default')) {
      setting_value = setting.default
    } else {
      setting_value = ""
    }
    
    label_class = 'control-label'
    if (isLocked) {
      label_class += ' locked'
    }
    
    if (setting.type === 'checkbox') {
      form_group += "<label class='" + label_class + "'>" + setting.label + "</label>"
      form_group += "<div class='checkbox" + (isLocked ? " disabled" : "") + "'>"
      form_group += "<label for='" + setting_name + "'>"
      form_group += "<input type='checkbox' name='" + setting_name + "' " + 
        (setting_value ? "checked" : "") + (isLocked ? " disabled" : "") + "/>"
      form_group += " " + setting.help + "</label>";
      form_group += "</div>"
    } else {
      input_type = _.has(setting, 'type') ? setting.type : "text"
      
      form_group += "<label for='" + setting_name + "' class='" + label_class + "'>" + setting.label + "</label>";
      form_group += "<input type='" + input_type + "' class='form-control' name='" + setting_name + 
        "' placeholder='" + (_.has(setting, 'placeholder') ? setting.placeholder : "") + 
        "' value='" + setting_value + "'" + (isLocked ? " disabled" : "") + "/>"
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
  })
  
  $('#settings-form').on('change', 'input', function(){
    // this input was changed, add the changed data attribute to it
    $(this).attr('data-changed', true)
    
    badgeSidebarForDifferences($(this))
  })
  
  $('#advanced-toggle-button').click(function(){
    Settings.showAdvanced = !Settings.showAdvanced
    var advancedSelector = $('.advanced-setting') 
    
    if (Settings.showAdvanced) {
      advancedSelector.show()
      $(this).html("Hide advanced")
    } else {
      advancedSelector.hide()
      $(this).html("Show advanced")
    }
    
    $(this).blur()
  })
  
  $('#settings-form').on('click', '#choose-domain-btn', function(){
    chooseFromHighFidelityDomains($(this))
  })

  var panelsSource = $('#panels-template').html()
  Settings.panelsTemplate = _.template(panelsSource)
  
  var sidebarTemplate = $('#list-group-template').html()
  Settings.sidebarTemplate = _.template(sidebarTemplate)
  
  // $('body').scrollspy({ target: '#setup-sidebar'})
  
  reloadSettings()
})

function reloadSettings() {
  $.getJSON('/settings.json', function(data){
    _.extend(data, viewHelpers)
    
    $('.nav-stacked').html(Settings.sidebarTemplate(data))
    $('#panels').html(Settings.panelsTemplate(data))
    
    Settings.initialValues = form2js('settings-form', ".", false, cleanupFormValues, true);
    
    // add tooltip to locked settings
    $('label.locked').tooltip({
      placement: 'right',
      title: 'This setting is in the master config file and cannot be changed'
    })
    
    appendDomainSelectionModal()
  });
}

function appendDomainSelectionModal() {
  var metaverseFormGroup = $("[name='metaverse.id']").parent('.form-group');
  var chooseButton = "<button type='button' id='choose-domain-btn' class='btn btn-primary'>Choose from my domains</button>";
  metaverseFormGroup.append(chooseButton);
}

var SETTINGS_ERROR_MESSAGE = "There was a problem saving domain settings. Please try again!";

$('body').on('click', '.save-button', function(e){
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
  
  // remove focus from the button
  $(this).blur()
  
  // POST the form JSON to the domain-server settings.json endpoint so the settings are saved
  $.ajax('/settings.json', {
    data: JSON.stringify(formJSON),
    contentType: 'application/json',
    type: 'POST'
  }).done(function(data){
    if (data.status == "success") {
      showRestartModal();
    } else {
      showAlertMessage(SETTINGS_ERROR_MESSAGE, false);
      reloadSettings();
    }
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
  var rootJSON = form2js(panelParentID, ".", false, cleanupFormValues, true);
  var panelJSON = rootJSON[panelParentID]
  
  var badgeValue = 0
  
  for (var setting in panelJSON) {
    if (panelJSON[setting] != Settings.initialValues[panelParentID][setting]) {
      badgeValue += 1
    }
  }
  
  // update the list-group-item badge to have the new value
  if (badgeValue == 0) {
    badgeValue = ""
  }
  
  $("a[href='#" + panelParentID + "'] .badge").html(badgeValue);
}

function showRestartModal() {
  $('#restart-modal').modal({
    backdrop: 'static',
    keyboard: false
  });
  
  var secondsElapsed = 0;
  var numberOfSecondsToWait = 3;
  
  var refreshSpan = $('span#refresh-time')
  refreshSpan.html(numberOfSecondsToWait +  " seconds");
  
  // call ourselves every 1 second to countdown
  var refreshCountdown = setInterval(function(){
    secondsElapsed++;
    secondsLeft = numberOfSecondsToWait - secondsElapsed
    refreshSpan.html(secondsLeft + (secondsLeft == 1 ? " second" : " seconds"))
    
    if (secondsElapsed == numberOfSecondsToWait) {
      location.reload(true);
      clearInterval(refreshCountdown);
    }
  }, 1000);
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

function chooseFromHighFidelityDomains(clickedButton) {
  // setup the modal to help user pick their domain
  if (Settings.initialValues.metaverse.access_token) {
    
    // add a spinner to the choose button
    
    // get a list of user domains from data-web
    data_web_domains_url = "https://data.highfidelity.io/api/v1/domains?access_token="
    $.getJSON(data_web_domains_url + Settings.initialValues.metaverse.access_token, function(data){
      
      modal_buttons = {
        cancel: {
          label: 'Cancel',
          className: 'btn-default'
        }
      }
      
      if (data.data.domains.length) {
        // setup a select box for the returned domains
        modal_body = "<p>Choose the High Fidelity domain you want this domain-server to represent.<br/>This will set your domain ID on the settings page.</p>"
        domain_select = $("<select id='domain-name-select' class='form-control'></select>")
        _.each(data.data.domains, function(domain){
          domain_select.append("<option value='" + domain.id + "'>" + domain.name + "</option>")
        })
        modal_body += "<label for='domain-name-select'>Domains</label>" + domain_select[0].outerHTML
        modal_buttons["success"] = {
          label: 'Choose domain',
          callback: function() {
            domainID = $('#domain-name-select').val()
            // set the domain ID on the form
            $("[name='metaverse.id']").val(domainID).change();
          }
        }
      } else {
        modal_buttons["success"] = {
          label: 'Create new domain',
          callback: function() {
            window.open("https://data.highfidelity.io/domains", '_blank');
          }
        }
        modal_body = "<p>You do not have any domains in your High Fidelity account." + 
          "<br/><br/>Go to your domains page to create a new one. Once your domain is created re-open this dialog to select it.</p>"
      }
      
      
      bootbox.dialog({
        title: "Choose matching domain",
        message: modal_body,
        buttons: modal_buttons
      })
    })
    
  } else {
    bootbox.alert({
      message: "You must have an access token to query your High Fidelity domains.<br><br>" + 
        "Please follow the instructions on the settings page to add an access token.",
      title: "Access token required"
    })
  }
}