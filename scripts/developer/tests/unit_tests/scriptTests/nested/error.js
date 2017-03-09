afterError = false;
throw new Error('nested/error.js');
afterError = true;

(1,eval)("this").$finishes.push(Script.resolvePath(''));
