"use strict";

const MODE = {
    UNKNOWN: -1,
    OPEN: 0,
    OPENMULTIPLE: 1,
    UPLOADFOLDER: 2,
    SAVE: 3
};

const COLOR_CHOOSER = "color_chooser";
const FILE_CHOOSER = "file_chooser";

const CONTROL_SECTIONS = ["show_color_chooser", "show_file_chooser"];

window.onload = initEmulatorPage;
let mode = MODE.UNKNOWN;

function openColorChooser(params_JSON) {
    let args = JSON.parse(params_JSON);
    if (args.hasOwnProperty("color") && args.hasOwnProperty("suggestions")) {
        showColorChooser(args);
    } else {
        console.error("Invalid params for 'openColorChooser()'.");
    }
}

function showColorChooser(args) {
    let color_chooser = document.getElementById(COLOR_CHOOSER);
    let color_value = args.color & 0xffffff; //RGB format is used.
    color_chooser.value = "#" + ("00000" + color_value.toString(16)).substr(-6);
    showControlSection("show_color_chooser");
}

function closeColorChooser() {
    createExpectation(kNativeControls_closedColorChooser,"");
    resetDialogView();
}

function onChangeColorChooser() {
    let chosen_color = document.getElementById(COLOR_CHOOSER).value;
    chosen_color = chosen_color.substr(1);
    let chosen_color_value = parseInt(chosen_color, 16) | 0xFF000000;
    let params = { color: chosen_color_value.toString() };
    createExpectation(
        kNativeControls_changedColorChooser,
        JSON.stringify(params)
    );
}

function runFileChooser(params_JSON) {
    let args = JSON.parse(params_JSON);
    if (
        args.hasOwnProperty("mode") &&
        args.hasOwnProperty("title") &&
        args.hasOwnProperty("default_file_name") &&
        args.hasOwnProperty("accept_types") &&
        args.hasOwnProperty("need_local_path") &&
        args.hasOwnProperty("url")
    ) {
        showFileChooser(args);
    } else {
        console.error("Invalid params for 'runFileChooser()'.");
    }
}

function showFileChooser(args) {
    let file_chooser = document.getElementById(FILE_CHOOSER);
    let accept_types = "";
    let first = true;
    for (let accept_type of args.accept_types) {
        if (!first) accept_types += ",";
        else first = false;
        accept_types += accept_type;
    }
    file_chooser.accept = accept_types;
    file_chooser.multiple = args.mode == MODE.OPENMULTIPLE;
    mode = args.mode;
    showControlSection("show_file_chooser");
}

function callFileChooser() {
    document.getElementById(FILE_CHOOSER).click();
}

function readFileChooser() {
    let file_chooser = document.getElementById(FILE_CHOOSER);
    let files = [];
    if ("files" in file_chooser) {
        for (let file of file_chooser.files) {
            var date = new Date(file.lastModified);
            files.push({
                display_name: file.name,
                file_path: "/tmp/" + file.name,
                file_system_url: "",
                size: file.size,
                is_directory: false,
                modification_time: date.getTime().toFixed(0),
                type: file.type
            });
        }
    }
    let params = { mode: mode, file_list: files };
    createExpectation(kNativeControls_retFileChooser, JSON.stringify(params));
    resetDialogView();
}

function showControlSection(section_id) {
    document.getElementById("no_calls").style.display = "none";
    document.getElementById(section_id).style.display = "block";
}

function resetDialogView() {
    for (let section of CONTROL_SECTIONS) {
        document.getElementById(section).style.display = "none";
    }
    document.getElementById("no_calls").style.display = "block";
}

var poll_period = 500;

function initEmulatorPage() {
    resetDialogView();
    let color_chooser = document.getElementById(COLOR_CHOOSER);
    color_chooser.onchange = onChangeColorChooser;
    document.getElementById(FILE_CHOOSER).onchange = readFileChooser;
    addURLForPolling(kNativeControls_openColorChooser, openColorChooser);
    addURLForPolling(kNativeControls_runFileChooser, runFileChooser);
    setTimeout(periodicPoll, poll_period);
}
