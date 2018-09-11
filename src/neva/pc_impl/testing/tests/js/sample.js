"use strict";

function checkSampleAPI() {
    let output_element = 'web_api_tests';
    document.getElementById(output_element).innerHTML = "";

    let result = checkSampleInterfaces();
    printResult(new TestResult(result.passed, result.text), output_element);
}

function checkSampleInterfaces() {
    let result = {
        passed: true,
        text:   'All methods are supported.'
    };

    if (!IS_DEFINED(navigator.sample)) {
        result.text = 'navigator.sample is undefined';
        result.passed = false;
        return result;
    }

    let interface_methods = [
        [navigator.sample.getPlatformValue, 'getPlatformValue'],
        [navigator.sample.removeEventListener, 'removeEventListener'],
        [navigator.sample.addEventListener, 'addEventListener']
    ];

    for (let method of interface_methods) {
        if (!IS_FUNCTION_DEFINED(method[0])) {
            result.text = 'navigator.sample.' + method[1]
                + ' is either undefined' + ' or not a function';
            result.passed = false;
            return result;
        }
    }

    return result;
}

function sampleUpdateCallback(e) {
    console.info("event: ", e);
    console.info("data: ", e.detail);
    numberSampleUpdatesMessages++;
    document.getElementById('sample_update_msg_num').innerHTML =
        numberSampleUpdatesMessages.toString();
    document.getElementById('sample_update_returned_value').innerHTML =
        e.detail.value;
};

function sampleUpdateChangeSubscriptionStatus() {
    if (isSampleUpdateSubscribed) {
        navigator.sample.removeEventListener('samplenotify',
            sampleUpdateCallback)
    } else {
        navigator.sample.addEventListener('samplenotify',
            sampleUpdateCallback)
    }
    isSampleUpdateSubscribed = !isSampleUpdateSubscribed;
    let btn_txt = isSampleUpdateSubscribed ?
        "RemoveEventListener" : "AddEventListener";
    document.getElementById('sample_update_subscribe_btn').innerHTML =
        btn_txt;
}

function getPlatformValue() {
    numberGetPlatformValueMessages++;
    document.getElementById('get_platform_value_num').innerHTML =
        numberGetPlatformValueMessages.toString();
    document.getElementById('get_platform_returned_value').innerHTML =
        navigator.sample.getPlatformValue();
}

function processDataCallback(messageNumber, result) {
    let spanId = RESPONSE_ID_PREFIX + messageNumber.toString();
    let spanResponse = document.getElementById(spanId);
    if (spanResponse == null) return;
    let str = 'Received value = ';
    str += COLOR_SPAN_PREFIX + result.toString() + SPAN_POST + '. ';
    spanResponse.innerHTML = str;
    spanResponse.style.color = 'green';
};

function processData() {
    numberProcessDataMessages++;
    let data = document.getElementById('process_data_value_input').value;
    addListElement('process_data_sent_requests', numberProcessDataMessages,
                   ' sent request with data = ', data);
    navigator.sample.processData(createProcessDataCallback(
            numberProcessDataMessages), data);
}

function createProcessDataCallback(messageID) {
    return function(result) {
        console.info("JL: id = ", messageID);
        processDataCallback(messageID, result);
    }
}

function callFunc() {
    numberCallFuncMessages++;
    document.getElementById('call_func_msg_num').innerHTML =
        numberCallFuncMessages.toString();
    let arg1 = document.getElementById('call_func_arg1_input').value;
    let arg2 = document.getElementById('call_func_arg2_input').value;
    navigator.sample.callFunc(arg1, arg2);
}

const COLOR_SPAN_PREFIX = '<span style="color:blue">'
const SPAN_POST = '</span>'
const RESPONSE_ID_PREFIX = 'request_'

function addListElement(listId, messageNumber, text, value) {
    let list = document.getElementById(listId);
    let entry = document.createElement('li');
    let messageNumberStr = messageNumber.toString();
    let str = COLOR_SPAN_PREFIX + messageNumberStr + SPAN_POST;
    str += text;
    str += COLOR_SPAN_PREFIX + value.toString() + SPAN_POST + '. ';
    entry.innerHTML = str;
    let spanResult = document.createElement('span');
    let textNode = document.createTextNode('Waiting for response...');
    spanResult.style.color = 'red';
    spanResult.appendChild(textNode);
    spanResult.id = RESPONSE_ID_PREFIX + messageNumberStr;
    entry.appendChild(spanResult);
    list.insertBefore(entry, list.childNodes[0]);
}
