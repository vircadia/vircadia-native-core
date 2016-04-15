
Script.include("../libraries/jasmine/jasmine.js");
Script.include("../libraries/jasmine/hifi-boot.js");

describe("Avatar", function() {

    beforeEach(function() {
        print("beforeEach");
    });

    it("test one", function() {
        print("test one!");
        expect(10).toEqual(11);
        throw "wtf";
    });
});

jasmine.getEnv().execute();

