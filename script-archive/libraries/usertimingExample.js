Script.include('usertiming.js');
var timing = loadUserTiming();
//set a mark
timing.performance.mark('firstMark');

//do something that takes time -- we're just going to set a timeout here as an example

Script.setTimeout(function() {
    //and set another mark
    timing.performance.mark('secondMark');
    
    //measure time between marks (first parameter is a name for the measurement)
    timing.performance.measure('howlong', 'firstMark', 'secondMark');
    
    //you can also get the marks by changing the type 
    var measures = timing.performance.getEntriesByType('measure');
    print('measures:::' + JSON.stringify(measures))
}, 1000)
