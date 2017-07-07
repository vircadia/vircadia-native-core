$(document).ready(function(){
  // setup the underscore templates
  var nodeTemplate = _.template($('#nodes-template').html());
  var queuedTemplate = _.template($('#queued-template').html());

  // setup a function to grab the assignments
  function getNodesAndAssignments() {
    $.getJSON("nodes.json", function(json){

      json.nodes.sort(function(a, b){
          if (a.type === b.type) {
            if (a.uptime < b.uptime) {
              return 1;
            } else if (a.uptime > b.uptime) {
              return -1;
            } else {
              return 0;
            }
          }

          if (a.type === "agent" && b.type !== "agent") {
            return 1;
          } else if (b.type === "agent" && a.type !== "agent") {
            return -1;
          }

          if (a.type > b.type) {
            return 1;
          }

          if (a.type < b.type) {
            return -1;
          }
      });

      $('#nodes-table tbody').html(nodeTemplate(json));
    }).fail(function(jqXHR, textStatus, errorThrown) {
      // we assume a 401 means the DS has restarted
      // and no longer has our OAuth produced uuid
      // so just reload and re-auth
      if (jqXHR.status == 401) {
        location.reload();
      }
    });

    $.getJSON("assignments.json", function(json){
      $('#assignments-table tbody').html(queuedTemplate(json));
    }).fail(function(jqXHR, textStatus, errorThrown) {
      // we assume a 401 means the DS has restarted
      // and no longer has our OAuth produced uuid
      // so just reload and re-auth
      if (jqXHR.status == 401) {
        location.reload();
      }
    });
  }

  // do the first GET on page load
  getNodesAndAssignments();
  // grab the new assignments JSON every two seconds
  var getNodesAndAssignmentsInterval = setInterval(getNodesAndAssignments, 2000);

  // hook the node delete to the X button
  $(document.body).on('click', '.glyphicon-remove', function(){
    // fire off a delete for this node
    $.ajax({
        url: "/nodes/" + $(this).data('uuid'),
        type: 'DELETE',
        success: function(result) {
          console.log("Succesful request to delete node.");
        }
    });
  });

  $(document.body).on('click', '#kill-all-btn', function() {
    var confirmed_kill = confirm("Are you sure?");

    if (confirmed_kill == true) {
      $.ajax({
        url: "/nodes/",
        type: 'DELETE',
        success: function(result) {
          console.log("Successful request to delete all nodes.");
        }
      });
    }
  });
});
