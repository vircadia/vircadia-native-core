$(document).ready(function(){
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

  Settings.extraGroupsAtEnd = Settings.extraDomainGroupsAtEnd;
  Settings.extraGroupsAtIndex = Settings.extraDomainGroupsAtIndex;
  var METAVERSE_URL = URLs.DEFAULT_METAVERSE_URL;

  var SSL_PRIVATE_KEY_FILE_ID = 'ssl-private-key-file';
  var SSL_PRIVATE_KEY_CONTENTS_ID = 'key-contents';
  var SSL_PRIVATE_KEY_CONTENTS_NAME = 'oauth.key-contents';
  var SSL_CERT_UPLOAD_ID = 'ssl-cert-button';
  var SSL_CERT_FILE_ID = 'ssl-cert-file';
  var SSL_CERT_FINGERPRINT_ID = 'cert-fingerprint';
  var SSL_CERT_FINGERPRINT_SPAN_ID = 'cert-fingerprint-span-id';
  var SSL_CERT_CONTENTS_ID = 'cert-contents';
  var SSL_CERT_CONTENTS_NAME = 'oauth.cert-contents';
  var SSL_PRIVATE_KEY_PATH = 'oauth.key';
  var SSL_CERT_PATH = 'oauth.cert';

  Settings.afterReloadActions = function(data) {

    getMetaverseUrl(function(metaverse_url) {
      METAVERSE_URL = metaverse_url;

      // call our method to setup the HF account button
      setupHFAccountButton();

      // call our method to setup the place names table
      setupPlacesTable();
      // hide the places table for now because we do not want that interacted with from the domain-server
      $('#' + Settings.PLACES_TABLE_ID).hide();

      setupDomainNetworkingSettings();
      // setupDomainLabelSetting();

      setupSettingsOAuth(data);

      setupSettingsBackup();

      if (domainIDIsSet()) {
        // now, ask the API for what places, if any, point to this domain
        reloadDomainInfo();

        // we need to ask the API what a shareable name for this domain is
        getShareName(function(success, shareName) {
          if (success) {
            var shareLink = "https://hifi.place/" + shareName;
            $('#visit-domain-link').attr("href", shareLink).show();
          }
        });
      } else if (accessTokenIsSet()) {
        $('#' + Settings.GET_TEMPORARY_NAME_BTN_ID).show();
      }

      if (Settings.data.values.wizard.cloud_domain) {
        $('#manage-cloud-domains-link').show();

        var cloudWizardExit = qs["cloud-wizard-exit"];
        if (cloudWizardExit != undefined) {
          $('#cloud-domains-alert').show();
        }

        $(Settings.DOMAIN_ID_SELECTOR).siblings('span').append("</br><strong>Changing the domain ID for a Cloud Domain may result in an incorrect status for the domain on your Cloud Domains page.</strong>");
      } else {
        // append the domain selection modal
        appendDomainIDButtons();
      }

      handleAction();
    });
  }

  Settings.handlePostSettings = function(formJSON) {
      
    if (!verifyAvatarHeights()) {
        return false;
    }
	  
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
          return false;
        }
      }
    }

    console.log("----- handlePostSettings() called ------");
    console.log(formJSON);

    if (formJSON["security"]) {
      var username = formJSON["security"]["http_username"];

      var password = formJSON["security"]["http_password"];

      if ((password == sha256_digest("")) && (username == undefined || (username && username.length != 0))) {
        swalAreYouSure(
          "You have entered a blank password with a non-blank username. Are you sure you want to require a blank password?",
          "Use blank password",
          function() {
            swal.close();
            
            formJSON["security"]["http_password"] = "";

            postSettings(formJSON);
          }
        );

        return;
      }
    }

    if (formJSON["oauth"]) {
      var private_key = formJSON["oauth"]["key-contents"];
      var cert = formJSON["oauth"]["cert-contents"];
      var oauthErrors = "";
      if (private_key != undefined) {
        var pattern = /-+BEGIN PRIVATE KEY-+[A-Za-z0-9+/\n=]*-+END PRIVATE KEY-+/m;
        if (!pattern.test(private_key)) {
          oauthErrors += "Private key must be in PEM format<BR/>";
        }
      }
      if (cert != undefined) {
        var pattern = /-+BEGIN CERTIFICATE-+[A-Za-z0-9+/\n=]*-+END CERTIFICATE-+/m;
        if (!pattern.test(cert)) {
          oauthErrors += "Certificate must be in PEM format<BR/>";
        }
      }
      if ($('#oauth.panel').length) {
        if (!$('input[name="oauth.client-id"]').val()) {
          oauthErrors += "OAuth requires a client Id.<BR/>";
        }
        if (!$('input[name="oauth.provider"]').val()) {
          oauthErrors += "OAuth requires a provider.<BR/>";
        }
        if (!$('input[name="oauth.hostname"]').val()) {
          oauthErrors += "OAuth requires a hostname.<BR/>";
        }
        if (!$('input[name="' + SSL_PRIVATE_KEY_PATH + '"]').val() && !$('input[name="' + SSL_PRIVATE_KEY_CONTENTS_NAME + '"]').val()) {
          oauthErrors += "OAuth requires an SSL Private Key.<BR/>";
        }
        if (!$('input[name="' + SSL_CERT_PATH + '"]').val() && !$('input[name="' + SSL_CERT_CONTENTS_NAME + '"]').val()) {
          oauthErrors += "OAuth requires an SSL Certificate.<BR/>";
        }
        if (!$("table[name='oauth.admin-users'] tr.value-row").length &&
            !$("table[name='oauth.admin-roles'] tr.value-row").length) {
          oauthErrors += "OAuth must have at least one admin user or admin role.<BR/>";
        }
      }
      if (oauthErrors) {
        bootbox.alert({ "message": oauthErrors, "title": "OAuth Configuration Error" });
        return false;
      }
    }
    postSettings(formJSON);
  };

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

  function accessTokenIsSet() {
    if (typeof Settings.data.values.metaverse !== 'undefined' &&
      typeof Settings.data.values.metaverse.access_token !== 'undefined') {

      return Settings.data.values.metaverse.access_token.length > 0;
    } else {
      return false;
    }
  }

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
      if (domainIDIsSet()) {
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
              text: "There was a problem retrieving domain information from the Metaverse API.",
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

  function setupHFAccountButton() {

    var hasAccessToken = accessTokenIsSet();
    var el;

    if (hasAccessToken) {
      el = "<p>";
      el += "<span class='account-connected-header'>Metaverse Account Connected</span>";
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
      buttonSetting.button_label = "Connect Metaverse Account";
      buttonSetting.html_id = Settings.CONNECT_ACCOUNT_BTN_ID;

      buttonSetting.href = METAVERSE_URL + "/user/tokens/new?for_domain_server=true";

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

  function showDomainCreationAlert(justConnected) {
    swal({
      title: 'Create new domain ID',
      type: 'input',
      text: 'Enter a label for this machine.</br></br>This will help you identify which domain ID belongs to which machine.</br></br>',
      showCancelButton: true,
      confirmButtonText: "Create",
      closeOnConfirm: false,
      html: true
    }, function (inputValue) {
      if (inputValue === false) {
        swal.close();

        // user cancelled domain ID creation - if we're supposed to save after cancel then save here
        if (justConnected) {
          saveSettings();
        }
      } else {
        // we're going to change the alert to a new one with a spinner while we create this domain
        // showSpinnerAlert('Creating domain ID');
        createNewDomainID(inputValue, justConnected);
      }
    });
  }

  function createNewDomainID(label, justConnected) {
    // get the JSON object ready that we'll use to create a new domain
    var domainJSON = {
      "label": label
    }

    $.post("/api/domains", domainJSON, function(data) {
      if (data.status === "failure") {
        failedToCreateDomainID(data, justConnected);
        return;
      }

      // we successfully created a domain ID, set it on that field
      var domainID = data.domain.domainId;
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
      }, function () {
        saveSettings();
      });
    }, 'json').fail(function (data) {
      failedToCreateDomainID(data, justConnected);
    });
  }
  
  function failedToCreateDomainID(data, justConnected) {
    var errorText = "There was a problem creating your new domain ID. Do you want to try again or";

    if (data && data.status === "failure") {
      errorText = "Error: " + data.error + "</br>Do you want to try again or";
      console.log("Error: " + data.error);
    } else {
      console.log("Error: Failed to post to metaverse.");
    }

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
    }, function (isConfirm) {
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
      if (isCloudDomain()) {
        modal_body += '<div style="color:red;font-weight: bold">Changing the network settings may actually break your domain.</div>';
      }
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
      help: "To point places to this domain, "
        + " go to the <a href='" + METAVERSE_URL + "/user/places'>Places</a> "
        + "page in your Metaverse account.",
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
    $('#places .panel-body').prepend(placesTableGroup);

    var spinner = createDomainSpinner();
    $('#' + Settings.PLACES_TABLE_ID).after($(spinner));

    var errorEl = createDomainLoadingError("There was an error retrieving your places.");
    $("#" + Settings.PLACES_TABLE_ID).after(errorEl);

    // DISABLE TEMP PLACE NAME BUTTON...
    // var temporaryPlaceButton = dynamicButton(Settings.GET_TEMPORARY_NAME_BTN_ID, 'Get a temporary place name');
    // temporaryPlaceButton.hide();
    // $('#' + Settings.PLACES_TABLE_ID).after(temporaryPlaceButton);
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
        $('#' + Settings.GET_TEMPORARY_NAME_BTN_ID).hide();
        $('.domain-loading-hide').show();
        if (data.domain.owner_places && data.domain.owner_places.length > 0) {
          // add a table row for each of these names
          _.each(data.domain.owner_places, function(place){
            $('#' + Settings.PLACES_TABLE_ID + " tbody").append(placeTableRowForPlaceObject(place));
          });
        } else if (data.domain.name) {
          // add a table row for this temporary domain name
          $('#' + Settings.PLACES_TABLE_ID + " tbody").append(placeTableRow(data.domain.name, '/', true));
        } else {
          $('#' + Settings.GET_TEMPORARY_NAME_BTN_ID).show();
        }
        // Update label
        if (showOrHideLabel()) {
          var label = data.domain.name;
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

        if (getCurrentDomainIDType() === DOMAIN_ID_TYPE_TEMP) {
          $(Settings.DOMAIN_ID_SELECTOR).siblings('span').append("  <b>This is a temporary domain and will not be visible in your domain list.</b>");
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
        chooseFromMetaversePlaces(Settings.initialValues.metaverse.access_token, null, function(placeName, newDomainID) {
          if (newDomainID) {
            Settings.data.values.metaverse.id = newDomainID;
            var domainIDEl = $("[data-keypath='metaverse.id']");
            domainIDEl.val(newDomainID);
            Settings.initialValues.metaverse.id = newDomainID;
            badgeForDifferences(domainIDEl);
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
            modal_body = "<p>Choose the Metaverse domain you want this domain-server to represent.<br/>This will set your domain ID on the settings page.</p>";
            domain_select = $("<select id='domain-name-select' class='form-control'></select>");
            _.each(data.data.domains, function(domain){
              var domainString = "";

              if (domain.name) {
                domainString += '"' + domain.name+ '" - ';
              }

              domainString += domain.domainId;

              domain_select.append("<option value='" + domain.domainId + "'>" + domainString + "</option>");
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
                window.open(METAVERSE_URL + "/user/domains", '_blank');
              }
            }
            modal_body = "<p>You do not have any domains in your Metaverse account." +
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
          bootbox.alert("Failed to retrieve your domains from the Metaverse");
        },
        complete: function() {
          // remove the spinner from the choose button
          clickedButton.html("Choose from my domains")
          clickedButton.removeAttr('disabled')
        }
      });

      } else {
        bootbox.alert({
          message: "You must have an access token to query your Metaverse domains.<br><br>" +
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
        $.post(METAVERSE_URL + '/api/v1/domains/temporary', function(data){
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

  function setupSettingsOAuth(data) {
    // construct the HTML needed for the settings backup panel
    var html = "<div class='form-group undefined'>";
    html += "<label class='control-label'>SSL Private Key</label><BR/>";
    html += "<label id='key-path-label'class='control-label'>Path</label>";
    html += "<input id='" + SSL_PRIVATE_KEY_FILE_ID + "' type='file' accept='.key'/>";
    html += "<input id='" + SSL_PRIVATE_KEY_CONTENTS_ID + "' name='" + SSL_PRIVATE_KEY_CONTENTS_NAME + "' type='hidden'/>";
    html += "</div>";
    html += "<div class='form-group undefined'>";
    html += "<label class='control-label'>SSL Cert</label>";
    html += "<div id='cert-fingerprint'><b>Fingerprint:</b><span id='" + SSL_CERT_FINGERPRINT_SPAN_ID + "'>" + data.values.oauth["cert-fingerprint"] + "</span></div>";
    html += "<label id='cert-path-label' class='control-label'>Path</label>";
    html += "<input id='" + SSL_CERT_FILE_ID + "' type='file' accept='.cer,.crt'/>";
    html += "<input id='" + SSL_CERT_CONTENTS_ID + "' name='" + SSL_CERT_CONTENTS_NAME + "' type='hidden'/>";
    html += "</div>";

    $('#oauth-advanced').append(html);

    $('#key-path-label').after($('[data-keypath="' + SSL_PRIVATE_KEY_PATH + '"]'));
    $('#cert-path-label').after($('[data-keypath="' + SSL_CERT_PATH + '"]'));
    $('[name="' + SSL_PRIVATE_KEY_PATH + '"]').val(data.values.oauth.key);
    $('[name="' + SSL_CERT_PATH + '"]').val(data.values.oauth.cert);

    $('body').on('change input propertychange', '#' + SSL_PRIVATE_KEY_FILE_ID, function(e){
      var f = e.target.files[0];
      var reader = new FileReader();
      reader.onload = function(e) {
        $('#' + SSL_PRIVATE_KEY_CONTENTS_ID).val(reader.result);
        $('#' + SSL_PRIVATE_KEY_CONTENTS_ID).attr('data-changed', true);
        $('[name="' + SSL_PRIVATE_KEY_PATH + '"]').val('');
        badgeForDifferences($('#' + SSL_PRIVATE_KEY_CONTENTS_ID));
      }
      reader.readAsText(f);
    });
    $('body').on('change input propertychange', '#' + SSL_CERT_FILE_ID, function(e){
      var f = e.target.files[0];
      var reader = new FileReader();
      reader.onload = function(e) {
        $('#' + SSL_CERT_CONTENTS_ID).val(reader.result);
        $('#' + SSL_CERT_CONTENTS_ID).attr('data-changed', true);
        $('[name="' + SSL_CERT_PATH + '"]').val('');
        $('#' + SSL_CERT_FINGERPRINT_SPAN_ID).text('');
        badgeForDifferences($('#' + SSL_CERT_CONTENTS_ID));
      }
      reader.readAsText(f);
    });

    $('body').on('change input propertychange', '[name="' + SSL_PRIVATE_KEY_PATH + '"]', function(e){
      $('#' + SSL_PRIVATE_KEY_FILE_ID).val('');
      $('#' + SSL_PRIVATE_KEY_CONTENTS_ID).val('');
      badgeForDifferences($('[name="' + SSL_PRIVATE_KEY_PATH + '"]').attr('data-changed', true));
    });

    $('body').on('change input propertychange', '[name="' + SSL_CERT_PATH + '"]', function(e){
      $('#' + SSL_CERT_FILE_ID).val('');
      $('#' + SSL_CERT_CONTENTS_ID).val('');
      $('#' + SSL_CERT_FINGERPRINT_SPAN_ID).text('');
      badgeForDifferences($('[name="' + SSL_CERT_PATH + '"]').attr('data-changed', true));
    });
  }

  var RESTORE_SETTINGS_UPLOAD_ID = 'restore-settings-button';
  var RESTORE_SETTINGS_FILE_ID = 'restore-settings-file';

  // when the restore button is clicked, AJAX send the settings file to DS
  // to restore its settings
  $('body').on('click', '#' + RESTORE_SETTINGS_UPLOAD_ID, function(e){
    e.preventDefault();

    swalAreYouSure(
      "Your domain settings will be replaced by the uploaded settings",
      "Restore settings",
      function() {
        var files = $('#' + RESTORE_SETTINGS_FILE_ID).prop('files');

        var fileFormData = new FormData();
        fileFormData.append('restore-file', files[0]);

        showSpinnerAlert("Restoring Settings");

        $.ajax({
          url: '/settings/restore',
          type: 'POST',
          processData: false,
          contentType: false,
          dataType: 'json',
          data: fileFormData
        }).done(function(data, textStatus, jqXHR) {
          swal.close();
          showRestartModal();
        }).fail(function(jqXHR, textStatus, errorThrown) {
          showErrorMessage(
            "Error",
            "There was a problem restoring domain settings.\n"
            + "Please ensure that your current domain settings are valid and try again."
          );

          reloadSettings();
        });
      }
    );
  });

  $('body').on('change', '#' + RESTORE_SETTINGS_FILE_ID, function() {
    if ($(this).val()) {
        $('#' + RESTORE_SETTINGS_UPLOAD_ID).attr('disabled', false);
    }
  });

  function setupSettingsBackup() {
    // construct the HTML needed for the settings backup panel
    var html = "<div class='form-group'>";

    html += "<label class='control-label'>Save Your Settings Configuration</label>";
    html += "<span class='help-block'>Download this domain's settings to quickly configure another domain or to restore them later</span>";
    html += "<a href='/settings/backup.json' class='btn btn-primary'>Download Domain Settings</a>";
    html += "</div>";

    html += "<div class='form-group'>";
    html += "<label class='control-label'>Upload a Settings Configuration</label>";
    html += "<span class='help-block'>Upload a settings configuration to quickly configure this domain";
    html += "<br/>Note: Your domain settings will be replaced by the settings you upload</span>";

    html += "<input id='restore-settings-file' name='restore-settings' type='file'>";
    html += "<button type='button' id='" + RESTORE_SETTINGS_UPLOAD_ID + "' disabled='true' class='btn btn-primary'>Upload Domain Settings</button>";

    html += "</div>";

    $('#settings_backup .panel-body').html(html);
  }
  
  function verifyAvatarHeights() {
    var errorString = '';
    var minAllowedHeight = 0.009;
    var maxAllowedHeight = 1755;
    var alertCss = { backgroundColor: '#ffa0a0' };
    var minHeightElement = $('input[name="avatars.min_avatar_height"]');
    var maxHeightElement = $('input[name="avatars.max_avatar_height"]');

    var minHeight = Number(minHeightElement.val());
    var maxHeight = Number(maxHeightElement.val());

    if (maxHeight < minHeight) {
      errorString = 'Maximum avatar height must not be less than minimum avatar height<br>';
      minHeightElement.css(alertCss);
      maxHeightElement.css(alertCss);
    };
    if (minHeight < minAllowedHeight) {
      errorString += 'Minimum avatar height must not be less than ' + minAllowedHeight + '<br>';
      minHeightElement.css(alertCss);
    }
    if (maxHeight > maxAllowedHeight) {
      errorString += 'Maximum avatar height must not be greater than ' + maxAllowedHeight + '<br>';
      maxHeightElement.css(alertCss);
    }

    if (errorString.length > 0) {
	  swal({
        type: 'error',
        title: '',
        text: errorString,
        html: true
	  });
	  return false;
    } else {
      return true;
    }

  }
});
