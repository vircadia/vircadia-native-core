//
//  index.js
//  examples
//
//  Created by Eric Levin on 11/10/2015.
//  Copyright 2013 High Fidelity, Inc.
//
//  This is a simple REST API that allows an interface client script to get a list of files paths from an S3 bucket.
//  To change your bucket, modify line 34 to your desired bucket.
//  Please refer to  http://docs.aws.amazon.com/AWSJavaScriptSDK/guide/node-configuring.html
//  for instructions on how to configure the SDK to work with your bucket.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

var express = require('express');
var app = express();
var AWS = require('aws-sdk');
var url = require('url');
var querystring = require('querystring');
var _ = require('underscore');

AWS.config.update({
    region: "us-east-1"
});

var s3 = new AWS.S3();

app.set('port', (process.env.PORT || 5000));

app.get('/', function(req, res) {
    var urlParts = url.parse(req.url)
    var query = querystring.parse(urlParts.query);

    var params = {
        Bucket: "hifi-public",
        Marker: query.assetDir,
        MaxKeys: 10
    };
    s3.listObjects(params, function(err, data) {
        if (err) {
            console.log(err, err.stack);
            res.send("ERROR")
        } else {
            var keys = _.pluck(data.Contents, 'Key')
            res.send({
                urls: keys
            });
        }
    });
});


app.listen(app.get('port'), function() {
    console.log('Node app is running on port', app.get('port'));
})

