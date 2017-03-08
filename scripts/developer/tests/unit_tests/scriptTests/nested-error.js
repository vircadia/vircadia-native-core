afterError = false;
outer = null;
Script.include('./nested/error.js?' + Settings.getValue('cache_buster'));
outer = {
    inner: inner.lib,
    sibling: sibling.lib,
};
afterError = true;

(1,eval)("this").$finishes.push(Script.resolvePath(''));
