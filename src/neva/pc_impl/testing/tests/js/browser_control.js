"use strict";

const NO_CALLS_VALUE = '-1';

function checkBrowserControlAPI() {
    let output_element = 'web_api_tests';
    document.getElementById(output_element).innerHTML = "";

    let result = checkBrowserControlInterfaces();
    printResult(new TestResult(result.passed, result.text), output_element);
}

function checkBrowserControlInterfaces() {
    let result = {
        passed: true,
        text:   'All methods are supported.'
    };

    if (!IS_DEFINED(navigator.browser_control)) {
        result.text = 'navigator.browser_control is undefined'
        result.passed = false
        return result
    }

    let interface_methods = [
        [navigator.browser_control.CallFunction, 'CallFunction'],
        [navigator.browser_control.SendCommand, 'SendCommand'],
    ]

    for (let method of interface_methods) {
        if (!IS_FUNCTION_DEFINED(method[0])) {
            result.text = 'navigator.browser_control.' + method[1]
                + ' is either undefined' + ' or not a function'
            result.passed = false
            return result
        }
    }
    return result
}

function callFunction() {
    callsNumber++;
    let name = document.getElementById('call_function_name_input').value.trim()
    let args_str = document.getElementById('call_function_arguments_input').value.trim()
    let params = args_str.split(',').map(function(x) {return x.trim()}).filter(word => word.length > 0)
    let args = [name].concat(params)
    let result = navigator.browser_control.CallFunction.apply(navigator.browser_control, args)
    let message = result + ' = callFunction(' + name + ', ' + params.join() + ')'
    addListElement(callsNumber, message)
}

function sendCommand() {
    callsNumber++;
    let name = document.getElementById('send_command_name_input').value.trim()
    let args_str = document.getElementById('send_command_arguments_input').value.trim()
    let params = args_str.split(',').map(function(x) {return x.trim()}).filter(word => word.length > 0)
    let args = [name].concat(params)
    navigator.browser_control.SendCommand.apply(navigator.browser_control, args)
    addListElement(callsNumber, 'sendCommand(' + name + ', ' + params.join() + ')')
}

const COLOR_SPAN_PREFIX = '<span style="color:blue">'
const SPAN_POST = '</span>'

function addListElement(messageNumber, message, iscommand) {
    let select = document.getElementById('browser_control_log')
    let option = document.createElement('option')
    let messageNumberStr = messageNumber.toString()
    let num = COLOR_SPAN_PREFIX + messageNumberStr + SPAN_POST
    let str = num + ' : ' + message
    option.innerHTML = str
    option.value = messageNumberStr

    let children = select.children;
    if (children.length == 1 && children[0].value.
        localeCompare(NO_CALLS_VALUE) == 0 ) {
        select.removeChild(children[0])
    }
    select.insertBefore(option, select.children[0])
}
