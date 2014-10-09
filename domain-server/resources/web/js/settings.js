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
    
    common_attrs = " class='" + (setting.type !== 'checkbox' ? 'form-control' : '')
      + " trigger-change' data-short-name='" + setting.name + "' name='" + setting_name + "' "
    
    if (setting.type === 'checkbox') {
      form_group += "<label class='" + label_class + "'>" + setting.label + "</label>"
      form_group += "<div class='checkbox" + (isLocked ? " disabled" : "") + "'>"
      form_group += "<label for='" + setting_name + "'>"
      form_group += "<input type='checkbox'" + common_attrs + (setting_value ? "checked" : "") + (isLocked ? " disabled" : "") + "/>"
      form_group += " " + setting.help + "</label>";
      form_group += "</div>"
    } else if (setting.type === 'table') {
      form_group += makeTable(setting, setting_name, setting_value);
    } else {
      input_type = _.has(setting, 'type') ? setting.type : "text"
      
      form_group += "<label for='" + setting_name + "' class='" + label_class + "'>" + setting.label + "</label>";
      
      if (setting.type === 'select') {
        form_group += "<select class='form-control' data-hidden-input='" + setting_name + "'>'"
        
        _.each(setting.options, function(option) {
          form_group += "<option value='" + option.value + "'" + 
          (option.value == setting_value ? 'selected' : '') + ">" + option.label + "</option>"
        })
        
        form_group += "</select>"
        
        form_group += "<input type='hidden'" + common_attrs + "value='" + setting_value + "'>"
      } else {
        
        if (input_type == 'integer') {
          input_type = "text"
        }
        
        form_group += "<input type='" + input_type + "'" +  common_attrs +
          "placeholder='" + (_.has(setting, 'placeholder') ? setting.placeholder : "") + 
          "' value='" + setting_value + "'" + (isLocked ? " disabled" : "") + "/>"
      }
      
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
                  
  $('#settings-form').on('click', '.add-row', function(){
    addTableRow(this);
  })
    
  $('#settings-form').on('click', '.del-row', function(){
    deleteTableRow(this);
  })
    
  $('#settings-form').on('change', 'input.trigger-change', function(){
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
  
  $('#settings-form').on('change', 'select', function(){
    console.log("Changed" + $(this))
    $("input[name='" +  $(this).attr('data-hidden-input') + "']").val($(this).val()).change()
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
  var metaverseInput = $("[name='metaverse.id']");
  var chooseButton = $("<button type='button' id='choose-domain-btn' class='btn btn-primary' style='margin-top:10px'>Choose ID from my domains</button>");
  metaverseInput.after(chooseButton);
}

var SETTINGS_ERROR_MESSAGE = "There was a problem saving domain settings. Please try again!";

$('body').on('click', '.save-button', function(e){
  // disable any inputs not changed
  $("input:not([data-changed])").each(function(){
    $(this).prop('disabled', true);
  });
  
  // grab a JSON representation of the form via form2js
  var formJSON = form2js('settings-form', ".", false, cleanupFormValues, true);
  
  console.log(formJSON);
  
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
      showErrorMessage("Error", SETTINGS_ERROR_MESSAGE)
      reloadSettings();
    }
  }).fail(function(){
    showErrorMessage("Error", SETTINGS_ERROR_MESSAGE)
    reloadSettings();
  });
  
  return false;
});

function makeTable(setting, setting_name, setting_value) {
  var isArray = !_.has(setting, 'key')
  
  var html = "<label class='control-label'>" + setting.label + "</label>"
  html += "<span class='help-block'>" + setting.help + "</span>"
  html += "<table class='table table-bordered' data-short-name='" + setting.name + "' name='" + setting_name + (isArray ? "[]" : "") 
    + "' data-setting-type='" + (isArray ? 'array' : 'hash') + "'>"
    
  // Column names
  html += "<tr class='headers'>"
  
  if (setting.numbered === true) {
    html += "<td class='number'><strong>#</strong></td>" // Row number
  }
  
  if (setting.key) {
    html += "<td class='key'><strong>" + setting.key.label + "</strong></td>" // Key
  }
  
  _.each(setting.columns, function(col) {
    html += "<td class='data'><strong>" + col.label + "</strong></td>" // Data
  })
  
  html += "<td class='buttons'><strong>+/-</strong></td></tr>"
    
  // populate rows in the table from existing values
  var row_num = 1
  
  _.each(setting_value, function(row, indexOrName) {
    html += "<tr class='row-data'" + (isArray ? "" : "name='" + setting_name + "." + indexOrName + "'") + ">"
    
    if (setting.numbered === true) {
      html += "<td class='numbered'>" + row_num + "</td>"
    }
    
    if (setting.key) {
        html += "<td class='key'>" + indexOrName + "</td>"
    }
    
    _.each(setting.columns, function(col) {
      html += "<td class='row-data'>"
      
      if (isArray) {
        html += row
        // for arrays we add a hidden input to this td so that values can be posted appropriately
        html += "<input type='hidden' name='" + setting_name + "[]' value='" + row + "'/>"
      } else if (row.hasOwnProperty(col.name)) {
        html += row[col.name] 
      }
      
      html += "</td>"
    })
    
    html += "<td class='buttons'><span class='glyphicon glyphicon-remove del-row'></span></td>"
    html += "</tr>"
    row_num++
  })
    
  // populate inputs in the table for new values
  html += makeTableInputs(setting)
  html += "</table>"
    
  return html;
}

function makeTableInputs(setting) {
  var html = "<tr class='inputs'>"
  
  if (setting.numbered === true) {
    html += "<td class='numbered'></td>"
  }
  
  if (setting.key) {
    html += "<td class='key' name='" + setting.key.name + "'>\
             <input type='text' class='form-control' placeholder='" + (_.has(setting.key, 'placeholder') ? setting.key.placeholder : "") + "' value=''>\
             </td>"
  }
  
  _.each(setting.columns, function(col) {
    html += "<td class='row-data'name='" + col.name + "'>\
             <input type='text' class='form-control' placeholder='" + (col.key ? col.key : "") + "' value=''>\
             </td>"
  })
  
  html += "<td class='buttons'><span class='glyphicon glyphicon-plus add-row'></span></td>"
  html += "</tr>"
    
  return html
}

function badgeSidebarForDifferences(changedElement) {
  // figure out which group this input is in
  var panelParentID = changedElement.closest('.panel').attr('id')
  
  // get a JSON representation of that section
  var panelJSON = form2js(panelParentID, ".", false, cleanupFormValues, true)[panelParentID]
  var initialPanelJSON = Settings.initialValues[panelParentID]
  
  var badgeValue = 0
  
  // badge for any settings we have that are not the same or are not present in initialValues
  for (var setting in panelJSON) {
    if (!_.isEqual(panelJSON[setting], initialPanelJSON[setting]) 
        && (panelJSON[setting] !== "" || _.has(initialPanelJSON, setting))) {
      badgeValue += 1
    }
  }
  
  // update the list-group-item badge to have the new value
  if (badgeValue == 0) {
    badgeValue = ""
  }
  
  $("a[href='#" + panelParentID + "'] .badge").html(badgeValue);
}

function addTableRow(add_glyphicon) {
  var row = $(add_glyphicon).closest('tr')
  
  var table = row.parents('table')
  var isArray = table.data('setting-type') === 'array'
  
  var data = row.parent().children(".row-data")
  
  if (!isArray) {
    // Check key spaces
    var name = row.children(".key").children("input").val()
    if (name.indexOf(' ') !== -1) {
      showErrorMessage("Error", "Key contains spaces")
      return
    }
    // Check keys with the same name
    var equals = false;
    _.each(data.children(".key"), function(element) {
      if ($(element).text() === name) {
        equals = true
        return
      }
    })
    if (equals) {
      showErrorMessage("Error", "Two keys cannot be identical")
      return
    }
  }
      
  // Check empty fields
  var empty = false;
  _.each(row.children(".row-data").children("input"), function(element) {
    if ($(element).val().length === 0) {
      empty = true
      return
    }
  })
  if (empty) {
    showErrorMessage("Error", "Empty field(s)")
    return
  }
      
  var input_clone = row.clone()
  
  // Change input row to data row
  var table = row.parents("table")
  var setting_name = table.attr("name") 
  var full_name = setting_name + "." + name
  row.addClass("row-data new-row")
  row.removeClass("inputs")
      
  _.each(row.children(), function(element) {
    if ($(element).hasClass("numbered")) { 
      // Index row
      var numbers = data.children(".numbered")
      if (numbers.length > 0) {
        $(element).html(parseInt(numbers.last().text()) + 1)
      } else {
        $(element).html(1)
      }
    } else if ($(element).hasClass("buttons")) { 
      // Change buttons
      var span = $(element).children("span")
      span.removeClass("glyphicon-ok add-row")
      span.addClass("glyphicon-remove del-row")
    } else if ($(element).hasClass("key")) {
      var input = $(element).children("input")
      $(element).html(input.val())
      input.remove()
    } else if ($(element).hasClass("row-data")) { 
      // Hide inputs
      var input = $(element).children("input")
      input.attr("type", "hidden")
      
      if (isArray) {
        input.attr("name", setting_name)
      } else {
        input.attr("name", full_name + "." + $(element).attr("name"))
      }
      
      input.attr("data-changed", "true")
             
      $(element).append(input.val())
    } else {
      console.log("Unknown table element")
    }
  })
  
  input_clone.find('input').each(function(){
    $(this).val('')
  });
  
  if (isArray) {
    updateDataChangedForSiblingRows(row, true)
  }
  
  badgeSidebarForDifferences($(table))
  
  row.parent().append(input_clone)
}

function deleteTableRow(delete_glyphicon) {
  var row = $(delete_glyphicon).closest('tr')
  
  var table = $(row).closest('table')
  var isArray = table.data('setting-type') === 'array'
  
  if (!isArray) {
    // this is a hash row, so we empty it but leave the hidden input blank so it is cleared when we save
    row.empty()
    row.html("<input type='hidden' class='form-control' name='" + table.attr("name") + "' data-changed='true' value=''>");
  } else if (table.find('tr.row-data').length > 1) {
    updateDataChangedForSiblingRows(row)
    
    // this isn't the last row - we can just remove it
    row.remove()
  } else {
    // this is the last row, we can't remove it completely since we need to post an empty array
    row.empty()
    
    row.html("<input type='hidden' class='form-control' name='" + table.attr("name").replace('[]', '') 
      + "' data-changed='true' value=''>");
  }
  
  // we need to fire a change event on one of the remaining inputs so that the sidebar badge is updated
  badgeSidebarForDifferences($(table))
} 

function updateDataChangedForSiblingRows(row, forceTrue) {
  // anytime a new row is added to an array we need to set data-changed for all sibling row-data inputs to true
  // unless it matches the inital set of values
  
  if (!forceTrue) {
    // figure out which group this row is in
    var panelParentID = row.closest('.panel').attr('id')
    // get the short name for the setting from the table
    var tableShortName = row.closest('table').data('short-name')
  
    // get a JSON representation of that section
    var panelSettingJSON = form2js(panelParentID, ".", false, cleanupFormValues, true)[panelParentID][tableShortName]
    var initialPanelSettingJSON = Settings.initialValues[panelParentID][tableShortName]
    
    // if they are equal, we don't need data-changed
    isTrue = _.isEqual(panelSettingJSON, initialPanelSettingJSON)
  } else {
    isTrue = true
  }
  
  row.siblings('.row-data').each(function(){
    var hiddenInput = $(this).find('td.row-data input')
    if (isTrue) {
      hiddenInput.attr('data-changed', isTrue)
    } else {
      hiddenInput.removeAttr('data-changed')
    }
    
  })
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
    return { name: node.name, value: node.checked ? true : false };
  } else {
    return false;
  }
}

function showErrorMessage(title, message) {
  swal(title, message)
}

function chooseFromHighFidelityDomains(clickedButton) {
  // setup the modal to help user pick their domain
  if (Settings.initialValues.metaverse.access_token) {
    
    // add a spinner to the choose button
    clickedButton.html("Loading domains...")
    clickedButton.attr('disabled', 'disabled')
    
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
            window.open("https://data.highfidelity.io/user/domains", '_blank');
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
      
      // remove the spinner from the choose button
      clickedButton.html("Choose from my domains")
      clickedButton.removeAttr('disabled')
    })
    
  } else {
    bootbox.alert({
      message: "You must have an access token to query your High Fidelity domains.<br><br>" + 
        "Please follow the instructions on the settings page to add an access token.",
      title: "Access token required"
    })
  }
}