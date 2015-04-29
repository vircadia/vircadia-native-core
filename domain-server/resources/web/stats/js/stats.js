function qs(key) {
    key = key.replace(/[*+?^$.\[\]{}()|\\\/]/g, "\\$&"); // escape RegEx meta chars
    var match = location.search.match(new RegExp("[?&]"+key+"=([^&]+)(&|$)"));
    return match && decodeURIComponent(match[1].replace(/\+/g, " "));
}

$(document).ready(function(){
  // setup a function to grab the nodeStats
  function getNodeStats() {
    
    var uuid = qs("uuid");
    
    $.getJSON("/nodes/" + uuid + ".json", function(json){
      
      // update the table header with the right node type
      $('#stats-lead h3').html(json.node_type + " stats (" + uuid + ")");
      
      delete json.node_type;
      
      var stats = JsonHuman.format(json);

      $('#stats-container').html(stats);
    }).fail(function(data) {
      $('#stats-table td').each(function(){
        $(this).addClass('stale');
      });
    });    
  }
  
  // do the first GET on page load
  getNodeStats();
  // grab the new assignments JSON every second
  var getNodeStatsInterval = setInterval(getNodeStats, 1000);
});
