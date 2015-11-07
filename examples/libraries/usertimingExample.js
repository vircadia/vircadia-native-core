Script.include('usertiming.js');
var timing = loadUserTiming();
//set a mark
timing.performance.mark('mark_fully_loaded')

var count = 0;
Script.setTimeout(function() {
    //do something that takes time
    //and set another mark
    timing.performance.mark('mark_fully_loaded2')
        //measure time between marks (first parameter is a name for the measurement)
    timing.performance.measure('howlong', 'mark_fully_loaded', 'mark_fully_loaded2');
    //you can also get the marks, but the measure is usually what you're looking for
    var items = timing.performance.getEntriesByType('measure');
    print('items is:::' + JSON.stringify(items))
}, 1000)