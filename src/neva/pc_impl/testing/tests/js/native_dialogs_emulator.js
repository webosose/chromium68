"use strict";

window.onload = initEmulatorPage;

function runJavaScriptDialog(params_JSON) {
    let args = JSON.parse(params_JSON);
    if (
        args.hasOwnProperty("dialog_type") &&
        args.hasOwnProperty("message_text") &&
        args.hasOwnProperty("default_prompt_text") &&
        args.hasOwnProperty("url")
    ) {
        showParamsJavaScriptDialog(
            args.dialog_type,
            args.message_text,
            args.default_prompt_text,
            args.url
        );
    } else {
        console.error("Invalid params for 'runJavaScriptDialog()'.");
    }
}

const DIALOG_TYPE = {
    ALERT:      "0",
    CONFIRM:    "1",
    PROMPT:     "2"
};

function cancelJavaScriptDialog() {
    console.info("Cansel dialog");
    resetDialogView();
}

var current_dialog_type = "";
function showParamsJavaScriptDialog(dlg_type, message, prompt, url) {
    try {
        let type = "";
        console.info(
            "dialog type = ",
            dlg_type.toString(),
            ", message = ",
            message,
            ", prompt = ",
            prompt
        );

        switch (dlg_type) {
            case DIALOG_TYPE.CONFIRM:
                type = "Confirm Dialog";
                document.getElementById("dialog_type").className =
                    "dialog_confirm";
                document.getElementById("cancel_button").style.display =
                    "block";
                break;
            case DIALOG_TYPE.ALERT:
                type = "Alert Dialog";
                document.getElementById("dialog_type").className =
                    "dialog_alert";
                break;
            case DIALOG_TYPE.PROMPT:
                type = "Prompt Dialog";
                document.getElementById("dialog_type").className =
                    "dialog_prompt";
                document.getElementById("cancel_button").style.display =
                    "block";
                document.getElementById("dilog_input").style.display = "block";
                document.getElementById("input_text").value = prompt;
                break;
            default:
                console.error(
                    "Invalid dialog type for 'runJavaScriptDialog()'."
                );
                return;
        }
        let title = url + " says:";
        document.getElementById("dialog_title").innerHTML = title;
        document.getElementById("dialog_message").innerHTML = message;
        current_dialog_type = dlg_type;
        document.getElementById("no_calls").style.display = "none";
        document.getElementById("show_dialog").style.display = "block";
        document.getElementById("dialog_type").innerHTML = type;
    } catch (err) {
        console.error("Error in showParamsJavaScriptDialog(): ", err);
    }
}

function closeDialog(exit_success) {
    let input_text = "";
    if (current_dialog_type == DIALOG_TYPE.PROMPT) {
        input_text = document.getElementById("input_text").value;
    }
    let params = {
        success: exit_success.toString(),
        dialog_type: current_dialog_type,
        input_text: input_text
    };
    createExpectation(
        kNativeDialogs_retJavaScriptDialog,
        JSON.stringify(params)
    );
    resetDialogView();
}

function resetDialogView() {
    document.getElementById("show_dialog").style.display = "none";
    document.getElementById("no_calls").style.display = "block";
    document.getElementById("dialog_type").innerHTML = "";
    document.getElementById("cancel_button").style.display = "none";
    document.getElementById("dilog_input").style.display = "none";
}

var poll_period = 500;
function initEmulatorPage() {
    document.getElementById("show_dialog").style.display = "none";
    addURLForPolling(
        kNativeDialogs_cancelJavaScriptDialog,
        cancelJavaScriptDialog
    );
    addURLForPolling(kNativeDialogs_runJavaScriptDialog, runJavaScriptDialog);
    setTimeout(periodicPoll, poll_period);
}
