import {showLoadingScreen} from "./utils.js"
import {
    getSettings, postSettings, getAcmeMeta,
    getZeroSSLEebFromApiKey, getZeroSSLEebFromEmail
} from "./api.js"

const certDirInput = document.getElementById("cert-dir");
const certNameInput = document.getElementById("cert-name");
const certKeyNameInput = document.getElementById("cert-key-name");
const certCANameInput = document.getElementById("cert-ca-name");
const certUpload = document.getElementById("cert-upload");
const certKeyUpload = document.getElementById("cert-key-upload");
const certCAUpload = document.getElementById("cert-ca-upload");
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

const zeroSSlAuthInput = document.getElementById("zero-ssl-auth-input");
const eabKidInput = document.getElementById("eab-kid-input");
const eabMacInput = document.getElementById("eab-mac-input");

const challengeSelect = document.getElementById("challenge-select");

const domainInputs = document.getElementById("domain-inputs");
const domainInput = domainInputs.children[0];
const addDomainButton = document.getElementById("add-domain-button");

function update() {
    directoryInput.hidden = directorySelect.value !== "custom";
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
            eabKidInput.hidden = false;
            eabMacInput.hidden = false;
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
        const zeroSSLSelected = directorySelect.selectedOptions[0].text === "ZeroSSL";
        for (const authOption of authSelect.options) {
            const classes = Array.from(authOption.classList);
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
        termsOfServiceLink.href = meta && meta.termsOfService || "404.html";
        update();
    }).finally(() => {
        showLoadingScreen(false);
    });
}

directorySelect.addEventListener("change", () => {
    updateDirectoryMeta();
});

challengeSelect.addEventListener("change", () => {
    update();
});

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
        setDirectoryUrl(settings.values.acme.directory_endpoint);
        eabKidInput.value = settings.values.acme.eab_kid;
        eabMacInput.value = settings.values.acme.eab_mac;
        challengeSelect.value = settings.values.acme.challenge_handler_type;
        setDomains(settings.values.acme.certificate_domains);
        updateStatus();
    });
}

saveButton.addEventListener("click", () => {
    showLoadingScreen(true);
    postSettings({acme: {
        directory_endpoint: getDirectoryUrl(),
        eab_kid: eabKidInput.value,
        eab_mac: eabMacInput.value,
        challenge_handler_type: challengeSelect.value,
        certificate_domains: getDomains()
    } }).then(() => {
        updateSettings();
    }).finally(() => {
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

updateSettings();
