afterError = false;
outer = null;
Script.include('./nested/lib.js');
Undefined_symbol;
outer = {
    inner: inner.lib,
    sibling: sibling.lib,
};
afterError = true;

(1,eval)("this").$finishes.push(Script.resolvePath(''));
