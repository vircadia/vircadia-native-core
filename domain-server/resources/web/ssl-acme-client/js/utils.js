
function enableChildren(element, enabled = true) {
    for (const child of element.children) {
        child.disabled = !enabled;
    }
}

export { enableChildren };
