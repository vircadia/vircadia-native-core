//  networkingConstants.js
//
//  Created by Kalila L. on 8/27/20
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

// Interface Metadata Source
var INTERFACE_METADATA_SOURCE = "https://cdn.vircadia.com/dist/launcher/vircadiaMeta.json";

// Base CDN URLs
var CONTENT_CDN_URL = ExternalResource.HF_Content; // "https://cdn-1.vircadia.com/" + usCDN;
var PUBLIC_BUCKET_CDN_URL = ExternalResource.HF_Public;
var euCDN = "eu-c-1/";
var usCDN = "us-c-1/";

module.exports = {
    INTERFACE_METADATA_SOURCE: INTERFACE_METADATA_SOURCE.
    CONTENT_CDN_URL: CONTENT_CDN_URL,
    PUBLIC_BUCKET_CDN_URL: PUBLIC_BUCKET_CDN_URL,
    euCDN: euCDN,
    usCDN: usCDN
}
