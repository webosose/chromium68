"use strict";

const NO_REQUESTS_VALUE = '-1';
var callsNumber = 0;
window.onload = initEmulatorPage;

function sendCommandCallback(params_JSON) {
    let args = JSON.parse(params_JSON);
    if(args.hasOwnProperty('name')) {
        callsNumber++;
        let args_str = '';
        for (let k in args) {
          if ((k.length > 3) && (k.substring(0, 3) == 'arg'))
            args_str += args[k] + ','
        }
        let str = args.name + '(' + args_str.substring(0, args_str.length - 1) + ')'
        addListElement(callsNumber, str)
    } else {
        console.error("Invalid params for 'callFuncCallback()'.")
    }
}

function callFunctionCallback(params_JSON) {
    let args = JSON.parse(params_JSON);
    if(args.hasOwnProperty('name') && args.hasOwnProperty('result')) {
        callsNumber++;
        let args_str = '';
        for (let k in args) {
          if ((k.length > 3) && (k.substring(0, 3) == 'arg'))
            args_str += args[k] + ','
        }
        let str = args.result + ' = '+ args.name + '(' +
                  args_str.substring(0, args_str.length - 1) + ')'
        addListElement(callsNumber, str)
    } else {
        console.error("Invalid params for 'callFuncCallback()'.")
    }
}

function initEmulatorPage() {
    addURLForPolling(kBrowserControl_callFunction, callFunctionCallback);
    addURLForPolling(kBrowserControl_sendCommand, sendCommandCallback);
}

const COLOR_SPAN_PREFIX = '<span style="color:blue">'
const SPAN_POST = '</span>'

function addListElement(messageNumber, value) {
    let select = document.getElementById('browser_control_log');
    let option = document.createElement('option');
    option.innerHTML = messageNumber.toString() + ' : ' + value;
    option.value = messageNumber;

    let children = select.children;
    if (children.length == 1 && children[0].value.
        localeCompare(NO_REQUESTS_VALUE) == 0 ) {
        select.removeChild(children[0]);
    }
    select.insertBefore(option, select.children[0]);
}
