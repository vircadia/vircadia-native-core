$(document).ready(function(){

  var currentHighchart;

  // setup a function to grab the nodeStats
  function getNodeStats() {

    var uuid = qs("uuid");

    $.getJSON("/nodes/" + uuid + ".json", function(json){

      // update the table header with the right node type
      $('#stats-lead h3').html(json.node_type + " stats (" + uuid + ")");

      delete json.node_type;

      var stats = JsonHuman.format(json);

      $('#stats-container').html(stats);

      // add the clickable class to anything that looks like a number
      $('.jh-value span').each(function(val){
        console.log(val);
        if (!isNaN($(this).text())) {
          // this looks like a number - give it the clickable class so we can get graphs for it
          $(this).addClass('graphable-stat');
        }
      });

      if (currentHighchart) {
        // get the current time to set with the point
        var x = (new Date()).getTime();

        // get the last value using underscore-keypath
        var y = Number(_(json).valueForKeyPath(graphKeypath));

        // start shifting the chart once we hit 20 data points
        var shift = currentHighchart.series[0].data.length > 20;
        currentHighchart.series[0].addPoint([x, y], true, shift);
      }
    }).fail(function(data) {
      $('#stats-container th').each(function(){
        $(this).addClass('stale');
      });
    });
  }

  // do the first GET on page load
  getNodeStats();
  // grab the new assignments JSON every second
  var getNodeStatsInterval = setInterval(getNodeStats, 1000);

  var graphKeypath = "";

  // set the global Highcharts option
  Highcharts.setOptions({
    global: {
      useUTC: false
    }
  });

  // add a function to help create the graph modal
  function createGraphModal() {
    var chartModal = bootbox.dialog({
      title: graphKeypath,
      message: "<div id='highchart-container'></div>",
      buttons: {},
      className: 'highchart-modal'
    });

    chartModal.on('hidden.bs.modal', function(e) {
      currentHighchart.destroy();
      currentHighchart = null;
    });

    currentHighchart = new Highcharts.Chart({
      chart: {
        renderTo: 'highchart-container',
        defaultSeriesType: 'line'
      },
      title: {
        text: ''
      },
      xAxis: {
        type: 'datetime',
        tickPixelInterval: 150
      },
      yAxis: {
        title: {
          text: 'Value'
        }
      },
      legend: {
        enabled: false
      },
      series: [{
        name: graphKeypath,
        data: []
      }]
    });
  }

  // handle clicks on numerical values - this lets the user show a line graph in a modal
  $('#stats-container').on('click', '.graphable-stat', function(){
    graphKeypath = $(this).data('keypath');

    // setup the new graph modal
    createGraphModal();
  });
});
