// Examples and understanding of console object. Include console methods like 
// info, log, debug, warn, error, exception, trace, clear, asserts, group, groupCollapsed, groupEnd, time, timeEnd.
// Useful in debugging and exclusively made for  JavaScript files.
// To view the logs click on Developer -> script logs [logs on debug window] and for text file logs go to Logs/log file.

main();

function main() {

    var someObject = { str: "Some text", id: 5 };
    var someValue = 5;

    // console.info examples
    console.info("[console.info] Hello World.");
    console.info(5 + 6);
    console.info(someObject.str);
    console.info(a = 2 * 6);
    console.info(someValue);
    console.info('someObject id ' + someObject.id);

    // console.log examples
    console.log("[console.log] Hello World");
    console.log(5 + 6);
    console.log(someObject.str);
    console.log(a = 2 * 6);
    console.log(someValue);
    console.log('someObject id ' + someObject.id);

    // console.debug examples
    console.debug("[console.debug] Hello World.");
    console.debug(5 + 6);
    console.debug(someObject.str);
    console.debug(a = 2 * 6);
    console.debug(someValue);
    console.debug('someObject id ' + someObject.id);

    // console.warn examples
    console.warn("[console.warn] This is warning message.");
    console.warn(5 + 6);
    console.warn(someObject.str);
    console.warn(a = 2 * 6);
    console.warn(someValue);
    console.warn('someObject id ' + someObject.id);

    // console.error examples
    console.error('An error occurred!');
    console.error('An error occurred! ', 'Value = ', someValue);
    console.error('An error occurred! ' + 'Value = ' + someValue);

    // console.exception examples
    console.exception('An exception occurred!');
    console.exception('An exception occurred! ', 'Value = ', someValue);
    console.exception('An exception occurred! ' + 'Value = ' + someValue);	 	

    // console.trace examples
    function fooA() {
        function fooB() {
            function fooC() {
                console.trace();
            }
            fooC();
        }
        fooB();
    }
    fooA();

   //console.asserts() examples
    var valA = 1, valB = "1";
    console.asserts(valA === valB, "Value A doesn't equal to B");
    console.asserts(valA === valB);
    console.asserts(5 === 5, "5 equals to 5");
    console.asserts(5 === 5);
    console.asserts(5 > 6, "5 is not greater than 6");
   
    //console.group() examples.
    console.group("Group 1");   
    console.log("Sentence 1");
    console.log("Sentence 2");
    console.group("Group 2");
    console.log("Sentence 3");
    console.log("Sentence 4");
    console.groupCollapsed("Group 3");
    console.log("Sentence 5");
    console.log("Sentence 6");
    console.groupEnd();
    console.log("Sentence 7");
    console.groupEnd();
    console.log("Sentence 8");
    console.groupEnd();
    console.log("Sentence 9");

    //console.time(),console.timeEnd() examples
    console.time('Timer1');
    // Do some process
    sleep(1000);
    console.timeEnd('Timer1');
    
    // use console.clear() to clean Debug Window logs
    // console.clear();
}

function sleep(milliseconds) {
    var start = new Date().getTime();
    for (var i = 0; i < 1e7; i++) {
        if ((new Date().getTime() - start) > milliseconds){
            break;
        }
    }
}
