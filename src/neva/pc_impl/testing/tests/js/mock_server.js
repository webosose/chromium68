"use strict";

var urls = {};

function createExpectation(url, value) {
    console.log(
        'createExpectation(): setting "' + url + '" to "' + value + '"');
    createExpectationPromise(url, value);
}

function createExpectationPromise(url, value) {
    return new Promise(function(resolve, reject) {
        var xmlHttpRequest = new XMLHttpRequest();
        // To avoid the 'Synchronous XMLHttpRequest on the main thread
        // is deprecated because of its detrimental effects to the end
        // user's experience. For more help, check
        // https://xhr.spec.whatwg.org/.' error message, the request
        // below shall be asynchronous (3rd parameter in open() is set to true).
        xmlHttpRequest.open("PUT", kEmulatorBaseURL + kEmulatorExpectationPath,
            true);

        xmlHttpRequest.setRequestHeader("Content-Type",
                                        "application/json; charset=utf-8");

        xmlHttpRequest.onload = function() {
            if (this.status >= 200 && this.status < 300) {
                resolve();
            } else {
                var error = new Error(this.statusText);
                error.code = this.status;
                reject(error);
            }
        };

        xmlHttpRequest.onerror = function() {
          reject(new Error("Network Error"));
        };
        xmlHttpRequest.send(JSON.stringify({
            "httpRequest": {
                "method": "",
                "path": '/' + url,
                "queryStringParameters": [],
                "body": "",
                "headers": [],
                "cookies": []
            },
            "httpResponse": {
                "statusCode": 200,
                "body": value,
                "cookies": [],
                "headers": [{
                    "name": "Content-Type",
                    "values": ["text/plain; charset=utf-8"]
                }, {
                    "name": "Cache-Control",
                    "values": ["no-cache, no-store"]
                }],
                "delay": {
                    "timeUnit": "MICROSECONDS",
                    "value": 0
                }
            },
            "times": {
                "remainingTimes": 1,
                "unlimited": false
            }
        }));
    });
}

function clearExpectations(url) {
    mockServerClient(kLocalHost, kLocalPort).clear("/" + url);
}

function periodicPoll() {
    for (let url in urls) {
        poll(url).then(data => { urls[url](data); }).catch(error => {} );
    };
    setTimeout(periodicPoll, poll_period);
}

function poll(url) {
  return new Promise(function(resolve, reject) {

    var xhr = new XMLHttpRequest();
    xhr.open('GET', kEmulatorBaseURL + url, true);

    xhr.onload = function() {
      if (this.status == 200) {
        resolve(this.responseText);
      } else {
        var error = new Error('Mock server polling error for url = '
            + url + ': ' + this.statusText);
        error.code = this.status;
        reject(error);
      }
    };

    xhr.onerror = function() {
      reject(new Error("Network Error"));
    };
    xhr.send();
  });
}

function addURLForPolling(url, callback) {
    urls[url] = callback;
}
