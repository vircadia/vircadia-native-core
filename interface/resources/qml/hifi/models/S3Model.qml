import QtQuick 2.0
import QtQuick.XmlListModel 2.0

//http://s3.amazonaws.com/hifi-public?prefix=models/attachments
/*
<?xml version="1.0" encoding="UTF-8"?>
<ListBucketResult xmlns="http://s3.amazonaws.com/doc/2006-03-01/">
  <Contents>
    <Key>models/attachments/guitar.fst</Key>
    <LastModified>2015-11-10T00:28:22.000Z</LastModified>
    <ETag>&quot;236c00c4802ba9c2605cabd5601d138e&quot;</ETag>
    <Size>2992</Size>
    <StorageClass>STANDARD</StorageClass>
  </Contents>
</ListBucketResult>
*/

// FIXME how to deal with truncated results?  Store the marker?
XmlListModel {
    id: xmlModel
    property string prefix: "models/attachments/"
    property string extension: "fst"
    property string filter;

    readonly property string realPrefix: prefix.match('.*/$') ? prefix : (prefix + "/")
    readonly property string nameRegex: realPrefix + (filter ? (".*" + filter) :  "") + ".*\." + extension
    readonly property string nameQuery: "Key/substring-before(substring-after(string(), '" + prefix + "'), '." + extension + "')"
    readonly property string baseUrl: "https://cdn-1.vircadia.com/eu-c-1/vircadia-public"

    // FIXME need to urlencode prefix?
    source: baseUrl + "?prefix=" + realPrefix
    query: "/ListBucketResult/Contents[matches(Key, '" + nameRegex + "')]"
    namespaceDeclarations: "declare default element namespace 'http://s3.amazonaws.com/doc/2006-03-01/';"

    XmlRole { name: "name"; query: nameQuery }
    XmlRole { name: "size"; query: "Size/string()" }
    XmlRole { name: "tag"; query: "ETag/string()" }
    XmlRole { name: "modified"; query: "LastModified/string()" }
    XmlRole { name: "key"; query: "Key/string()" }
}

