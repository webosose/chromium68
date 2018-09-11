"use strict";

const NO_REQUESTS_VALUE = '-1';
var numberCallFuncMessages = 0;
window.onload = initEmulatorPage;

function sendProcessDataValue() {
    let sel_value = document.getElementById('process_data_result');
    let ret_value = sel_value.options[sel_value.selectedIndex].value;
    let sel_request = document.getElementById('process_data_received_requests');
    let params = {
        id: sel_request.value,
        result: ret_value
    };
    createExpectation(kSample_processDataResponse, JSON.stringify(params));
    processDataRemoveSelectedRequest();
}

function processDataResponseChanged() {
    let message_id = document.getElementById(
        'process_data_received_requests').value;
    if (message_id.localeCompare(NO_REQUESTS_VALUE) === 0)
        return;
    document.getElementById('process_data_msg_num').innerHTML = message_id;
    enableSend(true);
}

function processDataNoSelectedResponse() {
    document.getElementById('process_data_msg_num').innerHTML =
        '(select request in the list)';
    enableSend(false);
}

function processDataRemoveSelectedRequest() {
    let sel_requests = document.getElementById(
        'process_data_received_requests');
    let selected_option = sel_requests.options[sel_requests.selectedIndex];
    sel_requests.removeChild(selected_option);
    if (sel_requests.children.length == 0) {
        let option = document.createElement('option');
        option.innerHTML = 'No unprocessed requests';
        option.value = NO_REQUESTS_VALUE;
        sel_requests.appendChild(option);
    }
    processDataNoSelectedResponse();
}

function enableSend(enable) {
    document.getElementById('process_data_btn').disabled = !enable;
}

function processDataRequestCallback(params_JSON) {
    let args = JSON.parse(params_JSON);
    if(args.hasOwnProperty('id') && args.hasOwnProperty('data')) {
        processDataInputValueCallback(args.id, args.data);
    } else {
        console.error("Invalid params for 'callFuncCallback()'.")
    }
}

function processDataInputValueCallback(request_ID, input_value) {
    addListElement(request_ID, input_value);
}

function calFunc(arg1, arg2) {
    numberCallFuncMessages++;
    document.getElementById('call_func_msg_num').innerHTML =
        numberCallFuncMessages.toString();

    document.getElementById('call_func_arg1').innerHTML = arg1;
    document.getElementById('call_func_arg2').innerHTML = arg2;
}

function callFuncCallback(params_JSON) {
    let args = JSON.parse(params_JSON);
    if(args.hasOwnProperty('arg1') && args.hasOwnProperty('arg2')) {
        calFunc(args.arg1, args.arg2);
    } else {
        console.error("Invalid params for 'callFuncCallback()'.")
    }
}

function initEmulatorPage() {
    document.getElementById('process_data_btn').disabled = true;
    addURLForPolling(kSample_callFunc, callFuncCallback);
    addURLForPolling(kSample_processDataReq, processDataRequestCallback);
}

const COLOR_SPAN_PREFIX = '<span style="color:blue">'
const SPAN_POST = '</span>'

function addListElement(messageNumber, value) {
    console.info("JL: messageNumber = ", messageNumber.toString(),
        ", value = ", value);
    let select = document.getElementById('process_data_received_requests');
    let option = document.createElement('option');
    let messageNumberStr = messageNumber.toString();
    let num = COLOR_SPAN_PREFIX + messageNumberStr + SPAN_POST;
    let str = ' Received request ' + num + ' with data = ';
    str += COLOR_SPAN_PREFIX + value.toString() + SPAN_POST + '. ';
    option.innerHTML = str;
    option.value = messageNumberStr;

    let children = select.children;
    if (children.length == 1 && children[0].value.
        localeCompare(NO_REQUESTS_VALUE) == 0 ) {
        select.removeChild(children[0]);
    }
    select.insertBefore(option, select.children[0]);
}
