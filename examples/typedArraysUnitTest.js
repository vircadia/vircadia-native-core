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

test('ArrayBuffer', function(finished) {
     this.assertEquals(new ArrayBuffer(0).byteLength, 0, 'no length');
     
     this.assertEquals(typeof(new ArrayBuffer(0)), 'object', 'creation');
     this.assertEquals(typeof(new ArrayBuffer(1)), 'object', 'creation');
     this.assertEquals(typeof(new ArrayBuffer(123)), 'object', 'creation');
     
     this.assertEquals(new ArrayBuffer(123).byteLength, 123, 'length');

  	// raises(function () { return new ArrayBuffer(-1); }, RangeError, 'negative length');
  	// raises(function () { return new ArrayBuffer(0x80000000); }, RangeError, 'absurd length');
});

test('DataView constructors', function (finished) {
  var d = new DataView(new ArrayBuffer(8));

  d.setUint32(0, 0x12345678);
  this.assertEquals(d.getUint32(0), 0x12345678, 'big endian/big endian');

  d.setUint32(0, 0x12345678, true);
  this.assertEquals(d.getUint32(0, true), 0x12345678, 'little endian/little endian');

  d.setUint32(0, 0x12345678, true);
  this.assertEquals(d.getUint32(0), 0x78563412, 'little endian/big endian');

  d.setUint32(0, 0x12345678);
  this.assertEquals(d.getUint32(0, true), 0x78563412, 'big endian/little endian');

  // raises(function () { return new DataView({}); }, 'non-ArrayBuffer argument');
  // raises(function () { return new DataView("bogus"); }, TypeError, 'non-ArrayBuffer argument');
});


test('ArrayBufferView', function () {
  var ab = new ArrayBuffer(48);
  var i32 = new Int32Array(ab, 16);
  i32.set([1, 2, 3, 4, 5, 6, 7, 8]);

  this.assertEquals(i32.buffer, ab, 'ArrayBuffers equal');
  this.assertEquals(i32.byteOffset, 16, 'byteOffset');
  this.assertEquals(i32.byteLength, 32, 'byteLength');

  var da = new DataView(i32.buffer, 8);
  this.assertEquals(da.buffer, ab, 'DataView: ArrayBuffers equal');
  this.assertEquals(da.byteOffset, 8, 'DataView: byteOffset');
  this.assertEquals(da.byteLength, 40, 'DataView: byteLength');
});

test('TypedArrays', function () {
  var a;

  this.assertEquals(Int8Array.BYTES_PER_ELEMENT, 1, 'Int8Array.BYTES_PER_ELEMENT');
  a = new Int8Array([1, 2, 3, 4, 5, 6, 7, 8]);
  this.assertEquals(a.BYTES_PER_ELEMENT, 1, 'int8Array.BYTES_PER_ELEMENT');
  this.assertEquals(a.byteOffset, 0, 'int8Array.byteOffset');
  this.assertEquals(a.byteLength, 8, 'int8Array.byteLength');

  this.assertEquals(Uint8Array.BYTES_PER_ELEMENT, 1, 'Uint8Array.BYTES_PER_ELEMENT');
  a = new Uint8Array([1, 2, 3, 4, 5, 6, 7, 8]);
  this.assertEquals(a.BYTES_PER_ELEMENT, 1, 'uint8Array.BYTES_PER_ELEMENT');
  this.assertEquals(a.byteOffset, 0, 'uint8Array.byteOffset');
  this.assertEquals(a.byteLength, 8, 'uint8Array.byteLength');

  this.assertEquals(Int16Array.BYTES_PER_ELEMENT, 2, 'Int16Array.BYTES_PER_ELEMENT');
  a = new Int16Array([1, 2, 3, 4, 5, 6, 7, 8]);
  this.assertEquals(a.BYTES_PER_ELEMENT, 2, 'int16Array.BYTES_PER_ELEMENT');
  this.assertEquals(a.byteOffset, 0, 'int16Array.byteOffset');
  this.assertEquals(a.byteLength, 16, 'int16Array.byteLength');

  this.assertEquals(Uint16Array.BYTES_PER_ELEMENT, 2, 'Uint16Array.BYTES_PER_ELEMENT');
  a = new Uint16Array([1, 2, 3, 4, 5, 6, 7, 8]);
  this.assertEquals(a.BYTES_PER_ELEMENT, 2, 'uint16Array.BYTES_PER_ELEMENT');
  this.assertEquals(a.byteOffset, 0, 'uint16Array.byteOffset');
  this.assertEquals(a.byteLength, 16, 'uint16Array.byteLength');

  this.assertEquals(Int32Array.BYTES_PER_ELEMENT, 4, 'Int32Array.BYTES_PER_ELEMENT');
  a = new Int32Array([1, 2, 3, 4, 5, 6, 7, 8]);
  this.assertEquals(a.BYTES_PER_ELEMENT, 4, 'int32Array.BYTES_PER_ELEMENT');
  this.assertEquals(a.byteOffset, 0, 'int32Array.byteOffset');
  this.assertEquals(a.byteLength, 32, 'int32Array.byteLength');

  this.assertEquals(Uint32Array.BYTES_PER_ELEMENT, 4, 'Uint32Array.BYTES_PER_ELEMENT');
  a = new Uint32Array([1, 2, 3, 4, 5, 6, 7, 8]);
  this.assertEquals(a.BYTES_PER_ELEMENT, 4, 'uint32Array.BYTES_PER_ELEMENT');
  this.assertEquals(a.byteOffset, 0, 'uint32Array.byteOffset');
  this.assertEquals(a.byteLength, 32, 'uint32Array.byteLength');

  this.assertEquals(Float32Array.BYTES_PER_ELEMENT, 4, 'Float32Array.BYTES_PER_ELEMENT');
  a = new Float32Array([1, 2, 3, 4, 5, 6, 7, 8]);
  this.assertEquals(a.BYTES_PER_ELEMENT, 4, 'float32Array.BYTES_PER_ELEMENT');
  this.assertEquals(a.byteOffset, 0, 'float32Array.byteOffset');
  this.assertEquals(a.byteLength, 32, 'float32Array.byteLength');

  this.assertEquals(Float64Array.BYTES_PER_ELEMENT, 8, 'Float64Array.BYTES_PER_ELEMENT');
  a = new Float64Array([1, 2, 3, 4, 5, 6, 7, 8]);
  this.assertEquals(a.BYTES_PER_ELEMENT, 8, 'float64Array.BYTES_PER_ELEMENT');
  this.assertEquals(a.byteOffset, 0, 'float64Array.byteOffset');
  this.assertEquals(a.byteLength, 64, 'float64Array.byteLength');
});


test('typed array constructors', function () {
  this.arrayEqual(new Int8Array({ length: 3 }), [0, 0, 0], 'array equal -1');
  var rawbuf = (new Uint8Array([0, 1, 2, 3, 4, 5, 6, 7])).buffer;

  var int8 = new Int8Array();
  this.assertEquals(int8.length, 0, 'no args 0');
  // raises(function () { return new Int8Array(-1); }, /*Range*/Error, 'bogus length');
  // raises(function () { return new Int8Array(0x80000000); }, /*Range*/Error, 'bogus length');

  int8 = new Int8Array(4);
  this.assertEquals(int8.BYTES_PER_ELEMENT, 1);
  this.assertEquals(int8.length, 4, 'length 1');
  this.assertEquals(int8.byteLength, 4, 'length 2');
  this.assertEquals(int8.byteOffset, 0, 'length 3');
  this.assertEquals(int8.get(-1), undefined, 'length, out of bounds 4');
  this.assertEquals(int8.get(4), undefined, 'length, out of bounds 5');

  int8 = new Int8Array([1, 2, 3, 4, 5, 6]);
  this.assertEquals(int8.length, 6, 'array 6');
  this.assertEquals(int8.byteLength, 6, 'array 7');
  this.assertEquals(int8.byteOffset, 0, 'array 8');
  this.assertEquals(int8.get(3), 4, 'array 9');
  this.assertEquals(int8.get(-1), undefined, 'array, out of bounds  10');
  this.assertEquals(int8.get(6), undefined, 'array, out of bounds 11');

  int8 = new Int8Array(rawbuf);
  this.assertEquals(int8.length, 8, 'buffer 12');
  this.assertEquals(int8.byteLength, 8, 'buffer 13');
  this.assertEquals(int8.byteOffset, 0, 'buffer 14');
  this.assertEquals(int8.get(7), 7, 'buffer 15');
  int8.set([111]);
  this.assertEquals(int8.get(0), 111, 'buffer 16');
  this.assertEquals(int8.get(-1), undefined, 'buffer, out of bounds 17');
  this.assertEquals(int8.get(8), undefined, 'buffer, out of bounds 18');

  int8 = new Int8Array(rawbuf, 2);
  this.assertEquals(int8.length, 6, 'buffer, byteOffset 19');
  this.assertEquals(int8.byteLength, 6, 'buffer, byteOffset 20');
  this.assertEquals(int8.byteOffset, 2, 'buffer, byteOffset 21');
  this.assertEquals(int8.get(5), 7, 'buffer, byteOffset 22');
  int8.set([112]);
  this.assertEquals(int8.get(0), 112, 'buffer 23');
  this.assertEquals(int8.get(-1), undefined, 'buffer, byteOffset, out of bounds 24');
  this.assertEquals(int8.get(6), undefined, 'buffer, byteOffset, out of bounds 25');
  
  int8 = new Int8Array(rawbuf, 8);
  this.assertEquals(int8.length, 0, 'buffer, byteOffset 26');

  // raises(function () { return new Int8Array(rawbuf, -1); }, 'invalid byteOffset 27');
  // raises(function () { return new Int8Array(rawbuf, 9); }, 'invalid byteOffset 28');
  // raises(function () { return new Int8Array(rawbuf, -1); }, 'invalid byteOffset 29');
  // raises(function () { return new Int32Array(rawbuf, 5); }, 'invalid byteOffset 30');

  int8 = new Int8Array(rawbuf, 2, 4);
  this.assertEquals(int8.length, 4, 'buffer, byteOffset, length 31');
  this.assertEquals(int8.byteLength, 4, 'buffer, byteOffset, length 32');
  this.assertEquals(int8.byteOffset, 2, 'buffer, byteOffset, length 33');
  this.assertEquals(int8.get(3), 5, 'buffer, byteOffset, length 34');
  int8.set([113]);
  this.assertEquals(int8.get(0), 113, 'buffer, byteOffset, length 35');
  this.assertEquals(int8.get(-1), undefined, 'buffer, byteOffset, length, out of bounds 36');
  this.assertEquals(int8.get(4), undefined, 'buffer, byteOffset, length, out of bounds 37');

  // raises(function () { return new Int8Array(rawbuf, 0, 9); }, 'invalid byteOffset+length');
  // raises(function () { return new Int8Array(rawbuf, 8, 1); }, 'invalid byteOffset+length');
  // raises(function () { return new Int8Array(rawbuf, 9, -1); }, 'invalid byteOffset+length');
});

test('TypedArray clone constructor', function () {
  var src = new Int32Array([1, 2, 3, 4, 5, 6, 7, 8]);
  var dst = new Int32Array(src);
  this.arrayEqual(dst, [1, 2, 3, 4, 5, 6, 7, 8], '1');
  src.set([99]);
  this.arrayEqual(src, [99, 2, 3, 4, 5, 6, 7, 8], '2');
  this.arrayEqual(dst, [1, 2, 3, 4, 5, 6, 7, 8], '3');
});


test('conversions', function () {
  var uint8 = new Uint8Array([1, 2, 3, 4]),
      uint16 = new Uint16Array(uint8.buffer),
      uint32 = new Uint32Array(uint8.buffer);

  // Note: can't probe individual bytes without endianness awareness
  this.arrayEqual(uint8, [1, 2, 3, 4]);
  uint16.set([0xffff]);
  this.arrayEqual(uint8, [0xff, 0xff, 3, 4]);
  uint16.set([0xeeee], 1);
  this.arrayEqual(uint8, [0xff, 0xff, 0xee, 0xee]);
  uint32.set([0x11111111]);
  this.assertEquals(uint16.get(0), 0x1111);
  this.assertEquals(uint16.get(1), 0x1111);
  this.arrayEqual(uint8, [0x11, 0x11, 0x11, 0x11]);
});


test('signed/unsigned conversions', function () {

  var int8 = new Int8Array(1), uint8 = new Uint8Array(int8.buffer);
  uint8.set([123]);
  this.assertEquals(int8.get(0), 123, 'int8/uint8');
  uint8.set([161]);
  this.assertEquals(int8.get(0), -95, 'int8/uint8');
  int8.set([-120]);
  this.assertEquals(uint8.get(0), 136, 'uint8/int8');
  int8.set([-1]);
  this.assertEquals(uint8.get(0), 0xff, 'uint8/int8');

  var int16 = new Int16Array(1), uint16 = new Uint16Array(int16.buffer);
  uint16.set([3210]);
  this.assertEquals(int16.get(0), 3210, 'int16/uint16');
  uint16.set([49232]);
  this.assertEquals(int16.get(0), -16304, 'int16/uint16');
  int16.set([-16384]);
  this.assertEquals(uint16.get(0), 49152, 'uint16/int16');
  int16.set([-1]);
  this.assertEquals(uint16.get(0), 0xffff, 'uint16/int16');

  var int32 = new Int32Array(1), uint32 = new Uint32Array(int32.buffer);
  uint32.set([0x80706050]);
  this.assertEquals(int32.get(0), -2140118960, 'int32/uint32');
  int32.set([-2023406815]);
  this.assertEquals(uint32.get(0), 0x87654321, 'uint32/int32');
  int32.set([-1]);
  this.assertEquals(uint32.get(0), 0xffffffff, 'uint32/int32');
});


test('Int32Array round trips', function () {
  var i32 = new Int32Array([0]);
  var data = [
    0,
    1,
      -1,
    123,
      -456,
    0x80000000 >> 0,
    0x7fffffff >> 0,
    0x12345678 >> 0,
    0x87654321 >> 0
  ];

  for (var i = 0; i < data.length; i += 1) {
    var datum = data[i];
    i32.set([datum]);
    this.assertEquals(datum, i32.get(0), String(datum));
  }
});


test('Int16Array round trips', function () {
  var i16 = new Int16Array([0]);
  var data = [
    0,
    1,
      -1,
    123,
      -456,
    0xffff8000 >> 0,
    0x00007fff >> 0,
    0x00001234 >> 0,
    0xffff8765 >> 0
  ];

  for (var i = 0; i < data.length; i += 1) {
    var datum = data[i];
    i16.set([datum]);
    this.assertEquals(datum, i16.get(0), String(datum));
  }
});


test('Int8Array round trips', function () {
  var i8 = new Int8Array([0]);
  var data = [
    0,
    1,
      -1,
    123,
      -45,
    0xffffff80 >> 0,
    0x0000007f >> 0,
    0x00000012 >> 0,
    0xffffff87 >> 0
  ];

  for (var i = 0; i < data.length; i += 1) {
    var datum = data[i];
    i8.set([datum]);
    this.assertEquals(datum, i8.get(0), String(datum));
  }
});

test('TypedArray setting', function () {

  var a = new Int32Array([1, 2, 3, 4, 5]);
  var b = new Int32Array(5);
  b.set(a);
  this.arrayEqual(b, [1, 2, 3, 4, 5], '1');
  // raises(function () { b.set(a, 1); });

  b.set(new Int32Array([99, 98]), 2);
  this.arrayEqual(b, [1, 2, 99, 98, 5], '2');

  b.set(new Int32Array([99, 98, 97]), 2);
  this.arrayEqual(b, [1, 2, 99, 98, 97], '3');

  // raises(function () { b.set(new Int32Array([99, 98, 97, 96]), 2); });
  // raises(function () { b.set([101, 102, 103, 104], 4); });

  //  ab = [ 0, 1, 2, 3, 4, 5, 6, 7 ]
  //  a1 = [ ^, ^, ^, ^, ^, ^, ^, ^ ]
  //  a2 =             [ ^, ^, ^, ^ ]
  var ab = new ArrayBuffer(8);
  var a1 = new Uint8Array(ab);
  for (var i = 0; i < a1.length; i += 1) { a1.set([i], i); }
  var a2 = new Uint8Array(ab, 4);
  a1.set(a2, 2);
  this.arrayEqual(a1, [0, 1, 4, 5, 6, 7, 6, 7]);
  this.arrayEqual(a2, [6, 7, 6, 7]);
});


test('TypedArray.subarray', function () {

  var a = new Int32Array([1, 2, 3, 4, 5]);
  this.arrayEqual(a.subarray(3), [4, 5]);
  this.arrayEqual(a.subarray(1, 3), [2, 3]);
  this.arrayEqual(a.subarray(-3), [3, 4, 5]);
  this.arrayEqual(a.subarray(-3, -1), [3, 4]);
  this.arrayEqual(a.subarray(3, 2), []);
  this.arrayEqual(a.subarray(-2, -3), []);
  this.arrayEqual(a.subarray(4, 1), []);
  this.arrayEqual(a.subarray(-1, -4), []);
  this.arrayEqual(a.subarray(1).subarray(1), [3, 4, 5]);
  this.arrayEqual(a.subarray(1, 4).subarray(1, 2), [3]);
});


test('DataView constructors', function () {

  var d = new DataView(new ArrayBuffer(8));

  d.setUint32(0, 0x12345678);
  this.assertEquals(d.getUint32(0), 0x12345678, 'big endian/big endian');

  d.setUint32(0, 0x12345678, true);
  this.assertEquals(d.getUint32(0, true), 0x12345678, 'little endian/little endian');

  d.setUint32(0, 0x12345678, true);
  this.assertEquals(d.getUint32(0), 0x78563412, 'little endian/big endian');

  d.setUint32(0, 0x12345678);
  this.assertEquals(d.getUint32(0, true), 0x78563412, 'big endian/little endian');

  // Chrome allows no arguments, throws if non-ArrayBuffer
  //stricterEqual(new DataView().buffer.byteLength, 0, 'no arguments');

  // Safari (iOS 5) does not
  //raises(function () { return new DataView(); }, TypeError, 'no arguments');

  // Chrome raises TypeError, Safari iOS5 raises isDOMException(INDEX_SIZE_ERR)
  // raises(function () { return new DataView({}); }, 'non-ArrayBuffer argument');

  // raises(function () { return new DataView("bogus"); }, TypeError, 'non-ArrayBuffer argument');
});




