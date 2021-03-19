if (typeof Settings === "undefined") {
  Settings = {};
}

$.extend(Settings, {
  DEPRECATED_CLASS: 'deprecated-setting',
  TRIGGER_CHANGE_CLASS: 'trigger-change',
  DATA_ROW_CLASS: 'value-row',
  DATA_COL_CLASS: 'value-col',
  DATA_CATEGORY_CLASS: 'value-category',
  ADD_ROW_BUTTON_CLASS: 'add-row',
  ADD_ROW_SPAN_CLASSES: 'glyphicon glyphicon-plus add-row',
  DEL_ROW_BUTTON_CLASS: 'del-row',
  DEL_ROW_SPAN_CLASSES: 'glyphicon glyphicon-remove del-row',
  ADD_CATEGORY_BUTTON_CLASS: 'add-category',
  ADD_CATEGORY_SPAN_CLASSES: 'glyphicon glyphicon-plus add-category',
  TOGGLE_CATEGORY_COLUMN_CLASS: 'toggle-category',
  TOGGLE_CATEGORY_SPAN_CLASS: 'toggle-category-icon',
  TOGGLE_CATEGORY_SPAN_CLASSES: 'glyphicon toggle-category-icon',
  TOGGLE_CATEGORY_EXPANDED_CLASS: 'glyphicon-triangle-bottom',
  TOGGLE_CATEGORY_CONTRACTED_CLASS: 'glyphicon-triangle-right',
  DEL_CATEGORY_BUTTON_CLASS: 'del-category',
  DEL_CATEGORY_SPAN_CLASSES: 'glyphicon glyphicon-remove del-category',
  MOVE_UP_BUTTON_CLASS: 'move-up',
  MOVE_UP_SPAN_CLASSES: 'glyphicon glyphicon-chevron-up move-up',
  MOVE_DOWN_BUTTON_CLASS: 'move-down',
  MOVE_DOWN_SPAN_CLASSES: 'glyphicon glyphicon-chevron-down move-down',
  TABLE_BUTTONS_CLASS: 'buttons',
  ADD_DEL_BUTTONS_CLASS: 'add-del-buttons',
  ADD_DEL_BUTTONS_CLASSES: 'buttons add-del-buttons',
  REORDER_BUTTONS_CLASS: 'reorder-buttons',
  REORDER_BUTTONS_CLASSES: 'buttons reorder-buttons',
  NEW_ROW_CLASS: 'new-row',
  CONNECT_ACCOUNT_BTN_ID: 'connect-account-btn',
  DISCONNECT_ACCOUNT_BTN_ID: 'disconnect-account-btn',
  CREATE_DOMAIN_ID_BTN_ID: 'create-domain-btn',
  CHOOSE_DOMAIN_ID_BTN_ID: 'choose-domain-btn',
  GET_TEMPORARY_NAME_BTN_ID: 'get-temp-name-btn',
  DOMAIN_ID_SELECTOR: '[name="metaverse.id"]',
  ACCESS_TOKEN_SELECTOR: '[name="metaverse.access_token"]',
  PLACES_TABLE_ID: 'places-table',
  ADD_PLACE_BTN_ID: 'add-place-btn',
  FORM_ID: 'settings-form',
  INVALID_ROW_CLASS: 'invalid-input',
  DATA_ROW_INDEX: 'data-row-index',
  CONTENT_ARCHIVES_PANEL_ID: 'content_archives',
  UPLOAD_CONTENT_BACKUP_PANEL_ID: 'upload_content',
  INSTALLED_CONTENT: 'installed_content'
});

var URLs = {
  // STABLE METAVERSE_URL: https://metaverse.highfidelity.com
  // STAGING METAVERSE_URL: https://staging.highfidelity.com
  DEFAULT_METAVERSE_URL: "https://metaverse.vircadia.com/live",
  CDN_URL: 'https://cdn-1.vircadia.com/eu-c-1',
  PLACE_URL: 'https://xr.place'
};

var Strings = {
  LOADING_SETTINGS_ERROR: "There was a problem loading the domain settings.\nPlease refresh the page to try again.",

  CHOOSE_DOMAIN_BUTTON: "Choose from my domains",
  CREATE_DOMAIN_BUTTON: "Create new domain ID",
  CREATE_DOMAIN_SUCCESS_JUST_CONNECTED: "We connnected your Metaverse account and created a new domain ID for this machine.",
  CREATE_DOMAIN_SUCCESS: "We created a new domain ID for this machine.",

  // When a place modification fails, they will be brought back to the previous
  // dialog with new path still set, allowing them to retry immediately, and without
  // having to type the new path in again.
  EDIT_PLACE_TITLE: "Modify Viewpoint or Path",
  EDIT_PLACE_ERROR: "Failed to update Viewpoint or Path for this Place Name. Please try again.",
  EDIT_PLACE_CONFIRM_BUTTON: "Save",
  EDIT_PLACE_CONFIRM_BUTTON_PENDING: "Saving...",
  EDIT_PLACE_CANCEL_BUTTON: "Cancel",

  REMOVE_PLACE_TITLE: "Are you sure you want to remove <strong>{{place}}</strong> and its path information?",
  REMOVE_PLACE_ERROR: "Failed to remove Place Name and its Path information.",
  REMOVE_PLACE_DELETE_BUTTON: "This action removes your Place Name",
  REMOVE_PLACE_DELETE_BUTTON_PENDING: "Deleting...",
  REMOVE_PLACE_CANCEL_BUTTON: "Cancel",

  ADD_PLACE_TITLE: "Choose a place",
  ADD_PLACE_MESSAGE: "Choose a Place Name that you own or register a new Place Name.",
  ADD_PLACE_CONFIRM_BUTTON: "Save",
  ADD_PLACE_CONFIRM_BUTTON_PENDING: "Saving...",
  ADD_PLACE_CANCEL_BUTTON: "Cancel",
  ADD_PLACE_UNKNOWN_ERROR: "There was an error adding this Place Name. Try saving again",

  ADD_PLACE_NO_PLACES_MESSAGE: "You don't have any Place Names registered. Once you have a Place Name, reopen this window to select it.",
  ADD_PLACE_NO_PLACES_BUTTON: "Create new place",
  ADD_PLACE_UNABLE_TO_LOAD_ERROR: "We were unable to load your place names. Please try again later.",
  ADD_PLACE_LOADING_DIALOG: "Loading your places...",

  ADD_PLACE_NOT_CONNECTED_TITLE: "Access token required",
  ADD_PLACE_NOT_CONNECTED_MESSAGE: "You must have an access token to query your Metaverse places.<br><br>Please follow the instructions on the settings page to add an access token.",
};

var DOMAIN_ID_TYPE_NONE = 0;
var DOMAIN_ID_TYPE_TEMP = 1;
var DOMAIN_ID_TYPE_FULL = 2;
var DOMAIN_ID_TYPE_UNKNOWN = 3;

function swalAreYouSure(text, confirmButtonText, callback) {
  swal({
    title: "Are you sure?",
    text: text,
    type: "warning",
    showCancelButton: true,
    confirmButtonText: confirmButtonText,
    closeOnConfirm: false
  }, callback);
}

function domainIDIsSet() {
  if (typeof Settings.data.values.metaverse !== 'undefined' &&
    typeof Settings.data.values.metaverse.id !== 'undefined') {

    return Settings.data.values.metaverse.id.length > 0;
  } else {
    return false;
  }
}

function getCurrentDomainIDType() {
  if (!domainIDIsSet()) {
    return DOMAIN_ID_TYPE_NONE;
  }
  if (typeof DomainInfo === 'undefined') {
    return DOMAIN_ID_TYPE_UNKNOWN;
  }
  if (DomainInfo !== null) {
    // Disabled because detecting as temp domain... and we're not even using temp domains right now.
    // if (DomainInfo.name !== undefined) {
    //   return DOMAIN_ID_TYPE_TEMP;
    // }
    return DOMAIN_ID_TYPE_FULL;
  }
  return DOMAIN_ID_TYPE_UNKNOWN;
}

function isCloudDomain() {

  if (!domainIDIsSet()) {
    return false;
  }
  if (typeof DomainInfo === 'undefined') {
    return false;
  }
  if (DomainInfo === null) {
    return false;
  }
  if (typeof DomainInfo.cloud_domain !== "boolean") {
    return false;
  }
  return DomainInfo.cloud_domain;
}

function showLoadingDialog(msg) {
  var message = '<div class="text-center">';
  message += '<span class="glyphicon glyphicon-refresh glyphicon-refresh-animate"></span> ' + msg;
  message += '</div>';

  return bootbox.dialog({
    message: message,
    closeButton: false
  });
}

function sendUpdatePlaceRequest(id, path, domainID, clearDomainID, onSuccess, onError) {
  var data = {
    place_id: id,
    path: path
  };
  if (domainID) {
    data.domain_id = domainID;
  }
  if (clearDomainID) {
    data.domain_id = null;
  }
  $.ajax({
    url: '/api/places',
    type: 'PUT',
    data: data,
    success: onSuccess,
    error: onError
  });
}

var pendingDomainRequest = null;
function getDomainFromAPI(callback) {
  if (pendingDomainRequest !== null) {
    pendingDomainRequest.success(callback);
    pendingDomainRequest.error(function() { callback({ status: 'fail' }) });
    return pendingDomainRequest;
  }

  if (callback === undefined) {
    callback = function() {};
  }

  if (!domainIDIsSet()) {
    callback({ status: 'fail' });
    return null;
  } else {
    var domainID = Settings.data.values.metaverse.id;
  }

  pendingDomainRequest = $.ajax({
    url: "/api/domains/" + domainID,
    dataType: 'json',
    success: function(data) {
      pendingDomainRequest = null;

      if (data.status === 'success') {
        DomainInfo = data.domain;
      } else {
        DomainInfo = null;
      }
      callback(data);
    },
    error: function() {
      pendingDomainRequest = null;

      DomainInfo = null;
      callback({ status: 'fail' });
    }
  });

  return pendingDomainRequest;
}

function chooseFromMetaversePlaces(accessToken, forcePathTo, onSuccessfullyAdded) {
  if (accessToken) {
    getMetaverseUrl(function(metaverse_url) {

      var loadingDialog = showLoadingDialog(Strings.ADD_PLACE_LOADING_DIALOG);

      function loadPlaces() {
        $.ajax("/api/places", {
          dataType: 'json',
          jsonp: false,
          success: function(data) {
            if (data.status == 'success') {
              var modal_buttons = {
                cancel: {
                  label: Strings.ADD_PLACE_CANCEL_BUTTON,
                  className: 'add-place-cancel-button btn-default'
                }
              };

              var dialog;
              var modal_body;
              if (data.data.places.length) {
                var places_by_id = {};

                modal_body = $('<div>');

                modal_body.append($("<p>Choose a place name that you own or <a href='" + metaverse_url + "/user/places' target='_blank'>register a new place name</a></p>"));

                var currentDomainIDType = getCurrentDomainIDType();
                if (currentDomainIDType === DOMAIN_ID_TYPE_TEMP) {
                  var warning = "<div class='domain-loading-error alert alert-warning'>";
                  warning += "If you choose a place name it will replace your current temporary place name.";
                  warning += "</div>";
                  modal_body.append(warning);
                }

                // setup a select box for the returned places
                modal_body.append($("<label for='place-name-select'>Places</label>"));
                place_select = $("<select id='place-name-select' class='form-control'></select>");
                _.each(data.data.places, function(place) {
                  places_by_id[place.id] = place;
                  place_select.append("<option value='" + place.id + "'>" + place.name + "</option>");
                })
                modal_body.append(place_select);
                modal_body.append($("<p id='place-name-warning' class='warning-text' style='display: none'>This place name already points to a place or path. Saving this would overwrite the previous settings associated with it.</p>"));

                if (forcePathTo === undefined || forcePathTo === null) {
                  var path = "<div class='form-group'>";
                  path += "<label for='place-path-input' class='control-label'>Path or Viewpoint</label>";
                  path += "<input type='text' id='place-path-input' class='form-control' value='/'>";
                  path += "</div>";
                  modal_body.append($(path));
                }

                var place_select = modal_body.find("#place-name-select")
                place_select.change(function(ev) {
                  var warning = modal_body.find("#place-name-warning");
                  var place = places_by_id[$(this).val()];
                  if (place === undefined || place.pointee === null) {
                    warning.hide();
                  } else {
                    warning.show();
                  }
                });
                place_select.trigger('change');

                modal_buttons["success"] = {
                  label: Strings.ADD_PLACE_CONFIRM_BUTTON,
                  className: 'add-place-confirm-button btn btn-primary',
                  callback: function() {
                    var placeID = $('#place-name-select').val();
                    // set the place ID on the form
                    $(Settings.place_ID_SELECTOR).val(placeID).change();

                    if (forcePathTo === undefined || forcePathTo === null) {
                      var placePath = $('#place-path-input').val();
                    } else {
                      var placePath = forcePathTo;
                    }

                    $('.add-place-confirm-button').attr('disabled', 'disabled');
                    $('.add-place-confirm-button').html(Strings.ADD_PLACE_CONFIRM_BUTTON_PENDING);
                    $('.add-place-cancel-button').attr('disabled', 'disabled');

                    function finalizeSaveDomainID(domainID) {
                      var jsonSettings = {
                        metaverse: {
                          id: domainID
                        }
                      }
                      var dialog = showLoadingDialog("Waiting for Domain Server to restart...");
                      $.ajax('/settings.json', {
                        data: JSON.stringify(jsonSettings),
                        contentType: 'application/json',
                        type: 'POST'
                      }).done(function(data) {
                        if (data.status == "success") {
                          waitForDomainServerRestart(function() {
                            dialog.modal('hide');
                            if (onSuccessfullyAdded) {
                              onSuccessfullyAdded(places_by_id[placeID].name, domainID);
                            }
                          });
                        } else {
                          bootbox.alert("Failed to add place");
                        }
                      }).fail(function() {
                        bootbox.alert("Failed to add place");
                      });
                    }

                    // If domainID is not specified, the current domain id will be used.
                    function finishSettingUpPlace(domainID) {
                      sendUpdatePlaceRequest(
                        placeID,
                        placePath,
                        domainID,
                        false,
                        function(data) {
                          dialog.modal('hide')
                          if (domainID) {
                            $(Settings.DOMAIN_ID_SELECTOR).val(domainID).change();
                            finalizeSaveDomainID(domainID);
                          } else {
                            if (onSuccessfullyAdded) {
                              onSuccessfullyAdded(places_by_id[placeID].name);
                            }
                          }
                        },
                        function(data) {
                          $('.add-place-confirm-button').removeAttr('disabled');
                          $('.add-place-confirm-button').html(Strings.ADD_PLACE_CONFIRM_BUTTON);
                          $('.add-place-cancel-button').removeAttr('disabled');
                          bootbox.alert(Strings.ADD_PLACE_UNKNOWN_ERROR);
                        }
                      );
                    }

                    function maybeCreateNewDomainID() {
                      console.log("Maybe creating domain id", currentDomainIDType)
                      if (currentDomainIDType === DOMAIN_ID_TYPE_FULL) {
                        finishSettingUpPlace();
                      } else {
                        sendCreateDomainRequest(function(domainID) {
                          console.log("Created domain", domainID);
                          finishSettingUpPlace(domainID);
                        }, function() {
                          $('.add-place-confirm-button').removeAttr('disabled');
                          $('.add-place-confirm-button').html(Strings.ADD_PLACE_CONFIRM_BUTTON);
                          $('.add-place-cancel-button').removeAttr('disabled');
                          bootbox.alert(Strings.ADD_PLACE_UNKNOWN_ERROR);
                        });
                      }
                    }

                    maybeCreateNewDomainID();

                    return false;
                  }
                }
              } else {
                modal_buttons["success"] = {
                  label: Strings.ADD_PLACE_NO_PLACES_BUTTON,
                  callback: function() {
                    window.open(metaverse_url + "/user/places", '_blank');
                  }
                }
                modal_body = Strings.ADD_PLACE_NO_PLACES_MESSAGE;
              }
              dialog = bootbox.dialog({
                title: Strings.ADD_PLACE_TITLE,
                message: modal_body,
                closeButton: false,
                buttons: modal_buttons,
                onEscape: true
              });
            } else {
              bootbox.alert(Strings.ADD_PLACE_UNABLE_TO_LOAD_ERROR);
            }
          },
          error: function() {
            bootbox.alert(Strings.ADD_PLACE_UNABLE_TO_LOAD_ERROR);
          },
          complete: function() {
            loadingDialog.modal('hide');
          }
        });
      }

      var domainType = getCurrentDomainIDType();
      if (domainType !== DOMAIN_ID_TYPE_UNKNOWN) {
        loadPlaces();
      } else {
        getDomainFromAPI(function(data) {
          if (data.status === 'success') {
            var domainType = getCurrentDomainIDType();
            loadPlaces();
          } else {
            loadingDialog.modal('hide');
            bootbox.confirm("We were not able to load your domain information from the Metaverse. Would you like to retry?", function(response) {
              if (response) {
                chooseFromMetaversePlaces(accessToken, forcePathTo, onSuccessfullyAdded);
              }
            });
          }
        })
      }
    });
  } else {
    bootbox.alert({
      title: Strings.ADD_PLACE_NOT_CONNECTED_TITLE,
      message: Strings.ADD_PLACE_NOT_CONNECTED_MESSAGE
    })
  }
}

function sendCreateDomainRequest(onSuccess, onError) {
  $.ajax({
    url: '/api/domains',
    dataType: 'json',
    type: 'POST',
    data: { label: "" },
    success: function(data) {
      onSuccess(data.domain.id);
    },
    error: onError
  });
}

function waitForDomainServerRestart(callback) {
  function checkForDomainUp() {
    $.ajax('', {
      success: function() {
        callback();
      },
      error: function() {
        setTimeout(checkForDomainUp, 50);
      }
    });
  }

  setTimeout(checkForDomainUp, 10);

}

function prepareAccessTokenPrompt(callback) {
  swal({
    title: "Connect Account",
    type: "input",
    text: "Paste your created access token here." +
      "</br></br>If you did not successfully create an access token click cancel below and attempt to connect your account again.</br></br>",
    showCancelButton: true,
    closeOnConfirm: false,
    html: true
  }, function(inputValue){
    if (inputValue === false) {
      return false;
    }

    if (inputValue === "") {
      swal.showInputError("Please paste your access token in the input field.")
      return false
    }

    if (callback) {
      callback(inputValue);
    }

    swal.close();
  });
}

function createDomainIDPrompt(callback) {
  swal({
    title: 'Finish Registering Domain',
    type: 'input',
    text: 'Enter a label for this Domain Server.</br></br>This will help you identify which domain ID belongs to which server.</br></br>This is a required step for registration.</br></br>Acceptable characters are [A-Z][a-z0-9]+-_.</br',
    showCancelButton: true,
    confirmButtonText: "Create",
    closeOnConfirm: false,
    html: true
  }, function (inputValue) {
    if (inputValue === false) {
      return false;
    }

    if (inputValue === "") {
      swal.showInputError("Please enter a valid label for your machine.");
      return false;
    }

    if (callback) {
      callback(inputValue);
    }
  });
}

function getMetaverseUrl(callback) {
    $.ajax('/api/metaverse_info', {
      success: function(data) {
        callback(data.metaverse_url);
      },
      error: function() {
        callback(URLs.DEFAULT_METAVERSE_URL);
      }
    });
}
