$(document).ready(function(){
  // setup a function to grab the assignments
  function getNodesAndAssignments() {
    $.getJSON("nodes.json", function(json){
      nodesTableBody = "";
      
      $.each(json.nodes, function (uuid, data) {
        nodesTableBody += "<tr>";
        nodesTableBody += "<td>" + data.type + "</td>";
        nodesTableBody += "<td><a href='stats/?uuid=" + uuid + "'>" + uuid + "</a></td>";
        nodesTableBody += "<td>" + (data.pool ? data.pool : "") + "</td>";
        nodesTableBody += "<td>" + data.public.ip + "<span class='port'>:" + data.public.port + "</span></td>";
        nodesTableBody += "<td>" + data.local.ip + "<span class='port'>:" + data.local.port + "</span></td>";
        nodesTableBody += "<td><span class='glyphicon glyphicon-remove' data-uuid=" + uuid + "></span></td>";
        nodesTableBody += "</tr>";
      });
      
      $('#nodes-table tbody').html(nodesTableBody);
    });
    
    $.getJSON("assignments.json", function(json){      
      queuedTableBody = "";
        
      $.each(json.queued, function (uuid, data) {
        queuedTableBody += "<tr>";
        queuedTableBody += "<td>" + data.type + "</td>";
        queuedTableBody += "<td>" + uuid + "</td>";
        queuedTableBody += "<td>" + (data.pool ? data.pool : "") + "</td>";
        queuedTableBody += "</tr>";
      });
      
      $('#assignments-table tbody').html(queuedTableBody);
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
