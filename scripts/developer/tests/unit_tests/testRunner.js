// Include testing library
Script.include('../../libraries/jasmine/jasmine.js');
Script.include('../../libraries/jasmine/hifi-boot.js')

// Include unit tests
// FIXME: Figure out why jasmine done() is not working.
// Script.include('avatarUnitTests.js');
Script.include('bindUnitTest.js');
Script.include('entityUnitTests.js');

// Run the tests
jasmine.getEnv().execute();
Script.stop();
