const fs = require('fs');
const arg = require('arg');
const path = require('path');

const args = arg({
	// Types
	'-i': String,
    '-c': String,
    '-j': String
});

const defs = require(path.resolve(process.cwd(), args['-i']));

var c_output = "";
var js_output = {};

c_output = "// This file is generated automatically. DO NOT EDIT.\n\n#pragma once\n\n";

defs.forEach(def => {
    js_output[def.type_t] = {};
    c_output += "typedef enum {\n";

    def.symbols.forEach((symbol, index) => {
        js_output[def.type_t][symbol] = index;
        c_output += '    ' + symbol + ' = 0x' + index.toString(16).toUpperCase() + ',\n';        
    });

    c_output += '} ' + def.type_t + ';\n\n';
});

fs.writeFileSync(path.resolve(process.cwd(), args['-c']), c_output);
fs.writeFileSync(path.resolve(process.cwd(), args['-j']), JSON.stringify(js_output));
