$(document).ready(function(){

  var RESTORE_SETTINGS_UPLOAD_ID = 'restore-settings-button';
  var RESTORE_SETTINGS_FILE_ID = 'restore-settings-file';
  var UPLOAD_CONTENT_ALLOWED_DIV_ID = 'upload-content-allowed';
  var UPLOAD_CONTENT_RECOVERING_DIV_ID = 'upload-content-recovering';

  var isRestoring = false;

  function progressBarHTML(extraClass, label) {
    var html = "<div class='progress'>";
    html += "<div class='" + extraClass + " progress-bar progress-bar-success progress-bar-striped active' role='progressbar' aria-valuemin='0' aria-valuemax='100'>";
    html += label + "<span class='sr-only'></span></div></div>";
    return html;
  }

  function setupBackupUpload() {
    // construct the HTML needed for the settings backup panel
    var html = "<div class='form-group'><div id='" + UPLOAD_CONTENT_ALLOWED_DIV_ID + "'>";

    html += "<span class='help-block'>Upload a content archive (.zip) or entity file (.json, .json.gz) to replace the content of this domain.";
    html += "<br/>Note: Your domain content will be replaced by the content you upload, but the existing backup files of your domain's content will not immediately be changed.</span>";

    html += "<input id='restore-settings-file' name='restore-settings' type='file'>";
    html += "<button type='button' id='" + RESTORE_SETTINGS_UPLOAD_ID + "' disabled='true' class='btn btn-primary'>Upload Content</button>";

    html += "</div><div id='" + UPLOAD_CONTENT_RECOVERING_DIV_ID + "'>";
    html += "<span class='help-block'>Restore in progress</span>";
    html += progressBarHTML('recovery', 'Restoring');
    html += "</div></div>";

    $('#' + Settings.UPLOAD_CONTENT_BACKUP_PANEL_ID + ' .panel-body').html(html);
  }

  // handle content archive or entity file upload

  // when the selected file is changed, enable the button if there's a selected file
  $('body').on('change', '#' + RESTORE_SETTINGS_FILE_ID, function() {
    $('#' + RESTORE_SETTINGS_UPLOAD_ID).attr('disabled', $(this).val().length == 0);
  });

  // when the upload button is clicked, send the file to the DS
  // and reload the page if restore was successful or
  // show an error if not
  $('body').on('click', '#' + RESTORE_SETTINGS_UPLOAD_ID, function(e){
    e.preventDefault();

    swalAreYouSure(
      "Your domain content will be replaced by the uploaded content archive or entity file",
      "Restore content",
      function() {
        var files = $('#' + RESTORE_SETTINGS_FILE_ID).prop('files');

        var fileFormData = new FormData();
        fileFormData.append('restore-file', files[0]);

        showSpinnerAlert("Uploading content to restore");

        $.ajax({
          url: '/content/upload',
          type: 'POST',
          timeout: 3600000, // Set timeout to 1h
          cache: false,
          processData: false,
          contentType: false,
          data: fileFormData
        }).done(function(data, textStatus, jqXHR) {
          isRestoring = true;

          // immediately reload backup information since one should be restoring now
          reloadBackupInformation();

          swal.close();
        }).fail(function(jqXHR, textStatus, errorThrown) {
          showErrorMessage(
            "Error",
            "There was a problem restoring domain content.\n"
            + "Please ensure that the content archive or entity file is valid and try again."
          );
        });
      }
    );
  });

  var GENERATE_ARCHIVE_BUTTON_ID = 'generate-archive-button';
  var CONTENT_ARCHIVES_NORMAL_ID = 'content-archives-success';
  var CONTENT_ARCHIVES_ERROR_ID = 'content-archives-error';
  var AUTOMATIC_ARCHIVES_TABLE_ID = 'automatic-archives-table';
  var AUTOMATIC_ARCHIVES_TBODY_ID = 'automatic-archives-tbody';
  var MANUAL_ARCHIVES_TABLE_ID = 'manual-archives-table';
  var MANUAL_ARCHIVES_TBODY_ID = 'manual-archives-tbody';
  var AUTO_ARCHIVES_SETTINGS_LINK_ID = 'auto-archives-settings-link';
  var ACTION_MENU_CLASS = 'action-menu';

  var automaticBackups = [];
  var manualBackups = [];

  function setupContentArchives() {
    // construct the HTML needed for the content archives panel
    var html = "<div id='" + CONTENT_ARCHIVES_NORMAL_ID + "'><div class='form-group'>";
    html += "<label class='control-label'>Automatic Content Archives</label>";
    html += "<span class='help-block'>Your domain server makes regular archives of the content in your domain. In the list below, you can see and download all of your backups of domain content and content settings."
    html += "<a href='/settings/#automatic_content_archives' id='" + AUTO_ARCHIVES_SETTINGS_LINK_ID + "'>Click here to manage automatic content archive intervals.</a></span>";
    html += "</div>";
    html += "<table class='table sortable' id='" + AUTOMATIC_ARCHIVES_TABLE_ID + "'>";

    var backups_table_head = "<thead><tr class='gray-tr'><th>Archive Name</th><th data-defaultsort='desc'>Archive Date</th>"
      + "<th data-defaultsort='disabled'></th><th class='" + ACTION_MENU_CLASS + "' data-defaultsort='disabled'>Actions</th>"
      + "</tr></thead>";

    html += backups_table_head;
    html += "<tbody id='" + AUTOMATIC_ARCHIVES_TBODY_ID + "'></tbody></table>";
    html += "<div class='form-group'>";
    html += "<label class='control-label'>Manual Content Archives</label>";
    html += "<span class='help-block'>You can generate and download an archive of your domain content right now. You can also download, delete and restore any archive listed.</span>";
    html += "<button type='button' id='" + GENERATE_ARCHIVE_BUTTON_ID + "' class='btn btn-primary'>Generate New Archive</button>";
    html += "</div>";
    html += "<table class='table sortable' id='" + MANUAL_ARCHIVES_TABLE_ID + "'>";
    html += backups_table_head;
    html += "<tbody id='" + MANUAL_ARCHIVES_TBODY_ID + "'></tbody></table></div>";

    html += "<div class='form-group' id='" + CONTENT_ARCHIVES_ERROR_ID + "' style='display:none;'>"
      + "<span class='help-block'>There was a problem loading your list of automatic and manual content archives. "
      + "Please reload the page to try again.</span></div>";

    // put the base HTML in the content archives panel
    $('#' + Settings.CONTENT_ARCHIVES_PANEL_ID + ' .panel-body').html(html);
  }

  var BACKUP_RESTORE_LINK_CLASS = 'restore-backup';
  var BACKUP_DOWNLOAD_LINK_CLASS = 'download-backup';
  var BACKUP_DELETE_LINK_CLASS = 'delete-backup';
  var ACTIVE_BACKUP_ROW_CLASS = 'active-backup';
  var CORRUPTED_ROW_CLASS = 'danger';

  $('body').on('click',  '.' + BACKUP_DOWNLOAD_LINK_CLASS, function(ev) {
    ev.preventDefault();
    var backupID = $(this).data('backup-id');

    showSpinnerAlert("Preparing backup...");
    function checkBackupStatus() {
      $.ajax({
        url: "/api/backups/" + backupID,
        dataType: 'json',
        success: function(data) {
          if (data.complete) {
            if (data.error == '') {
              location.href = "/api/backups/download/" + backupID;
              swal.close();
            } else {
              showErrorMessage(
                "Error",
                "There was an error preparing your backup. Please refresh the page and try again."
              );
            }
          } else {
            setTimeout(checkBackupStatus, 500);
          }
        },
        error: function() {
          showErrorMessage(
            "Error",
            "There was an error preparing your backup."
          );
        },
      });
    }
    checkBackupStatus();
  });

  function reloadBackupInformation() {
    // make a GET request to get backup information to populate the table
    $.ajax({
      url: '/api/backups',
      cache: false
    }).done(function(data) {

      // split the returned data into manual and automatic manual backups
      var splitBackups = _.partition(data.backups, function(value, index) {
        return value.isManualBackup;
      });

      if (isRestoring && !data.status.isRecovering) {
        // we were recovering and we finished - the DS is going to restart so show the restart modal
        showRestartModal();
        return;
      }

      isRestoring = data.status.isRecovering;

      manualBackups = splitBackups[0];
      automaticBackups = splitBackups[1];

      // populate the backups tables with the backups
      function createBackupTableRow(backup) {
        return "<tr data-backup-id='" + backup.id + "' data-backup-name='" + backup.name + "'>"
          + "<td data-value='" + backup.name.toLowerCase() + "'>" + backup.name + "</td><td data-value='" + backup.createdAtMillis + "'>"
          + moment(backup.createdAtMillis).format('lll')
          + "</td><td class='backup-status'></td><td class='" + ACTION_MENU_CLASS + "'>"
          + "<div class='dropdown'><div class='dropdown-toggle' data-toggle='dropdown' aria-expanded='false'><span class='glyphicon glyphicon-option-vertical'></span></div>"
          + "<ul class='dropdown-menu dropdown-menu-right'>"
          + "<li><a class='" + BACKUP_RESTORE_LINK_CLASS + "' href='#'>Restore from here</a></li><li class='divider'></li>"
          + "<li><a class='" + BACKUP_DOWNLOAD_LINK_CLASS + "' data-backup-id='" + backup.id + "' href='#'>Download</a></li><li class='divider'></li>"
          + "<li><a class='" + BACKUP_DELETE_LINK_CLASS + "' href='#' target='_blank'>Delete</a></li></ul></div></td>";
      }

      function updateProgressBars($progressBar, value) {
        $progressBar.attr('aria-valuenow', value).attr('style', 'width: ' + value + '%');
        $progressBar.find('.sr-only').html(value + "% Complete");
      }

      // before we add any new rows and update existing ones
      // remove our flag for active rows
      $('.' + ACTIVE_BACKUP_ROW_CLASS).removeClass(ACTIVE_BACKUP_ROW_CLASS);

      function updateOrAddTableRow(backup, tableBodyID) {
        // check for a backup with this ID
        var $backupRow = $("tr[data-backup-id='" + backup.id + "']");

        if ($backupRow.length == 0) {
          // create a new row and then add it to the table
          $backupRow = $(createBackupTableRow(backup));
          $('#' + tableBodyID).append($backupRow);
        }

        // update the row status column depending on if it is available or recovering
        if (!backup.isAvailable) {
          // add a progress bar to the status row for availability
          $backupRow.find('td.backup-status').html(progressBarHTML('availability', 'Archiving'));

          // set the value of the progress bar based on availability progress
          updateProgressBars($backupRow.find('.progress-bar'), backup.availabilityProgress * 100);
        } else if (backup.id == data.status.recoveringBackupId) {
          // add a progress bar to the status row for recovery
          $backupRow.find('td.backup-status').html(progressBarHTML('recovery', 'Restoring'));
        } else if (backup.isCorrupted) {
          // add text for corrupted status to row
          $backupRow.find('td.backup-status').html('<span>Corrupted</span>');
        } else {
          // no special status for this row, use an empty status column
          $backupRow.find('td.backup-status').html('');
        }

        // color the row red if it is corrupted
        $backupRow.toggleClass(CORRUPTED_ROW_CLASS, backup.isCorrupted);

        // disable restore if the backup is corrupted
        $backupRow.find('a.' + BACKUP_RESTORE_LINK_CLASS).parent().toggleClass('disabled', backup.isCorrupted);

        // toggle the dropdown menu depending on if the row is available
        $backupRow.find('td.' + ACTION_MENU_CLASS + ' .dropdown').toggle(backup.isAvailable);

        $backupRow.addClass(ACTIVE_BACKUP_ROW_CLASS);
      }

      if (automaticBackups.length > 0) {
        for (var backupIndex in automaticBackups) {
          updateOrAddTableRow(automaticBackups[backupIndex], AUTOMATIC_ARCHIVES_TBODY_ID);
        }
      }

      if (manualBackups.length > 0) {
        for (var backupIndex in manualBackups) {
          updateOrAddTableRow(manualBackups[backupIndex], MANUAL_ARCHIVES_TBODY_ID);
        }
      }

      // at this point, any rows that no longer have the ACTIVE_BACKUP_ROW_CLASS
      // are deleted backups, so we remove them from the table
      $('#' + CONTENT_ARCHIVES_NORMAL_ID + ' tbody tr:not(.' + ACTIVE_BACKUP_ROW_CLASS + ')').remove();

      // check if the restore action on all rows should be enabled or disabled
      $('tr:not(.' + CORRUPTED_ROW_CLASS + ') .' + BACKUP_RESTORE_LINK_CLASS).parent().toggleClass('disabled', data.status.isRecovering);

      // hide or show the manual content upload file and button depending on our recovering status
      $('#' + UPLOAD_CONTENT_ALLOWED_DIV_ID).toggle(!data.status.isRecovering);
      $('#' + UPLOAD_CONTENT_RECOVERING_DIV_ID).toggle(data.status.isRecovering);

      // update the progress bars for current restore status
      if (data.status.isRecovering) {
        updateProgressBars($('.recovery.progress-bar'), data.status.recoveryProgress * 100);
      }

      // tell bootstrap sortable to update for the new rows
      $.bootstrapSortable({ applyLast: true });

      $('#' + CONTENT_ARCHIVES_NORMAL_ID).toggle(true);
      $('#' + CONTENT_ARCHIVES_ERROR_ID).toggle(false);

    }).fail(function(){
      // we've hit the very rare case where we couldn't load the list of backups from the domain server

      // set our backups to empty
      automaticBackups = [];
      manualBackups = [];

      // replace the content archives panel with a simple error message
      // stating that the user should reload the page
      $('#' + CONTENT_ARCHIVES_NORMAL_ID).toggle(false);
      $('#' + CONTENT_ARCHIVES_ERROR_ID).toggle(true);

    }).always(function(){
      // toggle showing or hiding the tables depending on if they have entries
      $('#' + AUTOMATIC_ARCHIVES_TABLE_ID).toggle(automaticBackups.length > 0);
      $('#' + MANUAL_ARCHIVES_TABLE_ID).toggle(manualBackups.length > 0);
    });
  }

  // handle click in table to restore a given content backup
  $('body').on('click', '.' + BACKUP_RESTORE_LINK_CLASS, function(e) {
    // stop the default behaviour
    e.preventDefault();

    // if this is a disabled link, don't proceed with the restore
    if ($(this).parent().hasClass('disabled')) {
      return false;
    }

    // grab the name of this backup so we can show it in alerts
    var backupName = $(this).closest('tr').attr('data-backup-name');

    // grab the ID of this backup in case we need to send a POST
    var backupID = $(this).closest('tr').attr('data-backup-id');

    // make sure the user knows what is about to happen
    swalAreYouSure(
      "Your domain content will be replaced by the content archive " + backupName,
      "Restore content",
      function() {
        // show a spinner while we send off our request
        showSpinnerAlert("Starting restore of " +  backupName);

        // setup an AJAX POST to request content restore
        $.post('/api/backups/recover/' + backupID).done(function(data, textStatus, jqXHR) {
          isRestoring = true;

          // immediately reload our backup information since one should be restoring now
          reloadBackupInformation();

          swal.close();
        }).fail(function(jqXHR, textStatus, errorThrown) {
          showErrorMessage(
            "Error",
            "There was a problem restoring domain content.\n"
            + "If the problem persists, the content archive may be corrupted."
          );
        });
      }
    )
  });

  // handle click in table to delete a given content backup
  $('body').on('click', '.' + BACKUP_DELETE_LINK_CLASS, function(e){
    // stop the default behaviour
    e.preventDefault();

    // grab the name of this backup so we can show it in alerts
    var backupName = $(this).closest('tr').attr('data-backup-name');

    // grab the ID of this backup in case we need to send the DELETE request
    var backupID = $(this).closest('tr').attr('data-backup-id');

    // make sure the user knows what is about to happen
    swalAreYouSure(
      "The content archive " + backupName + " will be deleted and will no longer be available for restore or download from this page.",
      "Delete content archive",
      function() {
        // show a spinner while we send off our request
        showSpinnerAlert("Deleting content archive " +  backupName);

        // setup an AJAX DELETE to request content archive delete
        $.ajax({
          url: '/api/backups/' + backupID,
          type: 'DELETE'
        }).done(function(data, textStatus, jqXHR) {
          swal.close();
        }).fail(function(jqXHR, textStatus, errorThrown) {
          showErrorMessage(
            "Error",
            "There was an unexpected error deleting the content archive"
          );
        }).always(function(){
          // reload the list of content archives in case we deleted a backup
          // or it's no longer an available backup for some other reason
          reloadBackupInformation();
        });
      }
    )
  });

  // handle click on automatic content archive settings link
  $('body').on('click', '#' + AUTO_ARCHIVES_SETTINGS_LINK_ID, function(e) {
    if (Settings.pendingChanges > 0) {
      // don't follow the link right away, make sure the user knows they are about to leave
      // the page and lose changes
      e.preventDefault();

      var settingsLink = $(this).attr('href');

      swalAreYouSure(
        "You have pending changes to content settings that have not been saved. They will be lost if you leave the page to manage automatic content archive intervals.",
        "Proceed without Saving",
        function() {
          // user wants to drop their changes, switch pages
          window.location =  settingsLink;
        }
      );
    }
  });

  // handle click on manual archive creation button
  $('body').on('click', '#' + GENERATE_ARCHIVE_BUTTON_ID, function(e) {
    e.preventDefault();

    // show a sweet alert to ask the user to provide a name for their content archive
    swal({
      title: "Generate a content archive",
      type: "input",
      text: "This will capture the state of all the content in your domain right now, which you can save as a backup and restore from later.",
      confirmButtonText: "Generate Archive",
      showCancelButton: true,
      closeOnConfirm: false,
      inputPlaceholder: 'Archive Name'
    }, function(inputValue){
      if (inputValue === false) {
        return false;
      }

      if (inputValue === "") {
        swal.showInputError("Please give the content archive a name.")
        return false;
      }

      var MANUAL_ARCHIVE_NAME_REGEX = /^[a-zA-Z0-9\-_ ]+$/;
      if (!MANUAL_ARCHIVE_NAME_REGEX.test(inputValue)) {
        swal.showInputError("Valid characters include A-z, 0-9, ' ', '_', and '-'.");
        return false;
      }

      // post the provided archive name to ask the server to kick off a manual backup
      $.ajax({
        type: 'POST',
        url: '/api/backups',
        data: {
          'name': inputValue
        }
      }).done(function(data) {
        // since we successfully setup a new content archive, reload the table of archives
        // which should show that this archive is pending creation
        swal.close();
        reloadBackupInformation();
      }).fail(function(jqXHR, textStatus, errorThrown) {
        showErrorMessage(
          "Error",
          "There was an unexpected error creating the manual content archive"
        )
      });
    });
  });

  Settings.extraGroupsAtIndex = Settings.extraContentGroupsAtIndex;

  Settings.afterReloadActions = function() {
    setupBackupUpload();
    setupContentArchives();

    // load the latest backups immediately
    reloadBackupInformation();

    // setup a timer to reload them every 5 seconds
    setInterval(reloadBackupInformation, 5000);
  };
});
