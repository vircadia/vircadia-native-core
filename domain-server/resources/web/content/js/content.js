$(document).ready(function(){

  var RESTORE_SETTINGS_UPLOAD_ID = 'restore-settings-button';
  var RESTORE_SETTINGS_FILE_ID = 'restore-settings-file';

  function setupBackupUpload() {
    // construct the HTML needed for the settings backup panel
    var html = "<div class='form-group'>";

    html += "<span class='help-block'>Upload a Content Backup to replace the content of this domain";
    html += "<br/>Note: Your domain's content will be replaced by the content you upload, but the existing backup files of your domain's content will not immediately be changed.</span>";

    html += "<input id='restore-settings-file' name='restore-settings' type='file'>";
    html += "<button type='button' id='" + RESTORE_SETTINGS_UPLOAD_ID + "' disabled='true' class='btn btn-primary'>Upload Domain Settings</button>";

    html += "</div>";

    $('#' + Settings.UPLOAD_CONTENT_BACKUP_PANEL_ID + ' .panel-body').html(html);
  }

  var GENERATE_ARCHIVE_BUTTON_ID = 'generate-archive-button';
  var AUTOMATIC_ARCHIVES_TABLE_ID = 'automatic-archives-table';
  var AUTOMATIC_ARCHIVES_TBODY_ID = 'automatic-archives-tbody';
  var MANUAL_ARCHIVES_TABLE_ID = 'manual-archives-table';
  var MANUAL_ARCHIVES_TBODY_ID = 'manual-archives-tbody';
  var AUTO_ARCHIVES_SETTINGS_LINK_ID = 'auto-archives-settings-link';

  var automaticBackups = [];
  var manualBackups = [];

  function setupContentArchives() {
    // construct the HTML needed for the content archives panel
    var html = "<div class='form-group'>";
    html += "<label class='control-label'>Automatic Content Archives</label>";
    html += "<span class='help-block'>Your domain server makes regular archives of the content in your domain. In the list below, you can see and download all of your domain content and settings backups. "
    html += "<a href='/settings/#automatic_content_archives' id='" + AUTO_ARCHIVES_SETTINGS_LINK_ID + "'>Click here to manage automatic content archive intervals.</a>";
    html += "</div>";
    html += "<table class='table sortable' id='" + AUTOMATIC_ARCHIVES_TABLE_ID + "'>";

    var backups_table_head = "<thead><tr class='gray-tr'><th>Archive Name</th><th data-defaultsort='desc'>Archive Date</th><th class='text-right' data-defaultsort='disabled'>Actions</th></tr></thead>";

    html += backups_table_head;
    html += "<tbody id='" + AUTOMATIC_ARCHIVES_TBODY_ID + "'></tbody></table>";
    html += "<div class='form-group'>";
    html += "<label class='control-label'>Manual Content Archives</label>";
    html += "<span class='help-block'>You can generate and download an archive of your domain content right now. You can also download, delete and restore any archive listed.</span>";
    html += "<button type='button' id='" + GENERATE_ARCHIVE_BUTTON_ID + "' class='btn btn-primary'>Generate New Archive</button>";
    html += "</div>";
    html += "<table class='table sortable' id='" + MANUAL_ARCHIVES_TABLE_ID + "'>";
    html += backups_table_head;
    html += "<tbody id='" + MANUAL_ARCHIVES_TBODY_ID + "'></tbody></table>";

    // put the base HTML in the content archives panel
    $('#' + Settings.CONTENT_ARCHIVES_PANEL_ID + ' .panel-body').html(html);
  }

  function reloadLatestBackups() {
    // make a GET request to get backup information to populate the table
    $.get('/api/backups', function(data) {
      // split the returned data into manual and automatic manual backups
      var splitBackups = _.partition(data.backups, function(value, index) {
        return value.isManualBackup;
      });

      manualBackups = splitBackups[0];
      automaticBackups = splitBackups[1];

      // populate the backups tables with the backups
      function createBackupTableRow(backup) {
        return "<tr><td data-value='" + backup.name.toLowerCase() + "'>" + backup.name + "</td><td data-dateformat='lll'>"
          + moment(backup.createdAtMillis).format('lll')
          + "</td><td class='text-right'>"
          + "<div class='dropdown'><div class='dropdown-toggle' data-toggle='dropdown' aria-expanded='false'><span class='glyphicon glyphicon-option-vertical'></span></div>"
          + "<ul class='dropdown-menu dropdown-menu-right'><li><a class='update-server' href='#'>Restore from here</a></li><li class='divider'></li><li><a class='restart-server' href='#'>Download</a></li><li class='divider'></li><li><a class='' href='#'>Delete</a></li></ul></div>"
          + "</td>";
      }

      var automaticRows = "";

      if (automaticBackups.length > 0) {
        for (var backupIndex in automaticBackups) {
          // create a table row for this backup and add it to the rows we'll put in the table body
          automaticRows += createBackupTableRow(automaticBackups[backupIndex]);
        }
      }

      $('#' + AUTOMATIC_ARCHIVES_TBODY_ID).html(automaticRows);

      var manualRows = "";

      if (manualBackups.length > 0) {
        for (var backupIndex in manualBackups) {
          // create a table row for this backup and add it to the rows we'll put in the table body
          manualRows += createBackupTableRow(manualBackups[backupIndex]);
        }
      }

      $('#' + MANUAL_ARCHIVES_TBODY_ID).html(manualRows);

      // tell bootstrap sortable to update for the new rows
      $.bootstrapSortable({ applyLast: true });

    }).fail(function(){
      // we've hit the very rare case where we couldn't load the list of backups from the domain server

      // set our backups to empty
      automaticBackups = [];
      manualBackups = [];

      // replace the content archives panel with a simple error message
      // stating that the user should reload the page
      $('#' + Settings.CONTENT_ARCHIVES_PANEL_ID + ' .panel-body').html(
        "<div class='form-group'>" +
        "<span class='help-block'>There was a problem loading your list of automatic and manual content archives. Please reload the page to try again.</span>" +
        "</div>"
      );

    }).always(function(){
      // toggle showing or hiding the tables depending on if they have entries
      $('#' + AUTOMATIC_ARCHIVES_TABLE_ID).toggle(automaticBackups.length > 0);
      $('#' + MANUAL_ARCHIVES_TABLE_ID).toggle(manualBackups.length > 0);
    });
  }

  // handle click on automatic content archive settings link
  $('body').on('click', '#' + AUTO_ARCHIVES_SETTINGS_LINK_ID, function(e) {
    if (Settings.pendingChanges > 0) {
      // don't follow the link right away, make sure the user knows they are about to leave
      // the page and lose changes
      e.preventDefault();

      var settingsLink = $(this).attr('href');

      swal({
        title: "Are you sure?",
        text: "You have pending changes to content settings that have not been saved. They will be lost if you leave the page to manage automatic content archive intervals.",
        type: "warning",
        showCancelButton: true,
        confirmButtonText: "Leave and Lose Pending Changes",
        closeOnConfirm: true
      },
      function () {
        // user wants to drop their changes, switch pages
        window.location =  settingsLink;
      });
    }
  });

  // handle click on manual archive creation button
  $('body').on('click', '#' + GENERATE_ARCHIVE_BUTTON_ID, function(e) {
    e.preventDefault();

    // show a sweet alert to ask the user to provide a name for their content archive
    swal({
      title: "Generate a Content Archive",
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

      // post the provided archive name to ask the server to kick off a manual backup
      $.ajax({
        type: 'POST',
        url: '/api/backup',
        data: {
          'name': inputValue
        }
      }).done(function(data) {
        // since we successfully setup a new content archive, reload the table of archives
        // which should show that this archive is pending creation
        reloadContentArchives();
      }).fail(function(jqXHR, textStatus, errorThrown) {

      });

      swal.close();
    });
  });

  Settings.extraGroupsAtIndex = Settings.extraContentGroupsAtIndex;

  Settings.afterReloadActions = function() {
    setupBackupUpload();
    setupContentArchives();

    // load the latest backups immediately
    reloadLatestBackups();
  };
});
