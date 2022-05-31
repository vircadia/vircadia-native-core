//
//  api.js
//  domain-server/resources/web/ssl-acme-client/js
//
//  Created by Nshan G. on 2021-12-20
//  Copyright 2021 Vircadia contributors.
//  Copyright 2021 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function getSettings() {
    return fetch("/settings.json").then(response => response.json());
}

function postSettings(settings) {
    return fetch("/settings.json",
    {
        headers: {
          "Accept": "application/json",
          "Content-Type": "application/json"
        },
        method: "POST",
        body: JSON.stringify(settings)
    });
}

function putFile(endpoint, file) {
    return fetch("/acme/" + endpoint, {
        method: "PUT",
        body: file
    });
}

function deleteFile(endpoint) {
    return fetch("/acme/" + endpoint, {
        method: "DELETE",
    });
}

function restartClient() {
    return fetch("/acme/update", {
        method: "POST",
    });
}

function getStatus() {
    return fetch("/acme/status").then(response => response.json());
}

function getAcmeMeta(directory) {
    return fetch(directory)
        .then(response => response.text())
        .then(text => {
            let json = {};
            try { json = JSON.parse(text); }
            catch(error) {}
            return json.meta;
        })
        .catch( () => undefined )
    ;
}

function getZeroSSLEebFromEmail(email) {
    const data = new FormData();
    data.append("email", email);
    return fetch("https://api.zerossl.com/acme/eab-credentials-email", {
        method: "POST",
        body: data
    })
        .then(response => response.json());
}

function getZeroSSLEebFromApiKey(apiKey) {
    const url = new URL("https://api.zerossl.com/acme/eab-credentials");
    url.searchParams.append("access_key", apiKey);
    return fetch(url, { method: "POST" }).then(response => response.json());
}

export { getSettings, postSettings, getAcmeMeta,
    putFile, deleteFile, restartClient, getStatus,
    getZeroSSLEebFromApiKey, getZeroSSLEebFromEmail };
