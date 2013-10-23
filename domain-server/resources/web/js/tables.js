$(document).ready(function(){
  // setup a function to grab the assignments
  function getAssignments() {
    $.getJSON("assignments.json", function (json) {
      assignedTableBody = "";
      
      $.each(json.fulfilled, function (type, data) {
        assignedTableBody += "<tr>";
        assignedTableBody += "<td>" + type + "</td>";
        assignedTableBody += "<td>" + data.UUID + "</td>";
        assignedTableBody += "<td>" + (data.pool ? data.pool : "N/A") + "</td>";
        assignedTableBody += "<td>" + "</td>";
        assignedTableBody += "<td>" + "</td>";
        assignedTableBody += "<td>" + "</td>";
        assignedTableBody += "</tr>";
      });
      
      $('#nodes-table tbody').html(assignedTableBody);
      
      queuedTableBody = "";
        
      $.each(json.queued, function (type, data) {
        queuedTableBody += "<tr>";
        queuedTableBody += "<td>" + type + "</td>";
        queuedTableBody += "<td>" + data.UUID + "</td>";
        queuedTableBody += "<td>" + (data.pool ? data.pool : "N/A") + "</td>";
        queuedTableBody += "</tr>";
      });
      
      $('#assignments-table tbody').html(queuedTableBody);
    });
  }
  
  // do the first GET on page load
  getAssignments();
  // grab the new assignments JSON every two seconds
  var getAssignmentsInterval = setInterval(getAssignments, 2000);
});
