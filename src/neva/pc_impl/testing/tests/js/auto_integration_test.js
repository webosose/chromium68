"use strict";

const RESPONSE_DELAY = 1000;
const SET_EXPECTAION_DELAY = 100;
const WAIT_COUNT = 50;

const OUTPUT_ELEMENT = 'test_state';

window.onload = runAutoIntegrationTest;

function runAutoIntegrationTest() {
    clearStatus();
    printString('Tests are running...', OUTPUT_ELEMENT);
    let result = checkSampleInterfaces();
    if (!result.passed) {
        notifyResult(result);
        return;
    }
    testCallFunc()
        .then(testGetPlatformValue)
        .then(testSampleUpdate)
        .then(testProcessData)
        .then(() => {
            result.passed = true;
            result.text = 'All tests PASSED!!!';
            notifyResult(result);
        })
        .catch(error => {
            result.passed = false;
            result.text = error.message;
            notifyResult(result);
        })
}

function pollSeveralTimes(url, delay, count) {
    function pollCycle(url, delay, count, resolve, reject) {
        poll(url).then(
            data =>  { resolve(data); },
            error => {
                if (--count > 0 && error.code == 404) {
                    setTimeout(() => { pollCycle(url, delay, count, resolve,
                                       reject) }, delay);
                } else {
                    reject(error);
                }
            }
        );
    }
    return new Promise((resolve, reject) => {
        pollCycle(url, delay, count, resolve, reject);
    })
}

function testCallFunc() {
    const TEST_ARG_1 = 'CallFunc() TEST Argument 1';
    const TEST_ARG_2 = 'CallFunc() TEST Argument 2';
    const WAIT_DELAY = SET_EXPECTAION_DELAY;

    let wait_count = WAIT_COUNT;

    return new Promise((resolve, reject) => {
        navigator.sample.callFunc(TEST_ARG_1, TEST_ARG_2);
        setTimeout(() => { resolve(); }, WAIT_DELAY);
    }).then(() => {
        return pollSeveralTimes(kSample_callFunc, WAIT_DELAY, WAIT_COUNT);
    }).then(data => {
        return new Promise((resolve, reject) => {
            let args = JSON.parse(data);
            if(args.hasOwnProperty('arg1') && args.hasOwnProperty('arg2')) {
                if ((args.arg1.localeCompare(TEST_ARG_1) == 0)
                        && (args.arg2.localeCompare(TEST_ARG_2) == 0)) {
                    resolve();
                    return;
                }
            }
            let error =
                new Error("Invalid transfered params for 'CallFunc()'.");
            console.error(error);
            reject(error);
        });
    });
}

function testGetPlatformValue() {
    const TEST_RET_VALUE = 'GetPlatformValue() TEST Return Value';
    const READ_VALUE_TO_CACHE_DELAY = RESPONSE_DELAY;

    return createExpectationPromise(kSample_getPlatformValue, TEST_RET_VALUE)
        .then(() => { return createTimeout(READ_VALUE_TO_CACHE_DELAY); })
        .then(() => {
            return new Promise((resolve, reject) => {
                let value = navigator.sample.getPlatformValue();
                if (value.localeCompare(TEST_RET_VALUE) == 0) {
                    resolve();
                    return;
                }
                let error = new Error(
                    "Invalid returned value for 'GetPlatformValue()'.");
                console.error(error);
                reject(error);
                });
        });
}

var sampleUpdateValue = '';
function sampleUpdateCallback(e) {
    sampleUpdateValue = e.detail.value;
};

function testSampleUpdate() {
    const TEST_NOTIFY_VALUE = 'SampleUpdate() TEST Notify Value';
    const ADD_LISTENER_DELAY = 100;
    const GET_NOTIFICATION_DELAY = RESPONSE_DELAY;

    return new Promise((resolve, reject) => {
        sampleUpdateValue = '';
        navigator.sample.addEventListener('samplenotify', sampleUpdateCallback);
        setTimeout(() => { resolve(); }, ADD_LISTENER_DELAY);
    })
    .then(() => {
        return createExpectationPromise(kSample_sampleUpdate, TEST_NOTIFY_VALUE);
    })
    .then(() => { return createTimeout(GET_NOTIFICATION_DELAY); })
    .then(() => {
        return new Promise((resolve, reject) => {
            if (sampleUpdateValue.localeCompare(TEST_NOTIFY_VALUE) == 0) {
                resolve();
                return;
            }
            let error = new Error(
                "Incorrect or no notification received for 'SampleUpdate()'.");
            console.error(error);
            reject(error);
            });
    });
}

const TEST_PROCESS_DATA_MESAGES_NUM = 2;
const TEST_PROCESS_DATA_ARGUMENTS = [
    'ProcessData() TEST Argument 1',
    'ProcessData() TEST Argument 2'
    ];
const TEST_PROCESS_DATA_RETURN_VALUES = [false, true];

var processDataResult = null;
var processDataRequestId = [];

function processDataCallback(messageID, result) {
    processDataResult = {};
    processDataResult.data = result;
    processDataResult.id = messageID;
};

function createProcessDataCallback(messageID) {
    return function(result) {
        processDataCallback(messageID, result);
    }
}

function checkRequestProcessData(messageNum, data) {
    const ERR_INVALID_PARAMS = "Invalid transfered params for 'ProcessData()'.";
    return new Promise((resolve, reject) => {
        let args = JSON.parse(data);
        if(args.hasOwnProperty('id') && args.hasOwnProperty('data')) {
            processDataRequestId[messageNum] = args.id;
            if (args.data.localeCompare(
                TEST_PROCESS_DATA_ARGUMENTS[messageNum]) == 0) {
                resolve();
                return;
            }
        }
        let error = new Error(ERR_INVALID_PARAMS);
        reject(error);
    });
}

function sendResponseProcessData(messageNum) {
    let params = {
        id: processDataRequestId[messageNum].toString(),
        result: TEST_PROCESS_DATA_RETURN_VALUES[messageNum].toString()
    };
    return createExpectationPromise(
        kSample_processDataResponse, JSON.stringify(params));
}

function checkResponseProcessData(messageNum) {
    return new Promise((resolve, reject) => {
        if (processDataResult != null && processDataResult.id == messageNum
            && processDataResult.data ==
            TEST_PROCESS_DATA_RETURN_VALUES[messageNum]) { resolve(); return; }
        let errorText = "Incorrect or no response received for 'ProcessData("
            + messageNum.toString() + ")'.";
        let error = new Error(errorText);
        reject(error);
    });
}

function callProcessData(messageNum) {
    return new Promise((resolve, reject) => {
        navigator.sample.processData(createProcessDataCallback(messageNum),
            TEST_PROCESS_DATA_ARGUMENTS[messageNum]);
        setTimeout(() => { resolve(); }, SET_EXPECTAION_DELAY);
    });
}

function testProcessData() {
    const NEXT_CALL_DELAY = 100;

    return callProcessData(0)
    .then(() => {
        return pollSeveralTimes(kSample_processDataReq, SET_EXPECTAION_DELAY,
            WAIT_COUNT);
    })
    .then(data => { return checkRequestProcessData(0, data); })
    .then(() => { return callProcessData(1); })
    .then(() => {
        return pollSeveralTimes(kSample_processDataReq, SET_EXPECTAION_DELAY,
            WAIT_COUNT);
     })
    .then(data => { return checkRequestProcessData(1, data); })
    .then(() => { return sendResponseProcessData(1); })
    .then(() => { return createTimeout(RESPONSE_DELAY); })
    .then(() => { return checkResponseProcessData(1); })
    .then(() => { return sendResponseProcessData(0); })
    .then(() => { return createTimeout(RESPONSE_DELAY); })
    .then(() => { return checkResponseProcessData(0); })
    ;
}

function createTimeout(delay) {
    return new Promise((resolve, reject) => {
        setTimeout(() => { resolve(); }, delay);
    });
}

function clearStatus() {
    document.getElementById(OUTPUT_ELEMENT).innerHTML = '';
}

function notifyResult(result) {
    clearStatus();
    printResult(new TestResult(result.passed, result.text), OUTPUT_ELEMENT);
    result.name = 'Auto integration test for PAL and Web API mechanism';
    sendResultToBTS(result);
}

function sendResultToBTS(result) {
    let url = window.location.href;
    var xhttp = new XMLHttpRequest();
    xhttp.open("POST", url, true);
    xhttp.setRequestHeader("Content-Type", "application/json; charset=utf-8");
    xhttp.send(JSON.stringify(result));
}
