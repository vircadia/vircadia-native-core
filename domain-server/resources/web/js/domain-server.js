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

function settingsGroupAnchor(base, html_id) {
  return base + "#" + html_id + "_group"
}

$(document).ready(function(){
  var url = location.protocol + '//' + location.host+location.pathname;

  // Will only work if string in href matches with location
  $('ul.nav a[href="'+ url +'"]').parent().addClass('active');

  // Will also work for relative and absolute hrefs
  $('ul.nav a').filter(function() {
      return this.href == url;
  }).parent().addClass('active');

  $('body').on('click', '#restart-server', function(e) {
    swalAreYouSure(
      "This will restart your domain server, causing your domain to be briefly offline.",
      "Restart",
      function() {
        swal.close();
        $.get("/restart");
        showRestartModal();
      }
    )
    return false;
  });

  var $contentDropdown = $('#content-settings-nav-dropdown');
  var $settingsDropdown = $('#domain-settings-nav-dropdown');

  // define extra groups to add to setting panels, with their splice index
  Settings.extraContentGroupsAtIndex = {
    0: {
      html_id: Settings.INSTALLED_CONTENT,
      label: 'Installed Content'
    },
    1: {
      html_id: Settings.CONTENT_ARCHIVES_PANEL_ID,
      label: 'Content Archives'
    },
    2: {
      html_id: Settings.UPLOAD_CONTENT_BACKUP_PANEL_ID,
      label: 'Upload Content'
    }
  };

  Settings.extraContentGroupsAtEnd = [];

  Settings.extraDomainGroupsAtIndex = {
    1: {
      html_id: 'places',
      label: 'Places'
    }
  }

  Settings.extraDomainGroupsAtEnd = [
    {
      html_id: 'settings_backup',
      label: 'Settings Backup / Restore'
    }
  ]

  // for pages that have the settings dropdowns
  if ($contentDropdown.length && $settingsDropdown.length) {
    // make a JSON request to get the dropdown menus for content and settings
    // we don't error handle here because the top level menu is still clickable and usables if this fails
    $.getJSON('/settings-menu-groups.json', function(data){
      function makeGroupDropdownElement(group, base) {
        var html_id = group.html_id ? group.html_id : group.name;
        return "<li class='setting-group'><a href='" + settingsGroupAnchor(base, html_id) + "'>" + group.label + "<span class='badge'></span></a></li>";
      }

      // add the dummy settings groups that get populated via JS
      for (var spliceIndex in Settings.extraContentGroupsAtIndex) {
        data.content_settings.splice(spliceIndex, 0, Settings.extraContentGroupsAtIndex[spliceIndex]);
      }

      for (var endIndex in Settings.extraContentGroupsAtEnd) {
        data.content_settings.push(Settings.extraContentGroupsAtEnd[endIndex]);
      }

      $.each(data.content_settings, function(index, group){
        if (index > 0) {
          $contentDropdown.append("<li role='separator' class='divider'></li>");
        }

        $contentDropdown.append(makeGroupDropdownElement(group, "/content/"));
      });

      // add the dummy settings groups that get populated via JS
      for (var spliceIndex in Settings.extraDomainGroupsAtIndex) {
        data.domain_settings.splice(spliceIndex, 0, Settings.extraDomainGroupsAtIndex[spliceIndex]);
      }

      for (var endIndex in Settings.extraDomainGroupsAtEnd) {
        data.domain_settings.push(Settings.extraDomainGroupsAtEnd[endIndex]);
      }

      $.each(data.domain_settings, function(index, group){
        if (index > 0) {
          $settingsDropdown.append("<li role='separator' class='divider'></li>");
        }

        $settingsDropdown.append(makeGroupDropdownElement(group, "/settings/"));
      });
    });
  }
});
