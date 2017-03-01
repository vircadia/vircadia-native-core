/* eslint-env node */
//  dummy lines to make sure exception line number is well below parent test script
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//


function myfunc() {
    throw new Error('exception on line 32 in myfunc');
}
module.exports = myfunc;
if (Script[module.filename] === 'throw') {
    myfunc();
}
