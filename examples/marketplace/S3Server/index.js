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


//ozan/3d_marketplace