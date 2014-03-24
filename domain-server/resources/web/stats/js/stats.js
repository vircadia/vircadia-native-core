function qs(key) {
    key = key.replace(/[*+?^$.\[\]{}()|\\\/]/g, "\\$&"); // escape RegEx meta chars
    var match = location.search.match(new RegExp("[?&]"+key+"=([^&]+)(&|$)"));
    return match && decodeURIComponent(match[1].replace(/\+/g, " "));
}

$(document).ready(function(){
  // setup a function to grab the nodeStats
  function getNodeStats() {
    
    var uuid = qs("uuid");
    
    var statsTableBody = "";
    
    $.getJSON("/nodes/" + uuid + ".json", function(json){
      $.each(json, function (key, value) {
        statsTableBody += "<tr>";
        statsTableBody += "<td>" + key + "</td>";
        statsTableBody += "<td>" + value + "</td>";
        statsTableBody += "</tr>";
      });
      
      $('#stats-table tbody').html(statsTableBody);
    });
  }
  
  // do the first GET on page load
  getNodeStats();
  // grab the new assignments JSON every second
  var getNodeStatsInterval = setInterval(getNodeStats, 1000);
});
