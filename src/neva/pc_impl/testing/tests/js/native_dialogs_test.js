"use strict";

function callAlertDialog() {
    document.getElementById('alert_result').style.visibility = 'hidden';
    setTimeout(callAlertDialogEx, 0);
}

function callAlertDialogEx() {
    alert(document.getElementById('alert_message').value)
    document.getElementById('alert_result').style.visibility = 'visible';
}

function callConfirmDialog() {
    document.getElementById('confirm_result').style.visibility = 'hidden';
    setTimeout(callConfirmDialogEx, 0);
}

function callConfirmDialogEx() {
    let res = confirm(document.getElementById('confirm_message').value);
    let txt ='';
    if (res == true) {
        txt = 'You pressed OK!';
    } else {
        txt = 'You pressed Cancel!';
    }
    let res_element = document.getElementById('confirm_result');
    res_element.innerHTML = txt;
    res_element.style.visibility = 'visible';
}

function callPromptDialog() {
    document.getElementById('prompt_result').style.visibility = 'hidden';
    setTimeout(callPromptDialogEx, 0);
}

function callPromptDialogEx() {
    let default_text = document.getElementById('prompt_default_text').value;
    let prompt_text= document.getElementById('prompt_text').value;
    let res = prompt(prompt_text, default_text);
    let txt = '';
    if (res != null) {
        txt = 'You entered "' + res + '"';
    } else {
        txt = 'You pressed Cancel!';
    }
    let res_element = document.getElementById('prompt_result');
    res_element.innerHTML = txt;
    res_element.style.visibility = 'visible';
}
