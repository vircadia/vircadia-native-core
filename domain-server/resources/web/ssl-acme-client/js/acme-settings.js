import {enableChildren} from "./utils.js"
import {getSettings, postSettings, getAcmeMeta} from "./api.js"

const directorySelect = document.getElementById("directory-url-select");
const directoryInput = document.getElementById("directory-url");
const eabContainer = document.getElementById("eab-container");
const controlsContainer = document.getElementById("control-button-container");
const saveButton = document.getElementById("save-button");

function update() {
    directoryInput.hidden = directorySelect.value !== "custom";
}

function getDirectoryUrl() {
    return directorySelect.value !== "custom"
        ? directorySelect.value
        : directoryInput.value;
}

function updateDirectoryMeta() {
    document.body.hidden = true; // TODO: proper loading UI
    getAcmeMeta(getDirectoryUrl()).then(meta => {
        eabContainer.hidden = !(meta && meta.externalAccountRequired);
        update();
    }).finally(() => {
        document.body.hidden = false;
    });
}

directorySelect.addEventListener("change", () => {
    updateDirectoryMeta();
});

let directoryInputTimeout = undefined;
directoryInput.addEventListener("input", () => {
    if(undefined !== directoryInputTimeout)
        clearTimeout(directoryInputTimeout);
    directoryInputTimeout = setTimeout(updateDirectoryMeta, 500);
});

saveButton.addEventListener("click", () => {
    enableChildren(controlsContainer, false);
    postSettings({acme: {
        directory_endpoint: getDirectoryUrl()
    } }).then(() => {
        enableChildren(controlsContainer, true);
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

getSettings().then((settings) => {
    setDirectoryUrl(settings.values.acme.directory_endpoint);
    updateDirectoryMeta();
});
