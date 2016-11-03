var data = "this is some data";
var extension = "txt";
var uploadedFile;

Assets.uploadData(data, function (url) {
    print("data uploaded to:" + url);
    uploadedFile = url;
    Assets.downloadData(url, function (data) {
        print("data downloaded from:" + url + " the data is:" + data);
    });
});
