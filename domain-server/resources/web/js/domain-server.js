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
});