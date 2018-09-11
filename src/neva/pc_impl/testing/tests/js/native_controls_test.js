"use strict";
const FILE_CHOOSER = "file_chooser";
const FILE_CHOOSER_RESULT = "file_chooser_result";

window.onload = initPage;

function callFileChooser() {
    document.getElementById(FILE_CHOOSER_RESULT).innerHTML = '';
    let file_chooser = document.getElementById('file_chooser');
    file_chooser.accept = document.getElementById('accept_types').value;
    file_chooser.multiple = document.getElementById('multiple').checked;

    file_chooser.click();
}

function readFileChooser() {
    let file_chooser = document.getElementById(FILE_CHOOSER);
    let files = [];
    let text = '';
    if (!("files" in file_chooser && (file_chooser.files.length > 0))) {
        text = 'No selected files';
    } else {
        text = `<h2>Selected files list:<\h2>
                <table>
                    <tr>
                        <th>Name</th>
                        <th>Size</th>
                        <th>Date</th>
                    </tr>`;
        for (let file of file_chooser.files) {
            let date = new Date(file.lastModified);
            text += '<tr>';
            text += ('<td>' + file.name + '</td>');
            text += ('<td>' + file.size + '</td>');
            text += ('<td class="file-date">' + date + '</td>');
            text += '</tr>';
        }
        text += '</table>';
    }
    let res_element = document.getElementById(FILE_CHOOSER_RESULT);
    res_element.innerHTML = text;
    res_element.style.visibility = 'visible';
}

function initPage() {
    document.getElementById(FILE_CHOOSER).onchange = readFileChooser;
}