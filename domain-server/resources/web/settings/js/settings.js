var DomainInfo = null;

var viewHelpers = {
  getFormGroup: function(keypath, setting, values, isAdvanced) {
    form_group = "<div class='form-group " +
        (isAdvanced ? Settings.ADVANCED_CLASS : "") + " " +
        (setting.deprecated ? Settings.DEPRECATED_CLASS : "" ) + "' " +
        "data-keypath='" + keypath + "'>";
    setting_value = _(values).valueForKeyPath(keypath);

    if (_.isUndefined(setting_value) || _.isNull(setting_value)) {
      if (_.has(setting, 'default')) {
        setting_value = setting.default;
      } else {
        setting_value = "";
      }
    }

    label_class = 'control-label';

    function common_attrs(extra_classes) {
      extra_classes = (!_.isUndefined(extra_classes) ? extra_classes : "");
      return " class='" + (setting.type !== 'checkbox' ? 'form-control' : '')
        + " " + Settings.TRIGGER_CHANGE_CLASS + " " + extra_classes + "' data-short-name='"
        + setting.name + "' name='" + keypath + "' "
        + "id='" + (!_.isUndefined(setting.html_id) ? setting.html_id : keypath) + "'";
    }

    if (setting.type === 'checkbox') {
      if (setting.label) {
        form_group += "<label class='" + label_class + "'>" + setting.label + "</label>"
      }

      form_group += "<div class='toggle-checkbox-container'>"
      form_group += "<input type='checkbox'" + common_attrs('toggle-checkbox') + (setting_value ? "checked" : "") + "/>"

      if (setting.help) {
        form_group += "<span class='help-block checkbox-help'>" + setting.help + "</span>";
      }

      form_group += "</div>"
    } else {
      input_type = _.has(setting, 'type') ? setting.type : "text"

      if (setting.label) {
        form_group += "<label for='" + keypath + "' class='" + label_class + "'>" + setting.label + "</label>";
      }

      if (input_type === 'table') {
        form_group += makeTable(setting, keypath, setting_value)
      } else {
        if (input_type === 'select') {
          form_group += "<select class='form-control' data-hidden-input='" + keypath + "'>'"
          _.each(setting.options, function(option) {
            form_group += "<option value='" + option.value + "'" +
            (option.value == setting_value ? 'selected' : '') + ">" + option.label + "</option>"
          });

          form_group += "</select>"

          form_group += "<input type='hidden'" + common_attrs() + "value='" + setting_value + "'>"
        } else if (input_type === 'button') {
          // Is this a button that should link to something directly?
          // If so, we use an anchor tag instead of a button tag

          if (setting.href) {
            form_group += "<a href='" + setting.href + "'style='display: block;' role='button'"
              + common_attrs("btn " + setting.classes) + " target='_blank'>"
              + setting.button_label + "</a>";
          } else {
            form_group += "<button " + common_attrs("btn " + setting.classes) + ">"
              + setting.button_label + "</button>";
          }

        } else {

          if (input_type == 'integer') {
            input_type = "text"
          }

          form_group += "<input type='" + input_type + "'" +  common_attrs() +
            "placeholder='" + (_.has(setting, 'placeholder') ? setting.placeholder : "") +
            "' value='" + (_.has(setting, 'password_placeholder') ? setting.password_placeholder : setting_value) + "'/>"
        }

        form_group += "<span class='help-block'>" + setting.help + "</span>"
      }
    }

    form_group += "</div>"
    return form_group
  }
}

var qs = (function(a) {
    if (a == "") return {};
    var b = {};
    for (var i = 0; i < a.length; ++i)
    {
      var p=a[i].split('=', 2);
      if (p.length == 1) {
        b[p[0]] = "";
      } else {
        b[p[0]] = decodeURIComponent(p[1].replace(/\+/g, " "));
      }
    }
    return b;
})(window.location.search.substr(1).split('&'));

$(document).ready(function(){
  /*
  * Clamped-width.
  * Usage:
  *  <div data-clampedwidth=".myParent">This long content will force clamped width</div>
  *
  * Author: LV
  */

  $.ajaxSetup({
    timeout: 20000,
  });

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

  $('#' + Settings.FORM_ID).on('click', '.' + Settings.ADD_ROW_BUTTON_CLASS, function(){
    addTableRow($(this).closest('tr'));
  });

  $('#' + Settings.FORM_ID).on('click', '.' + Settings.DEL_ROW_BUTTON_CLASS, function(){
    deleteTableRow($(this).closest('tr'));
  });

  $('#' + Settings.FORM_ID).on('click', '.' + Settings.ADD_CATEGORY_BUTTON_CLASS, function(){
    addTableCategory($(this).closest('tr'));
  });

  $('#' + Settings.FORM_ID).on('click', '.' + Settings.DEL_CATEGORY_BUTTON_CLASS, function(){
    deleteTableCategory($(this).closest('tr'));
  });

  $('#' + Settings.FORM_ID).on('click', '.' + Settings.TOGGLE_CATEGORY_COLUMN_CLASS, function(){
    toggleTableCategory($(this).closest('tr'));
  });

  $('#' + Settings.FORM_ID).on('click', '.' + Settings.MOVE_UP_BUTTON_CLASS, function(){
    moveTableRow($(this).closest('tr'), true);
  });

  $('#' + Settings.FORM_ID).on('click', '.' + Settings.MOVE_DOWN_BUTTON_CLASS, function(){
    moveTableRow($(this).closest('tr'), false);
  });

  $('#' + Settings.FORM_ID).on('keyup', function(e){
    var $target = $(e.target);
    if (e.keyCode == 13) {
      if ($target.is('table input')) {
        // capture enter in table input
        // if we have a sibling next to us that has an input, jump to it, otherwise check if we have a glyphicon for add to click
        sibling = $target.parent('td').next();

        if (sibling.hasClass(Settings.DATA_COL_CLASS)) {
          // set focus to next input
          sibling.find('input').focus();
        } else {

          // jump over the re-order row, if that's what we're on
          if (sibling.hasClass(Settings.REORDER_BUTTONS_CLASS)) {
            sibling = sibling.next();
          }

          // for tables with categories we add the entry and setup the new row on enter
          if (sibling.find("." + Settings.ADD_CATEGORY_BUTTON_CLASS).length) {
            sibling.find("." + Settings.ADD_CATEGORY_BUTTON_CLASS).click();

            // set focus to the first input in the new row
            $target.closest('table').find('tr.inputs input:first').focus();
          }

          var tableRows = sibling.parent();
          var tableBody = tableRows.parent();

          // if theres no more siblings, we should jump to a new row
          if (sibling.next().length == 0 && tableRows.nextAll().length == 1) {
              tableBody.find("." + Settings.ADD_ROW_BUTTON_CLASS).click();
          }
        }

      } else if ($target.is('input')) {
        $target.change().blur();
      }

      e.preventDefault();
    }
  });

  $('#' + Settings.FORM_ID).on('keypress', function(e){
    if (e.keyCode == 13) {

      e.preventDefault();
    }
  });

  $('#' + Settings.FORM_ID).on('change', '.' + Settings.TRIGGER_CHANGE_CLASS , function(){
    // this input was changed, add the changed data attribute to it
    $(this).attr('data-changed', true);

    badgeSidebarForDifferences($(this));
  });

  $('#' + Settings.FORM_ID).on('switchChange.bootstrapSwitch', 'input.toggle-checkbox', function(){
    // this checkbox was changed, add the changed data attribute to it
    $(this).attr('data-changed', true);

    badgeSidebarForDifferences($(this));
  });

  // Bootstrap switch in table
  $('#' + Settings.FORM_ID).on('change', 'input.table-checkbox', function () {
    // Bootstrap switches in table: set the changed data attribute for all rows in table.
    var row = $(this).closest('tr');
    if (row.hasClass("value-row")) {  // Don't set attribute on input row switches prior to it being added to table.
      row.find('td.' + Settings.DATA_COL_CLASS + ' input').attr('data-changed', true);
      updateDataChangedForSiblingRows(row, true);
      badgeSidebarForDifferences($(this));
    }
  });

  $('#' + Settings.FORM_ID).on('change', 'input.table-time', function() {
    // Bootstrap switches in table: set the changed data attribute for all rows in table.
    var row = $(this).closest('tr');
    if (row.hasClass("value-row")) {  // Don't set attribute on input row switches prior to it being added to table.
      row.find('td.' + Settings.DATA_COL_CLASS + ' input').attr('data-changed', true);
      updateDataChangedForSiblingRows(row, true);
      badgeSidebarForDifferences($(this));
    }
  });

  $('.advanced-toggle').click(function(){
    Settings.showAdvanced = !Settings.showAdvanced
    var advancedSelector = $('.' + Settings.ADVANCED_CLASS)

    if (Settings.showAdvanced) {
      advancedSelector.show();
      $(this).html("Hide advanced")
    } else {
      advancedSelector.hide();
      $(this).html("Show advanced")
    }

    $(this).blur();
  })

  $('#' + Settings.FORM_ID).on('click', '#' + Settings.CREATE_DOMAIN_ID_BTN_ID, function(){
    $(this).blur();
    showDomainCreationAlert(false);
  })

  $('#' + Settings.FORM_ID).on('click', '#' + Settings.CHOOSE_DOMAIN_ID_BTN_ID, function(){
    $(this).blur();
    chooseFromHighFidelityDomains($(this))
  });

  $('#' + Settings.FORM_ID).on('click', '#' + Settings.GET_TEMPORARY_NAME_BTN_ID, function(){
    $(this).blur();
    createTemporaryDomain();
  });


  $('#' + Settings.FORM_ID).on('change', 'select', function(){
    $("input[name='" +  $(this).attr('data-hidden-input') + "']").val($(this).val()).change()
  });

  $('#' + Settings.FORM_ID).on('click', '#' + Settings.DISCONNECT_ACCOUNT_BTN_ID, function(e){
    $(this).blur();
    disonnectHighFidelityAccount();
    e.preventDefault();
  });

  $('#' + Settings.FORM_ID).on('click', '#' + Settings.CONNECT_ACCOUNT_BTN_ID, function(e){
    $(this).blur();
    prepareAccessTokenPrompt(function(accessToken) {
      // we have an access token - set the access token input with this and save settings
      $(Settings.ACCESS_TOKEN_SELECTOR).val(accessToken).change();
      saveSettings();
    });
  });

  var panelsSource = $('#panels-template').html()
  Settings.panelsTemplate = _.template(panelsSource)

  var sidebarTemplate = $('#list-group-template').html()
  Settings.sidebarTemplate = _.template(sidebarTemplate)

  var navbarHeight = $('.navbar').outerHeight(true);

  $('#setup-sidebar').affix({
    offset: {
      top: 1,
      bottom: navbarHeight
    }
  });

  reloadSettings(function(success){
    if (success) {
      handleAction();
    } else {
      swal({
        title: '',
        type: 'error',
        text: Strings.LOADING_SETTINGS_ERROR
      });
    }
    $('body').scrollspy({
      target: '#setup-sidebar',
      offset: navbarHeight
    });
  });
});

function getShareName(callback) {
  getDomainFromAPI(function(data){
    // check if we have owner_places (for a real domain) or a name (for a temporary domain)
    if (data && data.status == "success") {
      var shareName;
      if (data.domain.default_place_name) {
        shareName = data.domain.default_place_name;
      } else if (data.domain.name) {
        shareName = data.domain.name;
      } else if (data.domain.network_address) {
        shareName = data.domain.network_address;
        if (data.domain.network_port !== 40102) {
          shareName += ':' + data.domain.network_port;
        }
      }

      callback(true, shareName);
    } else {
      callback(false);
    }
  })
}

function handleAction() {
  // check if we were passed an action to handle
  var action = qs["action"];

  if (action == "share") {
    // figure out if we already have a stored domain ID
    if (Settings.data.values.metaverse.id.length > 0) {
      // we need to ask the API what a shareable name for this domain is
      getShareName(function(success, shareName){
        if (success) {
          var shareLink = "hifi://" + shareName;

          console.log(shareLink);

          // show a dialog with a copiable share URL
          swal({
            title: "Share",
            type: "input",
            inputPlaceholder: shareLink,
            inputValue: shareLink,
            text: "Copy this URL to invite friends to your domain.",
            closeOnConfirm: true
          });

          $('.sweet-alert input').select();

        } else {
          // show an error alert
          swal({
            title: '',
            type: 'error',
            text: "There was a problem retreiving domain information from High Fidelity API.",
            confirmButtonText: 'Try again',
            showCancelButton: true,
            closeOnConfirm: false
          }, function(isConfirm){
            if (isConfirm) {
              // they want to try getting domain share info again
              showSpinnerAlert("Requesting domain information...")
              handleAction();
            } else {
              swal.close();
            }
          });
        }
      });
    } else {
      // no domain ID present, just show the share dialog
      createTemporaryDomain();
    }
  }
}

function dynamicButton(button_id, text) {
  return $("<button type='button' id='" + button_id + "' class='btn btn-primary'>" + text + "</button>");
}

function postSettings(jsonSettings) {
  // POST the form JSON to the domain-server settings.json endpoint so the settings are saved
  $.ajax('/settings.json', {
    data: JSON.stringify(jsonSettings),
    contentType: 'application/json',
    type: 'POST'
  }).done(function(data){
    if (data.status == "success") {
      if ($(".save-button").html() === SAVE_BUTTON_LABEL_RESTART) {
        showRestartModal();
      } else {
        location.reload(true);
      }
    } else {
      showErrorMessage("Error", SETTINGS_ERROR_MESSAGE)
      reloadSettings();
    }
  }).fail(function(){
    showErrorMessage("Error", SETTINGS_ERROR_MESSAGE)
    reloadSettings();
  });
}

function accessTokenIsSet() {
  return Settings.data.values.metaverse.access_token.length > 0;
}

function setupHFAccountButton() {

  var hasAccessToken = accessTokenIsSet();
  var el;

  if (hasAccessToken) {
    el = "<p>";
    el += "<span class='account-connected-header'>High Fidelity Account Connected</span>";
    el += "<button id='" + Settings.DISCONNECT_ACCOUNT_BTN_ID + "' class='btn'>Disconnect</button>";
    el += "</p>";
    el = $(el);
  } else {
    // setup an object for the settings we want our button to have
    var buttonSetting = {
      type: 'button',
      name: 'connected_account',
      label: 'Connected Account',
    }
    buttonSetting.help = "";
    buttonSetting.classes = "btn-primary";
    buttonSetting.button_label = "Connect High Fidelity Account";
    buttonSetting.html_id = Settings.CONNECT_ACCOUNT_BTN_ID;

    buttonSetting.href = URLs.METAVERSE_URL + "/user/tokens/new?for_domain_server=true";

    // since we do not have an access token we change hide domain ID and auto networking settings
    // without an access token niether of them can do anything
    $("[data-keypath='metaverse.id']").hide();

    // use the existing getFormGroup helper to ask for a button
    el = viewHelpers.getFormGroup('', buttonSetting, Settings.data.values);
  }

  // add the button group to the top of the metaverse panel
  $('#metaverse .panel-body').prepend(el);
}

function disonnectHighFidelityAccount() {
  // the user clicked on the disconnect account btn - give them a sweet alert to make sure this is what they want to do
  swal({
    title: "Are you sure?",
    text: "This will remove your domain-server OAuth access token."
      + "</br></br>This could cause your domain to appear offline and no longer be reachable via any place names.",
    type: "warning",
    html: true,
    showCancelButton: true
  }, function(){
    // we need to post to settings to clear the access-token
    $(Settings.ACCESS_TOKEN_SELECTOR).val('').change();
    // reset the domain id to get a new temporary name
    $(Settings.DOMAIN_ID_SELECTOR).val('').change();
    saveSettings();
  });
}

function showSpinnerAlert(title) {
  swal({
    title: title,
    text: '<div class="spinner" style="color:black;"><div class="bounce1"></div><div class="bounce2"></div><div class="bounce3"></div></div>',
    html: true,
    showConfirmButton: false,
    allowEscapeKey: false
  });
}

function showDomainCreationAlert(justConnected) {
  swal({
    title: 'Create new domain ID',
    type: 'input',
    text: 'Enter a label this machine.</br></br>This will help you identify which domain ID belongs to which machine.</br></br>',
    showCancelButton: true,
    confirmButtonText: "Create",
    closeOnConfirm: false,
    html: true
  }, function(inputValue){
    if (inputValue === false) {
      swal.close();

      // user cancelled domain ID creation - if we're supposed to save after cancel then save here
      if (justConnected) {
        saveSettings();
      }
    } else {
      // we're going to change the alert to a new one with a spinner while we create this domain
      showSpinnerAlert('Creating domain ID');
      createNewDomainID(inputValue, justConnected);
    }
  });
}

function createNewDomainID(label, justConnected) {
  // get the JSON object ready that we'll use to create a new domain
  var domainJSON = {
    "label": label
  }

  $.post("/api/domains", domainJSON, function(data){
    // we successfully created a domain ID, set it on that field
    var domainID = data.domain.id;
    console.log("Setting domain id to ", data, domainID);
    $(Settings.DOMAIN_ID_SELECTOR).val(domainID).change();

    if (justConnected) {
      var successText = Strings.CREATE_DOMAIN_SUCCESS_JUST_CONNECTED
    } else {
      var successText = Strings.CREATE_DOMAIN_SUCCESS;
    }

    successText += "</br></br>Click the button below to save your new settings and restart your domain-server.";

    // show a sweet alert to say we are all finished up and that we need to save
    swal({
      title: 'Success!',
      type: 'success',
      text: successText,
      html: true,
      confirmButtonText: 'Save'
    }, function(){
      saveSettings();
    });
  }, 'json').fail(function(){

    var errorText = "There was a problem creating your new domain ID. Do you want to try again or";

    if (justConnected) {
      errorText += " just save your new access token?</br></br>You can always create a new domain ID later.";
    } else {
      errorText += " cancel?"
    }

    // we failed to create the new domain ID, show a sweet-alert that lets them try again or cancel
    swal({
      title: '',
      type: 'error',
      text: errorText,
      html: true,
      confirmButtonText: 'Try again',
      showCancelButton: true,
      closeOnConfirm: false
    }, function(isConfirm){
      if (isConfirm) {
        // they want to try creating a domain ID again
        showDomainCreationAlert(justConnected);
      } else {
        // they want to cancel
        if (justConnected) {
          // since they just connected we need to save the access token here
          saveSettings();
        }
      }
    });
  });
}

function createDomainSpinner() {
  var spinner = '<p class="loading-domain-info-spinner text-center" style="display: none">';
  spinner += 'Loading  <span class="glyphicon glyphicon-refresh glyphicon-refresh-animate"></span>';
  spinner += '</p>';
  return spinner;
}

function createDomainLoadingError(message) {
  var errorEl = $("<div class='domain-loading-error alert alert-warning' style='display: none'></div>");
  errorEl.append(message + "  ");

  var retryLink = $("<a href='#'>Please click here to try again.</a>");
  retryLink.click(function(ev) {
    ev.preventDefault();
    reloadDomainInfo();
  });
  errorEl.append(retryLink);

  return errorEl;
}

function parseJSONResponse(xhr) {
  try {
    return JSON.parse(xhr.responseText);
  } catch (e) {
  }
  return null;
}

function showOrHideLabel() {
  var type = getCurrentDomainIDType();
  var shouldShow = accessTokenIsSet() && (type === DOMAIN_ID_TYPE_FULL || type === DOMAIN_ID_TYPE_UNKNOWN);
  $(".panel#label").toggle(shouldShow);
  $("li a[href='#label']").parent().toggle(shouldShow);
  return shouldShow;
}

function setupDomainLabelSetting() {
  showOrHideLabel();

  var html = "<div>"
  html += "<label class='control-label'>Specify a label for your domain</label> <a class='domain-loading-hide' href='#'>Edit</a>";
  html += "<input id='network-label' type='text' class='form-control domain-loading-hide' disabled></input>";
  html += "</div>";

  html = $(html);

  html.find('a').click(function(ev) {
    ev.preventDefault();

    var label = DomainInfo.label === null ? "" : DomainInfo.label;
    var modal_body = "<div class='form'>";
    modal_body += "<label class='control-label'>Label</label>";
    modal_body += "<input type='text' id='domain-label-input' class='form-control' value='" + label + "'>";
    modal_body += "<div id='domain-label-error' class='error-message' data-property='label'></div>";
    modal_body += "</div>";

    var dialog = bootbox.dialog({
      title: 'Edit Label',
      message: modal_body,
      closeButton: false,
      onEscape: true,
      buttons: [
        {
          label: 'Cancel',
          className: 'edit-label-cancel-btn',
          callback: function() {
            dialog.modal('hide');
          }
        },
        {
          label: 'Save',
          className: 'edit-label-save-btn btn btn-primary',
          callback: function() {
            var data = {
              label: $('#domain-label-input').val()
            };

            $('.edit-label-cancel-btn').attr('disabled', 'disabled');
            $('.edit-label-save-btn').attr('disabled', 'disabled');
            $('.edit-label-save-btn').html("Saving...");

            $('.error-message').hide();

            $.ajax({
              url: '/api/domains',
              type: 'PUT',
              data: data,
              success: function(xhr) {
                dialog.modal('hide');
                reloadDomainInfo();
              },
              error: function(xhr) {
                var data = parseJSONResponse(xhr);
                console.log(data, data.status, data.data);
                if (data.status === "fail") {
                  for (var key in data.data) {
                    var errorMsg = data.data[key];
                    var errorEl = $('.error-message[data-property="' + key + '"');
                    errorEl.html(errorMsg);
                    errorEl.show();
                  }
                }
                $('.edit-label-cancel-btn').removeAttr('disabled');
                $('.edit-label-save-btn').removeAttr('disabled');
                $('.edit-label-save-btn').html("Save");
              }
            });
            return false;
          }
        }
      ],
      callback: function(result) {
        console.log("result: ", result);
      }
    });
  });

  var spinner = createDomainSpinner();
  var errorEl = createDomainLoadingError("Error loading label.");

  html.append(spinner);
  html.append(errorEl);

  $('div#label .panel-body').append(html);
}

function showOrHideAutomaticNetworking() {
  var type = getCurrentDomainIDType();
  if (!accessTokenIsSet() || (type !== DOMAIN_ID_TYPE_FULL && type !== DOMAIN_ID_TYPE_UNKNOWN)) {
    $("[data-keypath='metaverse.automatic_networking']").hide();
    return false;
  }
  $("[data-keypath='metaverse.automatic_networking']").show();
  return true;
}

function setupDomainNetworkingSettings() {
  if (!showOrHideAutomaticNetworking()) {
    return;
  }

  var autoNetworkingSetting = Settings.data.values.metaverse.automatic_networking;
  if (autoNetworkingSetting === 'full') {
    return;
  }

  var includeAddress = autoNetworkingSetting === 'disabled';

  if (includeAddress) {
    var label = "Network Address:Port";
  } else {
    var label = "Network Port";
  }

  var lowerName = name.toLowerCase();
  var form = '<div id="network-address-port">';
  form += '<label class="control-label">' + label + '</label>';
  form += ' <a id="edit-network-address-port" class="domain-loading-hide" href="#">Edit</a>';
  form += '<input type="text" class="domain-loading-hide form-control" disabled></input>';
  form += '<div class="domain-loading-hide help-block">This defines how nodes will connect to your domain. You can read more about automatic networking <a href="">here</a>.</div>';
  form += '</div>';

  form = $(form);

  form.find('#edit-network-address-port').click(function(ev) {
    ev.preventDefault();

    var address = DomainInfo.network_address === null ? '' : DomainInfo.network_address;
    var port = DomainInfo.network_port === null ? '' : DomainInfo.network_port;
    var modal_body = "<div class='form-group'>";
    if (includeAddress) {
      modal_body += "<label class='control-label'>Address</label>";
      modal_body += "<input type='text' id='network-address-input' class='form-control' value='" + address + "'>";
      modal_body += "<div id='network-address-error' class='error-message' data-property='network_address'></div>";
    }
    modal_body += "<label class='control-label'>Port</label>";
    modal_body += "<input type='text' id='network-port-input' class='form-control' value='" + port + "'>";
    modal_body += "<div id='network-port-error' class='error-message' data-property='network_port'></div>";
    modal_body += "</div>";

    var dialog = bootbox.dialog({
      title: 'Edit Network',
      message: modal_body,
      closeButton: false,
      onEscape: true,
      buttons: [
        {
          label: 'Cancel',
          className: 'edit-network-cancel-btn',
          callback: function() {
            dialog.modal('hide');
          }
        },
        {
          label: 'Save',
          className: 'edit-network-save-btn btn btn-primary',
          callback: function() {
            var data = {
              network_port: $('#network-port-input').val()
            };
            if (includeAddress) {
              data.network_address = $('#network-address-input').val();
            }

            $('.edit-network-cancel-btn').attr('disabled', 'disabled');
            $('.edit-network-save-btn').attr('disabled', 'disabled');
            $('.edit-network-save-btn').html("Saving...");

            console.log('data', data);

            $('.error-message').hide();

            $.ajax({
              url: '/api/domains',
              type: 'PUT',
              data: data,
              success: function(xhr) {
                console.log(xhr, parseJSONResponse(xhr));
                dialog.modal('hide');
                reloadDomainInfo();
              },
              error:function(xhr) {
                var data = parseJSONResponse(xhr);
                console.log(data, data.status, data.data);
                if (data.status === "fail") {
                  for (var key in data.data) {
                    var errorMsg = data.data[key];
                    console.log(key, errorMsg);
                    var errorEl = $('.error-message[data-property="' + key + '"');
                    console.log(errorEl);
                    errorEl.html(errorMsg);
                    errorEl.show();
                  }
                }
                $('.edit-network-cancel-btn').removeAttr('disabled');
                $('.edit-network-save-btn').removeAttr('disabled');
                $('.edit-network-save-btn').html("Save");
              }
            });
            return false;
          }
        }
      ],
      callback: function(result) {
        console.log("result: ", result);
      }
    });
  });

  var spinner = createDomainSpinner();

  var errorMessage = ''
  if (includeAddress) {
    errorMessage = "We were unable to load the network address and port.";
  } else {
    errorMessage = "We were unable to load the network port."
  }
  var errorEl = createDomainLoadingError(errorMessage);

  var autoNetworkingEl = $('div[data-keypath="metaverse.automatic_networking"]');
  autoNetworkingEl.after(spinner);
  autoNetworkingEl.after(errorEl);
  autoNetworkingEl.after(form);
}


function setupPlacesTable() {
  // create a dummy table using our view helper
  var placesTableSetting = {
    type: 'table',
    name: 'places',
    label: 'Places',
    html_id: Settings.PLACES_TABLE_ID,
    help: "The following places currently point to this domain.</br>To point places to this domain, "
      + " go to the <a href='" + URLs.METAVERSE_URL + "/user/places'>My Places</a> "
      + "page in your High Fidelity Metaverse account.",
    read_only: true,
    can_add_new_rows: false,
    columns: [
      {
        "name": "name",
        "label": "Name"
      },
      {
        "name": "path",
        "label": "Viewpoint or Path"
      },
      {
        "name": "remove",
        "label": "",
        "class": "buttons"
      }
    ]
  }

  // get a table for the places
  var placesTableGroup = viewHelpers.getFormGroup('', placesTableSetting, Settings.data.values);

  // append the places table in the right place
  $('#places_paths .panel-body').prepend(placesTableGroup);
  //$('#' + Settings.PLACES_TABLE_ID).append("<tr><td colspan=3></td></tr>");

  var spinner = createDomainSpinner();
  $('#' + Settings.PLACES_TABLE_ID).after($(spinner));

  var errorEl = createDomainLoadingError("There was an error retreiving your places.");
  $("#" + Settings.PLACES_TABLE_ID).after(errorEl);

  // do we have a domain ID?
  if (Settings.data.values.metaverse.id.length == 0) {
    // we don't have a domain ID - add a button to offer the user a chance to get a temporary one
    var temporaryPlaceButton = dynamicButton(Settings.GET_TEMPORARY_NAME_BTN_ID, 'Get a temporary place name');
    $('#' + Settings.PLACES_TABLE_ID).after(temporaryPlaceButton);
  }
  if (accessTokenIsSet()) {
    appendAddButtonToPlacesTable();
  }
}

function placeTableRow(name, path, isTemporary, placeID) {
  var name_link = "<a href='hifi://" + name + "'>" + (isTemporary ? name + " (temporary)" : name) + "</a>";

  function placeEditClicked() {
    editHighFidelityPlace(placeID, name, path);
  }

  function placeDeleteClicked() {
    var el = $(this);
    var confirmString = Strings.REMOVE_PLACE_TITLE.replace("{{place}}", name);
    var dialog = bootbox.dialog({
      message: confirmString,
      closeButton: false,
      onEscape: true,
      buttons: [
        {
          label: Strings.REMOVE_PLACE_CANCEL_BUTTON,
          className: "delete-place-cancel-btn",
          callback: function() {
            dialog.modal('hide');
          }
        },
        {
          label: Strings.REMOVE_PLACE_DELETE_BUTTON,
          className: "delete-place-confirm-btn btn btn-danger",
          callback: function() {
            $('.delete-place-cancel-btn').attr('disabled', 'disabled');
            $('.delete-place-confirm-btn').attr('disabled', 'disabled');
            $('.delete-place-confirm-btn').html(Strings.REMOVE_PLACE_DELETE_BUTTON_PENDING);
            sendUpdatePlaceRequest(
              placeID,
              '',
              null,
              true,
              function() {
                reloadDomainInfo();
                dialog.modal('hide');
              }, function() {
                $('.delete-place-cancel-btn').removeAttr('disabled');
                $('.delete-place-confirm-btn').removeAttr('disabled');
                $('.delete-place-confirm-btn').html(Strings.REMOVE_PLACE_DELETE_BUTTON);
                bootbox.alert(Strings.REMOVE_PLACE_ERROR);
              });
            return false;
          }
        },
      ]
    });
  }

  if (isTemporary) {
    var editLink = "";
    var deleteColumn = "<td class='buttons'></td>";
  } else {
    var editLink = " <a class='place-edit' href='javascript:void(0);'>Edit</a>";
    var deleteColumn = "<td class='buttons'><a class='place-delete glyphicon glyphicon-remove'></a></td>";
  }

  var row = $("<tr><td>" + name_link + "</td><td>" + path + editLink + "</td>" + deleteColumn + "</tr>");
  row.find(".place-edit").click(placeEditClicked);
  row.find(".place-delete").click(placeDeleteClicked);

  return row;
}

function placeTableRowForPlaceObject(place) {
  var placePathOrIndex = (place.path ? place.path : "/");
  return placeTableRow(place.name, placePathOrIndex, false, place.id);
}

function reloadDomainInfo() {
  $('#' + Settings.PLACES_TABLE_ID + " tbody tr").not('.headers').remove();

  $('.domain-loading-show').show();
  $('.domain-loading-hide').hide();
  $('.domain-loading-error').hide();
  $('.loading-domain-info-spinner').show();
  $('#' + Settings.PLACES_TABLE_ID).append("<tr colspan=3>Hello</tr>");

  getDomainFromAPI(function(data){
    $('.loading-domain-info-spinner').hide();
    $('.domain-loading-show').hide();

    // check if we have owner_places (for a real domain) or a name (for a temporary domain)
    if (data.status == "success") {
      $('.domain-loading-hide').show();
      if (data.domain.owner_places) {
        // add a table row for each of these names
        _.each(data.domain.owner_places, function(place){
          $('#' + Settings.PLACES_TABLE_ID + " tbody").append(placeTableRowForPlaceObject(place));
        });
      } else if (data.domain.name) {
        // add a table row for this temporary domain name
        $('#' + Settings.PLACES_TABLE_ID + " tbody").append(placeTableRow(data.domain.name, '/', true));
      }

      // Update label
      if (showOrHideLabel()) {
        var label = data.domain.label;
        label = label === null ? '' : label;
        $('#network-label').val(label);
      }

      // Update automatic networking
      if (showOrHideAutomaticNetworking()) {
        var autoNetworkingSetting = Settings.data.values.metaverse.automatic_networking;
        var address = data.domain.network_address === null ? "" : data.domain.network_address;
        var port = data.domain.network_port === null ? "" : data.domain.network_port;
        if (autoNetworkingSetting === 'disabled') {
          $('#network-address-port input').val(address + ":" + port);
        } else if (autoNetworkingSetting === 'ip') {
          $('#network-address-port input').val(port);
        }
      }

      if (accessTokenIsSet()) {
        appendAddButtonToPlacesTable();
      }

    } else {
      $('.domain-loading-error').show();
    }
  })
}

function appendDomainIDButtons() {
  var domainIDInput = $(Settings.DOMAIN_ID_SELECTOR);

  var createButton = dynamicButton(Settings.CREATE_DOMAIN_ID_BTN_ID, Strings.CREATE_DOMAIN_BUTTON);
  createButton.css('margin-top', '10px');
  var chooseButton = dynamicButton(Settings.CHOOSE_DOMAIN_ID_BTN_ID, Strings.CHOOSE_DOMAIN_BUTTON);
  chooseButton.css('margin', '10px 0px 0px 10px');

  domainIDInput.after(chooseButton);
  domainIDInput.after(createButton);
}

function editHighFidelityPlace(placeID, name, path) {
  var dialog;

  var modal_body = "<div class='form-group'>";
  modal_body += "<input type='text' id='place-path-input' class='form-control' value='" + path + "'>";
  modal_body += "</div>";

  var modal_buttons = [
    {
      label: Strings.EDIT_PLACE_CANCEL_BUTTON,
      className: "edit-place-cancel-button",
      callback: function() {
        dialog.modal('hide');
      }
    },
    {
      label: Strings.EDIT_PLACE_CONFIRM_BUTTON,
      className: 'edit-place-save-button btn btn-primary',
      callback: function() {
        var placePath = $('#place-path-input').val();

        if (path == placePath) {
          return true;
        }

        $('.edit-place-cancel-button').attr('disabled', 'disabled');
        $('.edit-place-save-button').attr('disabled', 'disabled');
        $('.edit-place-save-button').html(Strings.EDIT_PLACE_BUTTON_PENDING);

        sendUpdatePlaceRequest(
          placeID,
          placePath,
          null,
          false,
          function() {
            dialog.modal('hide')
            reloadDomainInfo();
          },
          function() {
            $('.edit-place-cancel-button').removeAttr('disabled');
            $('.edit-place-save-button').removeAttr('disabled');
            $('.edit-place-save-button').html(Strings.EDIT_PLACE_CONFIRM_BUTTON);
          }
        );

        return false;
      }
    }
  ];

  dialog = bootbox.dialog({
    title: Strings.EDIT_PLACE_TITLE,
    closeButton: false,
    onEscape: true,
    message: modal_body,
    buttons: modal_buttons
  })
}

function appendAddButtonToPlacesTable() {
    var addRow = $("<tr> <td></td> <td></td> <td class='buttons'><a href='#' class='place-add glyphicon glyphicon-plus'></a></td> </tr>");
    addRow.find(".place-add").click(function(ev) {
      ev.preventDefault();
      chooseFromHighFidelityPlaces(Settings.initialValues.metaverse.access_token, null, function(placeName, newDomainID) {
        if (newDomainID) {
          Settings.data.values.metaverse.id = newDomainID;
          var domainIDEl = $("[data-keypath='metaverse.id']");
          domainIDEl.val(newDomainID);
          Settings.initialValues.metaverse.id = newDomainID;
          badgeSidebarForDifferences(domainIDEl);
        }
        reloadDomainInfo();
      });
    });
    $('#' + Settings.PLACES_TABLE_ID + " tbody").append(addRow);
}

function chooseFromHighFidelityDomains(clickedButton) {
  // setup the modal to help user pick their domain
  if (Settings.initialValues.metaverse.access_token) {

    // add a spinner to the choose button
    clickedButton.html("Loading domains...");
    clickedButton.attr('disabled', 'disabled');

    // get a list of user domains from data-web
    $.ajax({
      url: "/api/domains",
      dataType: 'json',
      jsonp: false,
      success: function(data){

        var modal_buttons = {
          cancel: {
            label: 'Cancel',
            className: 'btn-default'
          }
        }

        if (data.data.domains.length) {
          // setup a select box for the returned domains
          modal_body = "<p>Choose the High Fidelity domain you want this domain-server to represent.<br/>This will set your domain ID on the settings page.</p>";
          domain_select = $("<select id='domain-name-select' class='form-control'></select>");
          _.each(data.data.domains, function(domain){
            var domainString = "";

            if (domain.label) {
              domainString += '"' + domain.label+ '" - ';
            }

            domainString += domain.id;

            domain_select.append("<option value='" + domain.id + "'>" + domainString + "</option>");
          })
          modal_body += "<label for='domain-name-select'>Domains</label>" + domain_select[0].outerHTML
          modal_buttons["success"] = {
            label: 'Choose domain',
            callback: function() {
              domainID = $('#domain-name-select').val()
              // set the domain ID on the form
              $(Settings.DOMAIN_ID_SELECTOR).val(domainID).change();
            }
          }
        } else {
          modal_buttons["success"] = {
            label: 'Create new domain',
            callback: function() {
              window.open(URLs.METAVERSE_URL + "/user/domains", '_blank');
            }
          }
          modal_body = "<p>You do not have any domains in your High Fidelity account." +
            "<br/><br/>Go to your domains page to create a new one. Once your domain is created re-open this dialog to select it.</p>"
        }

        bootbox.dialog({
          title: "Choose matching domain",
          onEscape: true,
          message: modal_body,
          buttons: modal_buttons
        })
      },
      error: function() {
        bootbox.alert("Failed to retrieve your domains from the High Fidelity Metaverse");
      },
      complete: function() {
        // remove the spinner from the choose button
        clickedButton.html("Choose from my domains")
        clickedButton.removeAttr('disabled')
      }
    });

    } else {
      bootbox.alert({
        message: "You must have an access token to query your High Fidelity domains.<br><br>" +
        "Please follow the instructions on the settings page to add an access token.",
        title: "Access token required"
      })
    }
  }

function createTemporaryDomain() {
  swal({
    title: 'Create temporary place name',
    text: "This will create a temporary place name and domain ID"
      + " so other users can easily connect to your domain.</br></br>"
      + "In order to make your domain reachable, this will also enable full automatic networking.",
    showCancelButton: true,
    confirmButtonText: 'Create',
    closeOnConfirm: false,
    html: true
  }, function(isConfirm){
    if (isConfirm) {
      showSpinnerAlert('Creating temporary place name');

      // make a get request to get a temporary domain
      $.post(URLs.METAVERSE_URL + '/api/v1/domains/temporary', function(data){
        if (data.status == "success") {
          var domain = data.data.domain;

          // we should have a new domain ID - set it on the domain ID value
          $(Settings.DOMAIN_ID_SELECTOR).val(domain.id).change();

          // we also need to make sure auto networking is set to full
          $('[data-hidden-input="metaverse.automatic_networking"]').val("full").change();

          swal({
            type: 'success',
            title: 'Success!',
            text: "We have created a temporary name and domain ID for you.</br></br>"
              + "Your temporary place name is <strong>" + domain.name + "</strong>.</br></br>"
              + "Press the button below to save your new settings and restart your domain-server.",
            confirmButtonText: 'Save',
            html: true
          }, function(){
            saveSettings();
          });
        }
      });
    }
  });
}

function reloadSettings(callback) {
  $.getJSON('/settings.json', function(data){
    _.extend(data, viewHelpers)

    $('.nav-stacked').html(Settings.sidebarTemplate(data))
    $('#panels').html(Settings.panelsTemplate(data))

    Settings.data = data;
    Settings.initialValues = form2js('settings-form', ".", false, cleanupFormValues, true);

    // append the domain selection modal
    appendDomainIDButtons();

    // call our method to setup the HF account button
    setupHFAccountButton();

    // call our method to setup the place names table
    setupPlacesTable();

    setupDomainNetworkingSettings();
    setupDomainLabelSetting();

    if (Settings.data.values.metaverse.id.length > 0) {
      // now, ask the API for what places, if any, point to this domain
      reloadDomainInfo();

      // we need to ask the API what a shareable name for this domain is
      getShareName(function(success, shareName) {
        if (success) {
          var shareLink = "https://hifi.place/" + shareName;
          $('#visit-domain-link').attr("href", shareLink).show();
        }
      });
    }

    if (Settings.data.values.wizard.cloud_domain) {
      $('#manage-cloud-domains-link').show();

      var cloudWizardExit = qs["cloud-wizard-exit"];
      if (cloudWizardExit != undefined) {
        $('#cloud-domains-alert').show();
      }
    }

    // setup any bootstrap switches
    $('.toggle-checkbox').bootstrapSwitch();

    $('[data-toggle="tooltip"]').tooltip();

    // call the callback now that settings are loaded
    callback(true);
  }).fail(function() {
    // call the failure object since settings load faild
    callback(false)
  });
}

function validateInputs() {
  // check if any new values are bad
  var tables = $('table');

  var inputsValid = true;

  var tables = $('table');

  // clear any current invalid rows
  $('tr.' + Settings.INVALID_ROW_CLASS).removeClass(Settings.INVALID_ROW_CLASS);

  function markParentRowInvalid(rowChild) {
    $(rowChild).closest('tr').addClass(Settings.INVALID_ROW_CLASS);
  }

  _.each(tables, function(table) {
    // validate keys specificially for spaces and equality to an existing key
    var newKeys = $(table).find('tr.' + Settings.NEW_ROW_CLASS + ' td.key');

    var keyWithSpaces = false;
    var empty = false;
    var duplicateKey = false;

    _.each(newKeys, function(keyCell) {
      var keyVal = $(keyCell).children('input').val();

      if (keyVal.indexOf(' ') !== -1) {
        keyWithSpaces = true;
        markParentRowInvalid(keyCell);
        return;
      }

      // make sure the key isn't empty
      if (keyVal.length === 0) {
        empty = true

        markParentRowInvalid(input);
        return;
      }

      // make sure we don't have duplicate keys in the table
      var otherKeys = $(table).find('td.key').not(keyCell);
      _.each(otherKeys, function(otherKeyCell) {
        var keyInput = $(otherKeyCell).children('input');

        if (keyInput.length) {
          if ($(keyInput).val() == keyVal) {
            duplicateKey = true;
          }
        } else if ($(otherKeyCell).html() == keyVal) {
            duplicateKey = true;
        }

        if (duplicateKey) {
          markParentRowInvalid(keyCell);
          return;
        }
      });

    });

    if (keyWithSpaces) {
      showErrorMessage("Error", "Key contains spaces");
      inputsValid = false;
      return
    }

    if (empty) {
      showErrorMessage("Error", "Empty field(s)");
      inputsValid = false;
      return
    }

    if (duplicateKey) {
      showErrorMessage("Error", "Two keys cannot be identical");
      inputsValid = false;
      return;
    }
  });

  return inputsValid;
}

var SETTINGS_ERROR_MESSAGE = "There was a problem saving domain settings. Please try again!";

function saveSettings() {

  if (validateInputs()) {
    // POST the form JSON to the domain-server settings.json endpoint so the settings are saved
    var canPost = true;

    // disable any inputs not changed
    $("input:not([data-changed])").each(function () {
      $(this).prop('disabled', true);
    });

    // grab a JSON representation of the form via form2js
    var formJSON = form2js('settings-form', ".", false, cleanupFormValues, true);

    // check if we've set the basic http password
    if (formJSON["security"]) {

      var password = formJSON["security"]["http_password"];
      var verify_password = formJSON["security"]["verify_http_password"];

      // if they've only emptied out the default password field, we should go ahead and acknowledge
      // the verify password field
      if (password != undefined && verify_password == undefined) {
        verify_password = "";
      }

      // if we have a password and its verification, convert it to sha256 for comparison
      if (password != undefined && verify_password != undefined) {
        formJSON["security"]["http_password"] = sha256_digest(password);
        formJSON["security"]["verify_http_password"] = sha256_digest(verify_password);

        if (password == verify_password) {
          delete formJSON["security"]["verify_http_password"];
        } else {
          bootbox.alert({ "message": "Passwords must match!", "title": "Password Error" });
          canPost = false;
        }
      }
    }

    console.log("----- SAVING ------");
    console.log(formJSON);

    // re-enable all inputs
    $("input").each(function () {
      $(this).prop('disabled', false);
    });

    // remove focus from the button
    $(this).blur();

    if (canPost) {
      if (formJSON["security"]) {
        var username = formJSON["security"]["http_username"];

        var password = formJSON["security"]["http_password"];

        if ((password == sha256_digest("")) && (username == undefined || (username && username.length != 0))) {
          swal({
            title: "Are you sure?",
            text: "You have entered a blank password with a non-blank username. Are you sure you want to require a blank password?",
            type: "warning",
            showCancelButton: true,
            confirmButtonColor: "#5cb85c",
            confirmButtonText: "Yes!",
            closeOnConfirm: true
          },
          function () {
              formJSON["security"]["http_password"] = "";
              postSettings(formJSON);
          });
          return;
        }
      }
      // POST the form JSON to the domain-server settings.json endpoint so the settings are saved
      postSettings(formJSON);
    }
  }
}

$('body').on('click', '.save-button', function(e){
  saveSettings();
  return false;
});

function makeTable(setting, keypath, setting_value) {
  var isArray = !_.has(setting, 'key');
  var categoryKey = setting.categorize_by_key;
  var isCategorized = !!categoryKey && isArray;

  if (!isArray && setting.can_order) {
    setting.can_order = false;
  }

  var html = "";

  if (setting.help) {
    html += "<span class='help-block'>" + setting.help + "</span>"
  }

  var nonDeletableRowKey = setting["non-deletable-row-key"];
  var nonDeletableRowValues = setting["non-deletable-row-values"];

  html += "<table class='table table-bordered' " +
                 "data-short-name='" + setting.name + "' name='" + keypath + "' " +
                 "id='" + (!_.isUndefined(setting.html_id) ? setting.html_id : keypath) + "' " +
                 "data-setting-type='" + (isArray ? 'array' : 'hash') + "'>";

  if (setting.caption) {
    html += "<caption>" + setting.caption + "</caption>"
  }

  // Column groups
  if (setting.groups) {
    html += "<tr class='headers'>"
    _.each(setting.groups, function (group) {
        html += "<td colspan='" + group.span  + "'><strong>" + group.label + "</strong></td>"
    })
    if (!setting.read_only) {
        if (setting.can_order) {
            html += "<td class='" + Settings.REORDER_BUTTONS_CLASSES +
                    "'><a href='javascript:void(0);' class='glyphicon glyphicon-sort'></a></td>";
        }
        html += "<td class='" + Settings.ADD_DEL_BUTTONS_CLASSES + "'></td></tr>"
    }
    html += "</tr>"
  }

  // Column names
  html += "<tr class='headers'>"

  if (setting.numbered === true) {
    html += "<td class='number'><strong>#</strong></td>" // Row number
  }

  if (setting.key) {
    html += "<td class='key'><strong>" + setting.key.label + "</strong></td>" // Key
  }

  var numVisibleColumns = 0;
  _.each(setting.columns, function(col) {
    if (!col.hidden) numVisibleColumns++;
    html += "<td " + (col.hidden ? "style='display: none;'" : "") + "class='data " +
      (col.class ? col.class : '') + "'><strong>" + col.label + "</strong></td>" // Data
  })

  if (!setting.read_only) {
    if (setting.can_order) {
      numVisibleColumns++;
      html += "<td class='" + Settings.REORDER_BUTTONS_CLASSES +
              "'><a href='javascript:void(0);' class='glyphicon glyphicon-sort'></a></td>";
    }
    numVisibleColumns++;
    html += "<td class='" + Settings.ADD_DEL_BUTTONS_CLASSES + "'></td></tr>";
  }

  // populate rows in the table from existing values
  var row_num = 1;

  if (keypath.length > 0 && _.size(setting_value) > 0) {
    var rowIsObject = setting.columns.length > 1;

    _.each(setting_value, function(row, rowIndexOrName) {
      var categoryPair = {};
      var categoryValue = "";
      if (isCategorized) {
        categoryValue = rowIsObject ? row[categoryKey] : row;
        categoryPair[categoryKey] = categoryValue;
        if (_.findIndex(setting_value, categoryPair) === rowIndexOrName) {
          html += makeTableCategoryHeader(categoryKey, categoryValue, numVisibleColumns, setting.can_add_new_categories, "");
        }
      }

      html += "<tr class='" + Settings.DATA_ROW_CLASS + "' " +
        (isCategorized ? ("data-category='" + categoryValue + "'") : "") + " " +
        (isArray ? "" : "name='" + keypath + "." + rowIndexOrName + "'") +
        (isArray ? Settings.DATA_ROW_INDEX + "='" + (row_num - 1) + "'" : "" ) + ">";

      if (setting.numbered === true) {
        html += "<td class='numbered'>" + row_num + "</td>"
      }

      if (setting.key) {
          html += "<td class='key'>" + rowIndexOrName + "</td>"
      }

      var isNonDeletableRow = !setting.can_add_new_rows;

      _.each(setting.columns, function(col) {

        var colValue, colName;
        if (isArray) {
          colValue = rowIsObject ? row[col.name] : row;
          colName = keypath + "[" + rowIndexOrName + "]" + (rowIsObject ? "." + col.name : "");
        } else {
          colValue = row[col.name];
          colName = keypath + "." + rowIndexOrName + "." + col.name;
        }

        isNonDeletableRow = isNonDeletableRow
          || (nonDeletableRowKey === col.name && nonDeletableRowValues.indexOf(colValue) !== -1);

        if (isArray && col.type === "checkbox" && col.editable) {
          html +=
            "<td class='" + Settings.DATA_COL_CLASS + "'name='" + col.name + "'>" +
              "<input type='checkbox' class='form-control table-checkbox' " +
                     "name='" + colName + "'" + (colValue ? " checked" : "") + "/>" +
            "</td>";
        } else if (isArray && col.type === "time" && col.editable) {
          html +=
            "<td class='" + Settings.DATA_COL_CLASS + "'name='" + col.name + "'>" +
              "<input type='time' class='form-control table-time' name='" + colName + "' " +
                     "value='" + (colValue || col.default || "00:00") + "'/>" +
            "</td>";
        } else {
          // Use a hidden input so that the values are posted.
          html +=
            "<td class='" + Settings.DATA_COL_CLASS + "' " + (col.hidden ? "style='display: none;'" : "") +
                "name='" + colName + "'>" +
              colValue +
              "<input type='hidden' name='" + colName + "' value='" + colValue + "'/>" +
            "</td>";
        }

      });

      if (!setting.read_only) {
        if (setting.can_order) {
          html += "<td class='" + Settings.REORDER_BUTTONS_CLASSES+
                  "'><a href='javascript:void(0);' class='" + Settings.MOVE_UP_SPAN_CLASSES + "'></a>"
                  + "<a href='javascript:void(0);' class='" + Settings.MOVE_DOWN_SPAN_CLASSES + "'></a></td>"
        }
        if (isNonDeletableRow) {
          html += "<td></td>";
        } else {
          html += "<td class='" + Settings.ADD_DEL_BUTTONS_CLASSES
                  + "'><a href='javascript:void(0);' class='" + Settings.DEL_ROW_SPAN_CLASSES + "'></a></td>";
        }
      }

      html += "</tr>"

      if (isCategorized && setting.can_add_new_rows && _.findLastIndex(setting_value, categoryPair) === rowIndexOrName) {
        html += makeTableInputs(setting, categoryPair, categoryValue);
      }

      row_num++
    });
  }

  // populate inputs in the table for new values
  if (!setting.read_only) {
    if (setting.can_add_new_categories) {
      html += makeTableCategoryInput(setting, numVisibleColumns);
    }

    if (setting.can_add_new_rows || setting.can_add_new_categories) {
      html += makeTableHiddenInputs(setting, {}, "");
    }
  }
  html += "</table>"

  return html;
}

function makeTableCategoryHeader(categoryKey, categoryValue, numVisibleColumns, canRemove, message) {
  var html =
    "<tr class='" + Settings.DATA_CATEGORY_CLASS + "' data-key='" + categoryKey + "' data-category='" + categoryValue + "'>" +
      "<td colspan='" + (numVisibleColumns - 1) + "' class='" + Settings.TOGGLE_CATEGORY_COLUMN_CLASS + "'>" +
        "<span class='" + Settings.TOGGLE_CATEGORY_SPAN_CLASSES + " " + Settings.TOGGLE_CATEGORY_EXPANDED_CLASS + "'></span>" +
        "<span message='" + message + "'>" + categoryValue + "</span>" +
      "</td>" +
      ((canRemove) ? (
        "<td class='" + Settings.ADD_DEL_BUTTONS_CLASSES + "'>" +
          "<a href='javascript:void(0);' class='" + Settings.DEL_CATEGORY_SPAN_CLASSES + "'></a>" +
        "</td>"
      ) : (
        "<td></td>"
      )) +
    "</tr>";
  return html;
}

function makeTableHiddenInputs(setting, initialValues, categoryValue) {
  var html = "<tr class='inputs'" + (setting.can_add_new_categories && !categoryValue ? " hidden" : "") + " " +
                  (categoryValue ? ("data-category='" + categoryValue + "'") : "") + " " +
                  (setting.categorize_by_key ? ("data-keep-field='" + setting.categorize_by_key + "'") : "") + ">";

  if (setting.numbered === true) {
    html += "<td class='numbered'></td>";
  }

  if (setting.key) {
    html += "<td class='key' name='" + setting.key.name + "'>\
             <input type='text' style='display: none;' class='form-control' placeholder='" + (_.has(setting.key, 'placeholder') ? setting.key.placeholder : "") + "' value=''>\
             </td>"
  }

  _.each(setting.columns, function(col) {
    var defaultValue = _.has(initialValues, col.name) ? initialValues[col.name] : col.default;
    if (col.type === "checkbox") {
      html +=
        "<td class='" + Settings.DATA_COL_CLASS + "'name='" + col.name + "'>" +
          "<input type='checkbox' style='display: none;' class='form-control table-checkbox' " +
                 "name='" + col.name + "'" + (defaultValue ? " checked" : "") + "/>" +
        "</td>";
    } else if (col.type === "select") {
        html += "<td class='" + Settings.DATA_COL_CLASS + "'name='" + col.name + "'>"
        html += "<select style='display: none;' class='form-control' data-hidden-input='" + col.name + "'>'"

        for (var i in col.options) {
            var option = col.options[i];
            html += "<option value='" + option.value + "' " + (option.value == defaultValue ? 'selected' : '') + ">" + option.label + "</option>";
        }

        html += "</select>";
        html += "<input type='hidden' class='table-dropdown form-control trigger-change' name='" + col.name + "' value='" +  defaultValue + "'></td>";
    } else {
      html +=
        "<td " + (col.hidden ? "style='display: none;'" : "") + " class='" + Settings.DATA_COL_CLASS + "' " +
            "name='" + col.name + "'>" +
          "<input type='text' style='display: none;' class='form-control' placeholder='" + (col.placeholder ? col.placeholder : "") + "' " +
                 "value='" + (defaultValue || "") + "' data-default='" + (defaultValue || "") + "'" +
                 (col.readonly ? " readonly" : "") + ">" +
        "</td>";
    }
  })

  if (setting.can_order) {
    html += "<td class='" + Settings.REORDER_BUTTONS_CLASSES + "'></td>"
  }
  html += "<td class='" + Settings.ADD_DEL_BUTTONS_CLASSES +
    "'><a href='javascript:void(0);' class='" + Settings.ADD_ROW_SPAN_CLASSES + "'></a></td>"
  html += "</tr>"

  return html
}

function makeTableCategoryInput(setting, numVisibleColumns) {
  var canAddRows = setting.can_add_new_rows;
  var categoryKey = setting.categorize_by_key;
  var placeholder = setting.new_category_placeholder || "";
  var message = setting.new_category_message || "";
  var html =
    "<tr class='" + Settings.DATA_CATEGORY_CLASS + " inputs' data-can-add-rows='" + canAddRows + "' " +
        "data-key='" + categoryKey + "' data-message='" + message + "'>" +
      "<td colspan='" + (numVisibleColumns - 1) + "'>" +
        "<input type='text' class='form-control' placeholder='" + placeholder + "'/>" +
      "</td>" +
      "<td class='" + Settings.ADD_DEL_BUTTONS_CLASSES + "'>" +
        "<a href='javascript:void(0);' class='" + Settings.ADD_CATEGORY_SPAN_CLASSES + "'></a>" +
      "</td>" +
    "</tr>";
  return html;
}

function getDescriptionForKey(key) {
  for (var i in Settings.data.descriptions) {
    if (Settings.data.descriptions[i].name === key) {
      return Settings.data.descriptions[i];
    }
  }
}

var SAVE_BUTTON_LABEL_SAVE = "Save";
var SAVE_BUTTON_LABEL_RESTART = "Save and restart";
var reasonsForRestart = [];
var numChangesBySection = {};

function badgeSidebarForDifferences(changedElement) {
  // figure out which group this input is in
  var panelParentID = changedElement.closest('.panel').attr('id');

  // if the panel contains non-grouped settings, the initial value is Settings.initialValues
  var isGrouped = $('#' + panelParentID).hasClass('grouped');

  if (isGrouped) {
    var initialPanelJSON = Settings.initialValues[panelParentID]
      ? Settings.initialValues[panelParentID]
      : {};

    // get a JSON representation of that section
    var panelJSON = form2js(panelParentID, ".", false, cleanupFormValues, true)[panelParentID];
  } else {
    var initialPanelJSON = Settings.initialValues;

    // get a JSON representation of that section
    var panelJSON = form2js(panelParentID, ".", false, cleanupFormValues, true);
  }

  var badgeValue = 0
  var description = getDescriptionForKey(panelParentID);

  // badge for any settings we have that are not the same or are not present in initialValues
  for (var setting in panelJSON) {
    if ((!_.has(initialPanelJSON, setting) && panelJSON[setting] !== "") ||
      (!_.isEqual(panelJSON[setting], initialPanelJSON[setting])
      && (panelJSON[setting] !== "" || _.has(initialPanelJSON, setting)))) {
      badgeValue += 1;

      // add a reason to restart
      if (description && description.restart != false) {
        reasonsForRestart.push(setting);
      }
    } else {
        // remove a reason to restart
        if (description && description.restart != false) {
          reasonsForRestart = $.grep(reasonsForRestart, function(v) { return v != setting; });
      }
    }
  }

  // update the list-group-item badge to have the new value
  if (badgeValue == 0) {
    badgeValue = ""
  }

  numChangesBySection[panelParentID] = badgeValue;

  var hasChanges = badgeValue > 0;

  if (!hasChanges) {
    for (var key in numChangesBySection) {
      if (numChangesBySection[key] > 0) {
        hasChanges = true;
        break;
      }
    }
  }

  $(".save-button").prop("disabled", !hasChanges);
  $(".save-button").html(reasonsForRestart.length > 0 ? SAVE_BUTTON_LABEL_RESTART : SAVE_BUTTON_LABEL_SAVE);
  $("a[href='#" + panelParentID + "'] .badge").html(badgeValue);
}

function addTableRow(row) {
  var table = row.parents('table');
  var isArray = table.data('setting-type') === 'array';
  var keepField = row.data("keep-field");

  var columns = row.parent().children('.' + Settings.DATA_ROW_CLASS);

  var input_clone = row.clone();

  if (!isArray) {
    // show the key input
    var keyInput = row.children(".key").children("input");
  }

  // Change input row to data row
  var table = row.parents("table");
  var setting_name = table.attr("name");
  row.addClass(Settings.DATA_ROW_CLASS + " " + Settings.NEW_ROW_CLASS);

  // if this is an array, add the row index (which is the index of the last row + 1)
  // as a data attribute to the row
  var row_index = 0;
  if (isArray) {
    var previous_row = row.siblings('.' + Settings.DATA_ROW_CLASS + ':last');

    if (previous_row.length > 0) {
      row_index = parseInt(previous_row.attr(Settings.DATA_ROW_INDEX), 10) + 1;
    } else {
      row_index = 0;
    }

    row.attr(Settings.DATA_ROW_INDEX, row_index);
  }

  var focusChanged = false;

  _.each(row.children(), function(element) {
    if ($(element).hasClass("numbered")) {
      // Index row
      var numbers = columns.children(".numbered")
      if (numbers.length > 0) {
        $(element).html(parseInt(numbers.last().text()) + 1)
      } else {
        $(element).html(1)
      }
    } else if ($(element).hasClass(Settings.REORDER_BUTTONS_CLASS)) {
      $(element).html("<td class='" + Settings.REORDER_BUTTONS_CLASSES + "'><a href='javascript:void(0);'"
                      + " class='" + Settings.MOVE_UP_SPAN_CLASSES + "'></a><a href='javascript:void(0);' class='"
                      + Settings.MOVE_DOWN_SPAN_CLASSES + "'></span></td>")
    } else if ($(element).hasClass(Settings.ADD_DEL_BUTTONS_CLASS)) {
      // Change buttons
      var anchor = $(element).children("a")
      anchor.removeClass(Settings.ADD_ROW_SPAN_CLASSES)
      anchor.addClass(Settings.DEL_ROW_SPAN_CLASSES)
    } else if ($(element).hasClass("key")) {
      var input = $(element).children("input")
      input.show();
    } else if ($(element).hasClass(Settings.DATA_COL_CLASS)) {
      // show inputs
      var input = $(element).find("input");
      input.show();

      var isCheckbox = input.hasClass("table-checkbox");
      var isDropdown = input.hasClass("table-dropdown");

      if (isArray) {
        var key = $(element).attr('name');

        // are there multiple columns or just one?
        // with multiple we have an array of Objects, with one we have an array of whatever the value type is
        var num_columns = row.children('.' + Settings.DATA_COL_CLASS).length
        var newName = setting_name + "[" + row_index + "]" + (num_columns > 1 ? "." + key : "");

        input.attr("name", newName);

        if (isDropdown) {
          // default values for hidden inputs inside child selects gets cleared so we need to remind it
          var selectElement = $(element).children("select");
          selectElement.attr("data-hidden-input", newName);
          $(element).children("input").val(selectElement.val());
        }
      } else {
        // because the name of the setting in question requires the key
        // setup a hook to change the HTML name of the element whenever the key changes
        var colName = $(element).attr("name");
        keyInput.on('change', function(){
          input.attr("name", setting_name + "." +  $(this).val() + "." + colName);
        });
      }

      if (!focusChanged) {
          input.focus();
          focusChanged = true;
      }

      // if we are adding a dropdown, we should go ahead and make its select
      // element is visible
      if (isDropdown) {
          $(element).children("select").attr("style", "");
      }

      if (isCheckbox) {
        $(input).find("input").attr("data-changed", "true");
      } else {
        input.attr("data-changed", "true");
      }
    } else {
      console.log("Unknown table element");
    }
  });

  input_clone.children('td').each(function () {
    if ($(this).attr("name") !== keepField) {
      $(this).find("input").val($(this).children('input').attr('data-default'));
    }
  });

  if (isArray) {
    updateDataChangedForSiblingRows(row, true)

    // the addition of any table row should remove the empty-array-row
    row.siblings('.empty-array-row').remove()
  }

  badgeSidebarForDifferences($(table))

  row.after(input_clone)
}

function deleteTableRow($row) {
  var $table = $row.closest('table');
  var categoryName = $row.data("category");
  var isArray = $table.data('setting-type') === 'array';

  $row.empty();

  if (!isArray) {
    if ($row.attr('name')) {
      $row.html("<input type='hidden' class='form-control' name='" + $row.attr('name') + "' data-changed='true' value=''>");
    } else {
      // for rows that didn't have a key, simply remove the row
      $row.remove();
    }
  } else {
    if ($table.find('.' + Settings.DATA_ROW_CLASS + "[data-category='" + categoryName + "']").length <= 1) {
      // This is the last row of the category, so delete the header
      $table.find('.' + Settings.DATA_CATEGORY_CLASS + "[data-category='" + categoryName + "']").remove();
    }

    if ($table.find('.' + Settings.DATA_ROW_CLASS).length > 1) {
      updateDataChangedForSiblingRows($row);

      // this isn't the last row - we can just remove it
      $row.remove();
    } else {
      // this is the last row, we can't remove it completely since we need to post an empty array
      $row
        .removeClass(Settings.DATA_ROW_CLASS)
        .removeClass(Settings.NEW_ROW_CLASS)
        .removeAttr("data-category")
        .addClass('empty-array-row')
        .html("<input type='hidden' class='form-control' name='" + $table.attr("name").replace('[]', '') + "' " +
              "data-changed='true' value=''>");
    }
  }

  // we need to fire a change event on one of the remaining inputs so that the sidebar badge is updated
  badgeSidebarForDifferences($table);
}

function addTableCategory($categoryInputRow) {
  var $input = $categoryInputRow.find("input").first();
  var categoryValue = $input.prop("value");
  if (!categoryValue || $categoryInputRow.closest("table").find("tr[data-category='" + categoryValue + "']").length !== 0) {
    $categoryInputRow.addClass("has-warning");

    setTimeout(function () {
      $categoryInputRow.removeClass("has-warning");
    }, 400);

    return;
  }

  var $rowInput = $categoryInputRow.next(".inputs").clone();
  if (!$rowInput) {
    console.error("Error cloning inputs");
  }

  var canAddRows = $categoryInputRow.data("can-add-rows");
  var message = $categoryInputRow.data("message");
  var categoryKey = $categoryInputRow.data("key");
  var width = 0;
  $categoryInputRow
    .children("td")
    .each(function () {
      width += $(this).prop("colSpan") || 1;
    });

  $input
    .prop("value", "")
    .focus();

  $rowInput.find("td[name='" + categoryKey + "'] > input").first()
    .prop("value", categoryValue);
  $rowInput
    .attr("data-category", categoryValue)
    .addClass(Settings.NEW_ROW_CLASS);

  var $newCategoryRow = $(makeTableCategoryHeader(categoryKey, categoryValue, width, true, " - " + message));
  $newCategoryRow.addClass(Settings.NEW_ROW_CLASS);

  $categoryInputRow
    .before($newCategoryRow)
    .before($rowInput);

  if (canAddRows) {
    $rowInput.removeAttr("hidden");
  } else {
    addTableRow($rowInput);
  }
}

function deleteTableCategory($categoryHeaderRow) {
  var categoryName = $categoryHeaderRow.data("category");

  $categoryHeaderRow
    .closest("table")
    .find("tr[data-category='" + categoryName + "']")
    .each(function () {
      if ($(this).hasClass(Settings.DATA_ROW_CLASS)) {
        deleteTableRow($(this));
      } else {
        $(this).remove();
      }
    });
}

function toggleTableCategory($categoryHeaderRow) {
  var $icon = $categoryHeaderRow.find("." + Settings.TOGGLE_CATEGORY_SPAN_CLASS).first();
  var categoryName = $categoryHeaderRow.data("category");
  var wasExpanded = $icon.hasClass(Settings.TOGGLE_CATEGORY_EXPANDED_CLASS);
  if (wasExpanded) {
    $icon
      .addClass(Settings.TOGGLE_CATEGORY_CONTRACTED_CLASS)
      .removeClass(Settings.TOGGLE_CATEGORY_EXPANDED_CLASS);
  } else {
    $icon
      .addClass(Settings.TOGGLE_CATEGORY_EXPANDED_CLASS)
      .removeClass(Settings.TOGGLE_CATEGORY_CONTRACTED_CLASS);
  }
  $categoryHeaderRow
    .closest("table")
    .find("tr[data-category='" + categoryName + "']")
    .toggleClass("contracted", wasExpanded);
}

function moveTableRow(row, move_up) {
  var table = $(row).closest('table')
  var isArray = table.data('setting-type') === 'array'
  if (!isArray) {
    return;
  }

  if (move_up) {
    var prev_row = row.prev()
    if (prev_row.hasClass(Settings.DATA_ROW_CLASS)) {
      prev_row.before(row)
    }
  } else {
    var next_row = row.next()
    if (next_row.hasClass(Settings.DATA_ROW_CLASS)) {
      next_row.after(row)
    }
  }

  // we need to fire a change event on one of the remaining inputs so that the sidebar badge is updated
  badgeSidebarForDifferences($(table))
}

function updateDataChangedForSiblingRows(row, forceTrue) {
  // anytime a new row is added to an array we need to set data-changed for all sibling row inputs to true
  // unless it matches the inital set of values

  if (!forceTrue) {
    // figure out which group this row is in
    var panelParentID = row.closest('.panel').attr('id')
    // get the short name for the setting from the table
    var tableShortName = row.closest('table').data('short-name')

    // get a JSON representation of that section
    var panelSettingJSON = form2js(panelParentID, ".", false, cleanupFormValues, true)[panelParentID][tableShortName]
    if (Settings.initialValues[panelParentID]) {
      var initialPanelSettingJSON = Settings.initialValues[panelParentID][tableShortName]
    } else {
      var initialPanelSettingJSON = {};
    }

    // if they are equal, we don't need data-changed
    isTrue = !_.isEqual(panelSettingJSON, initialPanelSettingJSON)
  } else {
    isTrue = true
  }

  row.siblings('.' + Settings.DATA_ROW_CLASS).each(function(){
    var hiddenInput = $(this).find('td.' + Settings.DATA_COL_CLASS + ' input')
    if (isTrue) {
      hiddenInput.attr('data-changed', isTrue)
    } else {
      hiddenInput.removeAttr('data-changed')
    }
  })
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
