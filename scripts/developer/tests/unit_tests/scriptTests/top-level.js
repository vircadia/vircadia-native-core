Script.include('./nested/lib.js');
outer = {
    inner: inner.lib,
    sibling: sibling.lib,
};
