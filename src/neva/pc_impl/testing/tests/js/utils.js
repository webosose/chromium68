"use strict";

var TestResult = function(result, msg) {
    Object.defineProperties(this, {
        result: {
            value: result,
            writable: false
        },
        message: {
            value: msg,
            writable: false
        }
    });
};

function IS_DEFINED(object) {
    if (object === undefined) {
        return false;
    }
    return true;
}

function IS_FUNCTION_DEFINED(fn) {
    if (fn === undefined || !(fn instanceof Function)) {
        return false;
    }
    return true;
}

function sleep(sleepDuration) {
    var now = new Date().getTime();
    while (new Date().getTime() < now + sleepDuration) {
        var a = 1 + 1;
    }
}

function printString(msg, id) {
    var textnode = document.createTextNode(msg);
    var span = document.createElement('span');
    span.style.fontStyle = 'bold';
    span.appendChild(textnode);
    document.getElementById(id).appendChild(span);
    var br = document.createElement('br');
    document.getElementById(id).appendChild(br);
};

function printResult(status, id) {
    var textnode = document.createTextNode(status.message + '........');
    var span_msg = document.createElement('span');
    span_msg.appendChild(textnode);
    var span_result;
    if (status.result) {
        var ok = document.createTextNode('OK');
        span_result = document.createElement('span');
        span_result.style.color = 'green';
        span_result.style.fontStyle = 'bold';
        span_result.appendChild(ok);
    } else {
        var fail = document.createTextNode('FAIL');
        span_result = document.createElement('span');
        span_result.style.color = 'red';
        span_result.style.fontStyle = 'bold';
        span_result.appendChild(fail);
    }
    document.getElementById(id).appendChild(span_msg);
    document.getElementById(id).appendChild(span_result);
    var br = document.createElement('br');
    document.getElementById(id).appendChild(br);
};

function printSuccessString(msg, id) {
    var textnode = document.createTextNode(msg);
    var span = document.createElement('span');
    span.style.color = 'green';
    span.style.fontStyle = 'bold';
    span.appendChild(textnode);
    document.getElementById(id).appendChild(span);
    var br = document.createElement('br');
    document.getElementById(id).appendChild(br);
};

function printErrorString(msg, id) {
    var textnode = document.createTextNode(msg);
    var span = document.createElement('span');
    span.style.color = 'red';
    span.style.fontStyle = 'bold';
    span.appendChild(textnode);
    document.getElementById(id).appendChild(span);
    var br = document.createElement('br');
    document.getElementById(id).appendChild(br);
};
