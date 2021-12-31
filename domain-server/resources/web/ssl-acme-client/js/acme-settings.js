import {showLoadingScreen} from "./utils.js"
import {
    getSettings, postSettings, getAcmeMeta,
    putFile, deleteFile, getStatus, restartClient,
    getZeroSSLEebFromApiKey, getZeroSSLEebFromEmail
} from "./api.js"

const enableCheckbox = document.getElementById("enable");
const enableContent = document.getElementById("enable-content");

const certDirInput = document.getElementById("cert-dir");
const certNameInput = document.getElementById("cert-name");
const certKeyNameInput = document.getElementById("cert-key-name");
const certCANameInput = document.getElementById("cert-ca-name");
const certUpload = document.getElementById("cert-upload");
const certKeyUpload = document.getElementById("cert-key-upload");
const certCAUpload = document.getElementById("cert-authorities-upload");
const certResetButton = document.getElementById("cert-reset");

const accountKeyPathInput = document.getElementById("account-key-path");
const accountKeyUpload = document.getElementById("account-key-upload");
const accountKeyResetButton = document.getElementById("account-key-reset");

const directorySelect = document.getElementById("directory-url-select");
const directoryInput = document.getElementById("directory-url");
const termsOfServiceLink = document.getElementById("terms-of-service");
const authSelect = document.getElementById("auth-select");
const controlsContainer = document.getElementById("control-button-container");
const saveButton = document.getElementById("save-button");
const restartButton = document.getElementById("restart-button");

const zeroSSlAuthInput = document.getElementById("zero-ssl-auth-input");
const eabKidInput = document.getElementById("eab-kid-input");
const eabMacInput = document.getElementById("eab-mac-input");

const challengeSelect = document.getElementById("challenge-select");

const domainInputs = document.getElementById("domain-inputs");
const domainInput = domainInputs.children[0];
const addDomainButton = document.getElementById("add-domain-button");

const statusViewEnable = document.getElementById("status-view-enable");
const statusView = document.getElementById("status-view");

function updateStatusView() {
    getStatus().then(json => {
        statusView.value = JSON.stringify(json, null, 1);
    });
}

function update() {
    directoryInput.hidden = directorySelect.value !== "custom";
    termsOfServiceLink.hidden = directorySelect.value === "zerossl-rest-api";
    const authType = authSelect.value;
    let authPlaceholder = "ZeroSSL API Key";
    switch(authType) {
        case "account-key-only":
            zeroSSlAuthInput.hidden = true;
            eabKidInput.hidden = true;
            eabMacInput.hidden = true;
            break;

        case "zero-ssl-email":
            authPlaceholder = "ZeroSSL Email";
        case "zero-ssl-api-key":
            zeroSSlAuthInput.placeholder = authPlaceholder;
            zeroSSlAuthInput.hidden = false;
            eabKidInput.hidden = directorySelect.value === "zerossl-rest-api";
            eabMacInput.hidden = directorySelect.value === "zerossl-rest-api";
            eabKidInput.disabled = true;
            eabMacInput.disabled = true;
            break;
        case "id-mac":
            zeroSSlAuthInput.hidden = true;
            eabKidInput.hidden = false;
            eabMacInput.hidden = false;
            eabKidInput.disabled = false;
            eabMacInput.disabled = false;
            break;
    }

    for (const dirInput of document.getElementsByClassName("domain-dir-input")) {
        dirInput.hidden = challengeSelect.value !== "files";
    }

    enableContent.hidden = !enableCheckbox.checked;
    restartButton.hidden = !enableCheckbox.checked;

    statusView.hidden = !statusViewEnable.checked;
}

function getDirectoryUrl() {
    return directorySelect.value !== "custom"
        ? directorySelect.value
        : directoryInput.value;
}

function updateStatus() {
    updateDirectoryMeta();
}

function updateDirectoryMeta() {
    showLoadingScreen(true);
    getAcmeMeta(getDirectoryUrl()).then(meta => {
        const zeroSSLSelected = directorySelect.selectedOptions[0].text === "ZeroSSL ACME v2";
        const zeroSSLREST = directorySelect.value === "zerossl-rest-api";
        for (const authOption of authSelect.options) {
            const classes = Array.from(authOption.classList);
            if (zeroSSLREST)
            {
                authOption.disabled = authOption.value !== "zero-ssl-api-key";
                authOption.selected = authOption.value === "zero-ssl-api-key";
            } else {
                if (classes.includes("eab-option")) {
                    authOption.disabled = !meta || !meta.externalAccountRequired;
                }

                if (classes.includes("zero-ssl-auth-option")) {
                    if (zeroSSLSelected) {
                        if (classes.includes("zero-ssl-auth-option-default")) {
                            authOption.selected = true;
                        }
                    } else {
                        authOption.disabled = true;
                    }
                }
                if(authOption.disabled && authOption.selected) authOption.selected = false;
            }
        }
        termsOfServiceLink.href = meta && meta.termsOfService || "404.html";
        update();
    }).finally(() => {
        showLoadingScreen(false);
    });
}


directorySelect.addEventListener("change", () => {
    updateDirectoryMeta();
});

challengeSelect.addEventListener("change", update);
enableCheckbox.addEventListener("change", update);
statusViewEnable.addEventListener("change", update);

function getEabFromZeroSSL() {
    const authType = authSelect.value;
    const method = authType === "zero-ssl-email" ? getZeroSSLEebFromEmail :
        authType === "zero-ssl-api-key" ? getZeroSSLEebFromApiKey :
        undefined;
    if (method) {
        showLoadingScreen(true);
        method(zeroSSlAuthInput.value)
            .then((json) => {
                if (json.success) {
                    eabKidInput.value = json.eab_kid;
                    eabMacInput.value = json.eab_hmac_key;
                } else {
                    eabKidInput.value = "";
                    eabMacInput.value = "";
                }
            }).finally(() => {
                showLoadingScreen(false);
                update();
            });
    } else {
        update();
    }
}

authSelect.addEventListener("change", () => {
    const authType = authSelect.value;
    if (authType === "account-key-only" || authType === "id-mac") {
        update();
    } else {
        getEabFromZeroSSL();
    }
});

let directoryInputTimeout = undefined;
directoryInput.addEventListener("input", () => {
    if(undefined !== directoryInputTimeout)
        clearTimeout(directoryInputTimeout);
    directoryInputTimeout = setTimeout(updateDirectoryMeta, 500);
});

let zeroSSlAuthInputTimeout = undefined;
zeroSSlAuthInput.addEventListener("input", () => {
    if(undefined !== zeroSSlAuthInputTimeout)
        clearTimeout(zeroSSlAuthInputTimeout);
    zeroSSlAuthInputTimeout = setTimeout(getEabFromZeroSSL, 500);
});

function getDomainValues(domainInput) {

    return {
        domain: domainInput.getElementsByClassName("domain-name-input")[0].value,
        directory: domainInput.getElementsByClassName("domain-dir-input")[0].value
    }
}

function setDomainValues(domainInput, values) {
    domainInput.getElementsByClassName("domain-name-input")[0].value = values.domain || "",
    domainInput.getElementsByClassName("domain-dir-input")[0].value = values.directory || "";
}

function getDomains() {
    const domains = [];
    for (const domainInput of domainInputs.children) {
        domains.push(getDomainValues(domainInput));
    }
    return domains;
}

function setDomains(domains) {
    for (let i = 1, len = domainInputs.children.length; i < len;  ++i) {
        domainInputs.removeChild(domainInputs.children[i]);
    }

    setDomainValues(domainInput, domains[0] || {})

    for (let i = 1, len = domains.length; i < len;  ++i) {
        addDomainInput(domains[i]);
    }
}

function addDomainInput(domainValues) {
    const newDomainInput = domainInput.cloneNode(true);
    const removeButton = newDomainInput.getElementsByClassName("remove-domain-button")[0];
    removeButton.addEventListener("click", () => {
        domainInputs.removeChild(newDomainInput);
    });

    setDomainValues(newDomainInput, domainValues || {});

    domainInputs.appendChild(newDomainInput);
};

addDomainButton.addEventListener("click", addDomainInput);

function updateSettings() {
    getSettings().then((settings) => {
        const acme = settings.values.acme;
        setDirectoryUrl(acme.directory_endpoint);
        eabKidInput.value = acme.eab_kid;
        eabMacInput.value = acme.eab_mac;
        challengeSelect.value = acme.challenge_handler_type;
        setDomains(acme.certificate_domains);
        enableCheckbox.checked = acme.enable_client;
        certDirInput.value = acme.certificate_directory;
        certNameInput.value = acme.certificate_filename;
        certKeyNameInput.value = acme.certificate_key_filename;
        certCANameInput.value = acme.certificate_authority_filename;
        accountKeyPathInput.value = acme.account_key_path;
        if (acme.zerossl_rest_api) {
            directorySelect.value = "zerossl-rest-api";
            zeroSSlAuthInput.value = acme.zerossl_api_key;
        }
        updateStatus();
    });
}

function uploadFiles() {
    const arrayFromUpload = upload => {
        const array = Array.from(upload.files).map(file =>
            { return {file, endpoint: upload.id.replace("-upload","")}; })
        upload.value = upload.defaultValue;
        return array;
    };

    const files = arrayFromUpload(certUpload)
        .concat(arrayFromUpload(certKeyUpload))
        .concat(arrayFromUpload(certCAUpload))
        .concat(arrayFromUpload(accountKeyUpload))
    ;

    let queue = Promise.resolve();
    for (const file of files) {
        console.log("queueing file", file);
        queue = queue.then(() => deleteFile(file.endpoint))
            .then(() => putFile(file.endpoint, file.file));
    }

    return queue;
}

certResetButton.addEventListener("click", () => {
    showLoadingScreen(true);
    deleteFile("cert").then(() => deleteFile("cert-key"))
        .finally(() => showLoadingScreen(false));
});

accountKeyResetButton.addEventListener("click", () => {
    showLoadingScreen(true);
    deleteFile("account-key").finally(() => showLoadingScreen(false));
});

restartButton.addEventListener("click", () => {
    showLoadingScreen(true);
    restartClient().finally(() => {
        updateStatusView();
        showLoadingScreen(false);
    });
});

saveButton.addEventListener("click", () => {
    showLoadingScreen(true);
    postSettings({acme: {
        directory_endpoint: getDirectoryUrl(),
        eab_kid: eabKidInput.value,
        eab_mac: eabMacInput.value,
        challenge_handler_type: challengeSelect.value,
        certificate_domains: getDomains(),
        enable_client: enableCheckbox.checked,
        certificate_directory: certDirInput.value,
        certificate_filename: certNameInput.value,
        certificate_key_filename: certKeyNameInput.value,
        certificate_authority_filename: certCANameInput.value,
        account_key_path: accountKeyPathInput.value,
        zerossl_rest_api: directorySelect.value === "zerossl-rest-api",
        zerossl_api_key: zeroSSlAuthInput.value
    } }).then(() => {
        return uploadFiles();
    }).then(() => {
        updateSettings();
    }).finally(() => {
        updateStatusView();
        showLoadingScreen(false);
    });
});

function setDirectoryUrl(url) {
    if (url === "custom") {
        directorySelect.value = "custom";
        directoryInput.value = "custom";
    } else {
        const option = Array.from(directorySelect.options)
            .find(o => o.value === url);
        if(undefined !== option) {
            directorySelect.value = option.value;
        } else {
            directorySelect.value = "custom";
            directoryInput.value = url;
        }
    }
}

setInterval(updateStatusView, 2000);

updateSettings();
