//
//  typedArraysunitTest.js
//  examples
//
//  Created by Clément Brisset on 7/7/14
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("Test.js");

test("ArrayBuffer", function(finished) {
     this.assertEquals(new ArrayBuffer().byteLength, 0, 'no length');
     
     this.assertEquals(typeof(new ArrayBuffer(0)), 'object', 'creation');
     this.assertEquals(typeof(new ArrayBuffer(1)), 'object', 'creation');
     this.assertEquals(typeof(new ArrayBuffer(123)), 'object', 'creation');
     
     this.assertEquals(new ArrayBuffer(123).byteLength, 123, 'length');
});