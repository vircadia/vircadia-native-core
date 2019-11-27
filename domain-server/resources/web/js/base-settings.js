var DomainInfo = null;

var viewHelpers = {
  getFormGroup: function(keypath, setting, values, isAdvanced) {
    if (setting.hidden) {
        return "";
    }
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
        if (setting.help) {
          form_group += "<span class='help-block'>" + setting.help + "</span>"
        }
      }
    }

    form_group += "</div>"
    return form_group
  }
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

function reloadSettings(callback) {
  $.getJSON(Settings.endpoint, function(data){
    _.extend(data, viewHelpers);

    for (var spliceIndex in Settings.extraGroupsAtIndex) {
      data.descriptions.splice(spliceIndex, 0, Settings.extraGroupsAtIndex[spliceIndex]);
    }

    for (var endGroupIndex in Settings.extraGroupsAtEnd) {
      data.descriptions.push(Settings.extraGroupsAtEnd[endGroupIndex]);
    }

    data.descriptions = data.descriptions.map(function(x) {
      x.hidden = x.hidden || (x.show_on_enable && data.values[x.name] && !data.values[x.name].enable);
      return x;
    });

    $('#panels').html(Settings.panelsTemplate(data));

    Settings.data = data;
    Settings.initialValues = form2js('settings-form', ".", false, cleanupFormValues, true);

    Settings.afterReloadActions(data);

    // setup any bootstrap switches
    $('.toggle-checkbox').bootstrapSwitch();

    $('[data-toggle="tooltip"]').tooltip();

    Settings.pendingChanges = 0;

    // call the callback now that settings are loaded
    if (callback) { 
      callback(true);
    }
  }).fail(function() {
    // call the failure object since settings load faild
    if (callback) {
      callback(false);
    }
  });
}

function postSettings(jsonSettings) {
  // POST the form JSON to the domain-server settings.json endpoint so the settings are saved
  $.ajax(Settings.endpoint, {
    data: JSON.stringify(jsonSettings),
    contentType: 'application/json',
    type: 'POST'
  }).done(function(data){
    if (data.status == "success") {
      if ($(".save-button-text").html() === SAVE_BUTTON_LABEL_RESTART) {
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

$(document).ready(function(){

  $(document).on('click', '.save-button', function(e){
    saveSettings();
    e.preventDefault();
  });

  $.ajaxSetup({
    timeout: 20000,
  });

  reloadSettings(function(success){
    if (success) {
      // if there was a hash in the URL, jump to it now
      if (location.hash) {
        location.href = location.hash;
      }
    } else {
      swal({
        title: '',
        type: 'error',
        text: Strings.LOADING_SETTINGS_ERROR
      });
    }
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
          } else {
              var tableRows = sibling.parent();
              var tableBody = tableRows.parent();

              // if theres no more siblings, we should jump to a new row
              if (sibling.next().length == 0 && tableRows.nextAll().length == 1) {
                  tableBody.find("." + Settings.ADD_ROW_BUTTON_CLASS).click();
              }
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

  $('#' + Settings.FORM_ID).on('change input propertychange', '.' + Settings.TRIGGER_CHANGE_CLASS , function(e){
    // this input was changed, add the changed data attribute to it
    $(this).attr('data-changed', true);

    badgeForDifferences($(this));
  });

  $('#' + Settings.FORM_ID).on('switchChange.bootstrapSwitch', 'input.toggle-checkbox', function(){
    // this checkbox was changed, add the changed data attribute to it
    $(this).attr('data-changed', true);

    badgeForDifferences($(this));
  });

  // Bootstrap switch in table
  $('#' + Settings.FORM_ID).on('change', 'input.table-checkbox', function () {
    // Bootstrap switches in table: set the changed data attribute for all rows in table.
    var row = $(this).closest('tr');
    if (row.hasClass("value-row")) {  // Don't set attribute on input row switches prior to it being added to table.
      row.find('td.' + Settings.DATA_COL_CLASS + ' input').attr('data-changed', true);
      updateDataChangedForSiblingRows(row, true);
      badgeForDifferences($(this));
    }
  });

  $('#' + Settings.FORM_ID).on('change', 'input.table-time', function() {
    // Bootstrap switches in table: set the changed data attribute for all rows in table.
    var row = $(this).closest('tr');
    if (row.hasClass("value-row")) {  // Don't set attribute on input row switches prior to it being added to table.
      row.find('td.' + Settings.DATA_COL_CLASS + ' input').attr('data-changed', true);
      updateDataChangedForSiblingRows(row, true);
      badgeForDifferences($(this));
    }
  });

  $('#' + Settings.FORM_ID).on('change', 'select', function(){
    $("input[name='" +  $(this).attr('data-hidden-input') + "']").val($(this).val()).change();
  });

  var panelsSource = $('#panels-template').html();
  Settings.panelsTemplate = _.template(panelsSource);
});

function dynamicButton(button_id, text) {
  return $("<button type='button' id='" + button_id + "' class='btn btn-primary'>" + text + "</button>");
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

function parseJSONResponse(xhr) {
  try {
    return JSON.parse(xhr.responseText);
  } catch (e) {
  }
  return null;
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

        markParentRowInvalid(input)
        return;
      }

      // make sure we don't have duplicate keys in the table
      var otherKeys = $(table).find('td.key').not(keyCell);
      _.each(otherKeys, function(otherKeyCell) {
        var keyInput = $(otherKeyCell).children('input');

        var lowerNewValue  = keyVal.toLowerCase();

        if (keyInput.length) {
          if ($(keyInput).val().toLowerCase() == lowerNewValue) {
            duplicateKey = true;
          }
        } else if ($(otherKeyCell).html().toLowerCase() == lowerNewValue) {
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

    // re-enable all inputs
    $("input").each(function () {
      $(this).prop('disabled', false);
    });

    // remove focus from the button
    $(this).blur();

    if (Settings.handlePostSettings === undefined) {
      console.log("----- saveSettings() called ------");
      console.log(formJSON);

      // POST the form JSON to the domain-server settings.json endpoint so the settings are saved
      postSettings(formJSON);
    } else {
      Settings.handlePostSettings(formJSON)
    }
  }
}

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
          "<input type='text' style='display: none;' class='form-control " + Settings.TRIGGER_CHANGE_CLASS +
          "' placeholder='" + (col.placeholder ? col.placeholder : "") + "' " +
          "value='" + (defaultValue || "") + "' data-default='" + (defaultValue || "") + "'" +
          (col.readonly ? " readonly" : "") + ">" + "</td>";
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

function badgeForDifferences(changedElement) {
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

  $('.save-button').prop("disabled", !hasChanges);
  $('.save-button-text').html(reasonsForRestart.length > 0 ? SAVE_BUTTON_LABEL_RESTART : SAVE_BUTTON_LABEL_SAVE);

  // add the badge to the navbar item and the panel header
  $("a[href='" + settingsGroupAnchor(Settings.path, panelParentID) + "'] .badge").html(badgeValue);
  $("#" + panelParentID + " .panel-heading .badge").html(badgeValue);

  // make the navbar dropdown show a badge that is the total of the badges of all groups
  var totalChanges = 0;
  $('.panel-heading .badge').each(function(index){
    if (this.innerHTML.length > 0) {
      totalChanges += parseInt(this.innerHTML);
    }
  });

  Settings.pendingChanges = totalChanges;

  if (totalChanges == 0) {
    totalChanges = ""
  }

  var totalBadgeClass = Settings.content_settings ? '.content-settings-badge' : '.domain-settings-badge';

  $(totalBadgeClass).html(totalChanges);
  $('#hamburger-badge').html(totalChanges);
}

function addTableRow(row) {
  var table = row.parents('table');
  var isArray = table.data('setting-type') === 'array';
  var keepField = row.data("keep-field");

  var columns = row.parent().children('.' + Settings.DATA_ROW_CLASS);

  var input_clone = row.clone();

  // Change input row to data row
  var table = row.parents("table");
  var setting_name = table.attr("name");
  row.addClass(Settings.DATA_ROW_CLASS + " " + Settings.NEW_ROW_CLASS);

  if (!isArray) {
    // show the key input
    var keyInput = row.children(".key").children("input");

    // whenever the keyInput changes, re-badge for differences
    keyInput.on('change input propertychange', function(e){
      // update siblings in the row to have the correct name
      var currentKey = $(this).val();

      $(this).closest('tr').find('.value-col input').each(function(index){
        var input = $(this);

        if (currentKey.length > 0) {
          input.attr("name", setting_name + "." +  currentKey + "." + input.parent().attr('name'));
        } else {
          input.removeAttr("name");
        }
      });

      badgeForDifferences($(this));
    });
  }

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
      }

      if (isArray && !focusChanged) {
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

  if (!isArray) {
    keyInput.focus();
  }

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

  badgeForDifferences($(table))

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
  badgeForDifferences($table);
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
  badgeForDifferences($(table));

  // figure out which group this row is in
  var panelParentID = row.closest('.panel').attr('id');

  // get the short name for the setting from the table
  var tableShortName = row.closest('table').data('short-name');

  var changed = tableHasChanged(panelParentID, tableShortName);
  $(table).find('.' + Settings.DATA_ROW_CLASS).each(function(){
    var hiddenInput = $(this).find('td.' + Settings.DATA_COL_CLASS + ' input');
    if (changed) {
      hiddenInput.attr('data-changed', true);
    } else {
      hiddenInput.removeAttr('data-changed');
    }
  });

}

function tableHasChanged(panelParentID, tableShortName) {
  // get a JSON representation of that section
  var panelSettingJSON = form2js(panelParentID, ".", false, cleanupFormValues, true)[panelParentID][tableShortName]
  if (Settings.initialValues[panelParentID]) {
    var initialPanelSettingJSON = Settings.initialValues[panelParentID][tableShortName]
  } else {
    var initialPanelSettingJSON = {};
  }

  return !_.isEqual(panelSettingJSON, initialPanelSettingJSON);
}

function updateDataChangedForSiblingRows(row, forceTrue) {
  // anytime a new row is added to an array we need to set data-changed for all sibling row inputs to true
  // unless it matches the inital set of values

  if (!forceTrue) {
    // figure out which group this row is in
    var panelParentID = row.closest('.panel').attr('id')
    // get the short name for the setting from the table
    var tableShortName = row.closest('table').data('short-name')

    // if they are equal, we don't need data-changed
    isTrue = tableHasChanged(panelParentID, tableShortName);
  } else {
    isTrue = true
  }

  row.siblings('.' + Settings.DATA_ROW_CLASS).each(function(){
    var hiddenInput = $(this).find('td.' + Settings.DATA_COL_CLASS + ' input')
    if (isTrue) {
      hiddenInput.attr('data-changed', isTrue);
    } else {
      hiddenInput.removeAttr('data-changed');
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
