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
  var url = window.location;

  // Will only work if string in href matches with location
  $('ul.nav a[href="'+ url +'"]').parent().addClass('active');

  // Will also work for relative and absolute hrefs
  $('ul.nav a').filter(function() {
      return this.href == url;
  }).parent().addClass('active');

  $('body').on('click', '#restart-server', function(e) {
    swal( {
      title: "Are you sure?",
      text: "This will restart your domain server, causing your domain to be briefly offline.",
      type: "warning",
      html: true,
      showCancelButton: true
    }, function() {
      $.get("/restart");
      showRestartModal();
    });
    return false;
  });

  var $contentDropdown = $('#content-settings-nav-dropdown');
  var $settingsDropdown = $('#domain-settings-nav-dropdown');

  // for pages that have the settings dropdowns
  if ($contentDropdown.length && $settingsDropdown.length) {
    // make a JSON request to get the dropdown menus for content and settings
    // we don't error handle here because the top level menu is still clickable and usables if this fails
    $.getJSON('/settings-menu-groups.json', function(data){
      function makeGroupDropdownElement(group, base) {
        var html_id = group.html_id ? group.html_id : group.name;
        return "<li class='setting-group'><a href='" + settingsGroupAnchor(base, html_id) + "'>" + group.label + "<span class='badge'></span></a></li>";
      }

      $.each(data.content_settings, function(index, group){
        if (index > 0) {
          $contentDropdown.append("<li role='separator' class='divider'></li>");
        }

        $contentDropdown.append(makeGroupDropdownElement(group, "/content/"));
      });

      $.each(data.domain_settings, function(index, group){
        if (index > 0) {
          $settingsDropdown.append("<li role='separator' class='divider'></li>");
        }

        $settingsDropdown.append(makeGroupDropdownElement(group, "/settings/"));

        // for domain settings, we add a dummy "Places" group that we fill
        // via the API - add it to the dropdown menu in the right spot
        // which is after "Metaverse / Networking"
        if (group.name == "metaverse") {
          $settingsDropdown.append("<li role='separator' class='divider'></li>");
          $settingsDropdown.append(makeGroupDropdownElement({ html_id: 'places', label: 'Places' }, "/settings/"));
        }
      });

      // append a link for the "Settings Backup" panel
      $settingsDropdown.append("<li role='separator' class='divider'></li>");
      $settingsDropdown.append(makeGroupDropdownElement({ html_id: 'settings_backup', label: 'Settings Backup'}, "/settings"));
    });
  }
});
