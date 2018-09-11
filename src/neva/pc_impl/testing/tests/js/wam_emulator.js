"use strict";

const NO_FRAME     = "thick solid black";
const BLUE_FRAME   = "thick solid #29B0D9";
const GREEN_FRAME  = "thick solid green";
const RED_FRAME    = "thick solid red";
const YELLOW_FRAME = "thick solid yellow";

const BUTTON_STATE_STOP   = "Stop";
const BUTTON_STATE_RELOAD = "Reload";

var stopReloadButtonState = BUTTON_STATE_STOP;

var activeNode= "bing";
var activeApp= "bing";
var poll_period = 100;

var appsDescription = {
  bing: {
    app_id:       'app1',
    default_url:  'http://www.bing.com',
    control_name: 'bing.img',
    image_src:    './images/bing.png',
  },
  espn: {
    app_id:       'app2',
    default_url:  'http://www.espn.com',
    control_name: 'espn.img',
    image_src:    './images/espn.png',
  },
  imdb: {
    app_id:       'app3',
    default_url:  'http://www.imdb.com',
    control_name: 'imdb.img',
    image_src:    './images/imdb.png',
  },
  wiki: {
    app_id:       'app4',
    default_url:  'http://www.wikipedia.org',
    control_name: 'wiki.img',
    image_src:    './images/wiki.png',
  },
  yelp: {
    app_id:       'app5',
    default_url:  'http://www.yelp.com',
    control_name: 'yelp.img',
    image_src:    './images/yelp.png',
  },
  bbc: {
    app_id:       'app6',
    default_url:  'http://bbc.co.uk',
    control_name: 'bbc.img',
    image_src:    './images/bbc.png',
  },
  youtube: {
    app_id:       'app7',
    default_url:  'https://www.youtube.com/watch?v=YE7VzlLtp-4&t=1s',
    control_name: 'youtube.img',
    image_src:    './images/youtube.png',
  },
  hid: {
    app_id:       'app8',
    default_url:  'http://@@EMULATOR_SERVER_ADDRESS@@/hid.html',
    control_name: 'hid.img',
    image_src:    './images/hid.png',
  },
  auto: {
    app_id:       'app9',
    default_url:  'http://@@EMULATOR_SERVER_ADDRESS@@/auto_integration_test_page.html',
    control_name: 'auto.img',
    image_src:    './images/auto.png',
  },
  manual: {
    app_id:       'app10',
    default_url:  'http://@@EMULATOR_SERVER_ADDRESS@@/test_page.html',
    control_name: 'manual.img',
    image_src:    './images/manual.png',
  },
  htmlkb: {
    app_id:       'app12',
    default_url:  'http://@@EMULATOR_SERVER_ADDRESS@@/html_system_keyboard_test_page.html',
    control_name: 'htmlkb.img',
    image_src:    './images/htmlvkb.png',
  },
  browsercontrol: {
    app_id:       'app13',
    default_url:  'http://@@EMULATOR_SERVER_ADDRESS@@/browser_control_page.html',
    control_name: 'browsercontrol.img',
    image_src:    './images/browsercontrol.png',
  },
  vps: {
    app_id:       'app14',
    default_url:  'http://@@EMULATOR_SERVER_ADDRESS@@/viewport_size_test_page.html',
    control_name: 'vps.img',
    image_src:    './images/vps.png',
  },
  fbt: {
    app_id:       'app15',
    default_url:  'http://@@EMULATOR_SERVER_ADDRESS@@/fake_bold_text_test_page.html',
    control_name: 'fbt.img',
    image_src:    './images/fbt.png',
  },
  visibilitystate: {
    app_id:       'app16',
    default_url:  'http://@@EMULATOR_SERVER_ADDRESS@@/visibility_state.html',
    control_name: 'visibilitystate.img',
    image_src:    './images/vs.png',
  }
};
{
  let x = 100;
  let y = 100;
  for (let node in appsDescription) {
    appsDescription[node].width = 640;
    appsDescription[node].height = 480;
    appsDescription[node].pos_x = x;
    appsDescription[node].pos_y = y;
    appsDescription[node].url = appsDescription[node].default_url;
    appsDescription[node].zoom = 100;
    appsDescription[node].pid = 'n/a';
    appsDescription[node].injections = '';
    appsDescription[node].keyboard_visibility = 'n/a';
    appsDescription[node].viewport_width = 0;
    appsDescription[node].viewport_height = 0;
    x += 50;
    y += 50;
  }
}

window.onload = onLoad;

function getNodeByUrl(url) {
  for (let node in appsDescription) {
    if (appsDescription[node].url == url) {
      return node;
    }
  }
  return null;
}

function getNodeByAppId(id) {
  for (let node in appsDescription) {
    if (appsDescription[node].app_id == id) {
      return node;
    }
  }
  return null;
}

function setStopReloadButtonDisabledState(isDisabled) {
  document.getElementById('stopReloadButton').disabled = isDisabled;
}

function updateStopReloadButton(state) {
  if (stopReloadButtonState === state)
    return;

  stopReloadButtonState = state;
  let btn = document.getElementById('stopReloadButton');
  btn.innerText = state;
}

function callFunc(command, id) {
  let pos = command.indexOf(':');
  let arg;
  if (pos > 0) {
    arg = command.substring(pos + 1, command.length);
    command = command.substring(0, pos);
  }
  let node = getNodeByAppId(id);
  if (node == null) {
    console.log("Command <", command, "> has incorrect URL");
    return;
  }
  if (command == 'appClosed') {
    document.getElementById(appsDescription[node].control_name).
        style.borderBottom = NO_FRAME;
    appsDescription[node].pid = 'n/a';
    appsDescription[node].keyboard_visibility = 'n/a';
    if (node == activeNode) {
      document.getElementById('render_process_pid').value = 'n/a';
      document.getElementById('get_render_process_pid').value = '';
      document.getElementById('keyboard_visibility').value = 'n/a';
    }
    updateStopReloadButton(BUTTON_STATE_STOP);
    setStopReloadButtonDisabledState(true);
  } else if (command == 'appStarted') {
    document.getElementById(appsDescription[node].control_name).
        style.borderBottom = GREEN_FRAME;
    setStopReloadButtonDisabledState(false);
  } else if (command == 'zoomUpdated') {
    if (typeof(arg) != 'undefined') {
      appsDescription[node].zoom = parseInt(arg);
      if (node == activeNode)
        document.getElementById('zoom_factor').value = parseInt(arg) + '%';
    }
  } else if (command == 'keyboardVisibility') {
    appsDescription[node].keyboard_visibility = arg;
    if (node == activeNode) {
      document.getElementById('keyboard_visibility').value = arg;
    }
  } else if (command == 'canGoBackAbility') {
    if (node == activeNode) {
      document.getElementById('can_go_back').value = arg;
    }
  } else if (command == 'devToolsEndpoint') {
    if (node == activeNode) {
      var [host, port] = arg.split(",");
      if (parseInt(port) > 0) {
        host = (host == "unknown") ? kHost : host;
        document.getElementById("dev_tools_link").href = "http://" + host + ":" + port;
        document.getElementById("dev_tools_link").text = "http://" + host + ":" + port;
        document.getElementById('dev_tools_port').value = port;
      } else
        document.getElementById('dev_tools_port').value = "none";
    }
  } else if (command == 'pidUpdated') {
    if (typeof(arg) != 'undefined') {
      appsDescription[node].pid = arg;
      if (node == activeNode) {
        document.getElementById('render_process_pid').value = arg;
        document.getElementById('get_render_process_pid').value = '';
      }
    }
  } else if (command == 'pidRequested') {
    if (typeof(arg) != 'undefined' && node == activeNode)
      document.getElementById('get_render_process_pid').value = arg;
  } else if (command == 'userAgentIs') {
    if (typeof(arg) != 'undefined' && node == activeNode)
    document.getElementById('user_agent_output').value = arg;
  } else if (command == 'processGone') {
    document.getElementById(appsDescription[node].control_name).
        style.borderBottom = RED_FRAME;
    appsDescription[node].pid = 'n/a';
    if (node == activeNode) {
      document.getElementById('render_process_pid').value = 'n/a';
      document.getElementById('get_render_process_pid').value = '';
    }
  } else if (command == 'loadFailed') {
    document.getElementById(appsDescription[node].control_name).
        style.borderBottom = YELLOW_FRAME;
    updateStopReloadButton(BUTTON_STATE_RELOAD);
  } else if (command == 'loadFinished') {
    updateStopReloadButton(BUTTON_STATE_RELOAD);
  } else if (command == 'windowStateRequested') {
    if (typeof(arg) != 'undefined' && node == activeNode)
      document.getElementById('window_state_output').value = arg;
  } else if (command == 'windowStateAboutToChangeRequested') {
    if (typeof(arg) != 'undefined' && node == activeNode)
      document.getElementById('window_state_about_to_change_output').value = arg;
  }
}

function callFuncCallback(params_JSON) {
  let args = JSON.parse(params_JSON);
  if(args.hasOwnProperty('arg1') && args.hasOwnProperty('arg2')) {
    callFunc(args.arg1, args.arg2);
  } else {
    console.error("Invalid params for 'callFuncCallback()'.")
  }
}

function initEmulatorPage() {
  addURLForPolling(kWAM_callFunc, callFuncCallback);
}

function getFocusedElementURL() {
  if (!(activeNode in appsDescription)) {
    console.log("Element is not correct");
    return '';
  }
  return appsDescription[activeNode].url;
}

function getFocusedElementDefaultURL() {
  if (!(activeNode in appsDescription)) {
    console.log("Element is not correct");
    return '';
  }
  return appsDescription[activeNode].default_url;
}

function getFocusedElementDefaultImgSrc() {
  if (!(activeNode in appsDescription)) {
    console.log("Element is not correct");
    return '';
  }
  return appsDescription[activeNode].image_src;
}

function getFocusedElementAppId() {
  if (!(activeNode in appsDescription)) {
    console.log("Element is not correct");
    return '';
  }
  return appsDescription[activeNode].app_id;
}

function checkChangedURL() {
  if (getFocusedElementURL() != getFocusedElementDefaultURL()) {
    let nodes = document.getElementById(activeNode).getElementsByTagName("img");
    for(let i=0; i<nodes.length; i++) {
          nodes[i].src = './images/urlchanged.png';
    }
  } else {
    let nodes = document.getElementById(activeNode).getElementsByTagName("img");
    for(let i=0; i<nodes.length; i++) {
          nodes[i].src = getFocusedElementDefaultImgSrc();
    }
  }
}

function updateAppWindow() {
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    width: document.getElementById('launch_width_input').value,
    height: document.getElementById('launch_height_input').value,
    pos_x: appsDescription[activeNode].pos_x.toString(),
    pos_y: appsDescription[activeNode].pos_y.toString(),
    cmd: 'updateAppWindow'
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function updateWindowBackgroundColor() {
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    red: document.getElementById('red_channel_input').value,
    green: document.getElementById('green_channel_input').value,
    blue: document.getElementById('blue_channel_input').value,
    cmd: 'setBackgroundColor'
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function setUserAgent(agent) {
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    user_agent: document.getElementById('user_agent_input').value,
    cmd: 'setUserAgent'
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function setFontFamily(fontFamily) {
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    font_family: fontFamily,
    cmd: 'setFontFamily'
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function setVisibilityState(visibilityState) {
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    visibility_state: visibilityState,
    cmd: 'setVisibilityState'
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function setWindowProperty() {
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    name: document.getElementById("set_window_property_name_input").value,
    value: document.getElementById("set_window_property_value_input").value,
    cmd: 'setWindowProperty'
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function stopOrReload() {
  if (stopReloadButtonState == BUTTON_STATE_RELOAD) {
    runDirectCommand('reloadPage');
    updateStopReloadButton(BUTTON_STATE_STOP);
  }
  else if (stopReloadButtonState == BUTTON_STATE_STOP) {
    runDirectCommand('stopLoading');
    updateStopReloadButton(BUTTON_STATE_RELOAD);
  }
}

function resetToDefaultURL() {

  appsDescription[activeNode].url = getFocusedElementDefaultURL();
  document.getElementById("url_data_value_input").value = getFocusedElementDefaultURL();

  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    cmd: 'changeURL'
  };
  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
  checkChangedURL();
}

function changeURL() {
  if (getFocusedElementURL() == document.getElementById("url_data_value_input").value) {
    return;
  }

  appsDescription[activeNode].url = document.getElementById("url_data_value_input").value;
  let url = appsDescription[activeNode].url;
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    cmd: 'changeURL'
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
  checkChangedURL();
}

function replaceBaseURL() {
  appsDescription[activeNode].url = document.getElementById("url_data_value_input").value;
  let url = appsDescription[activeNode].url;
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    cmd: 'replaceBaseURL'
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
  checkChangedURL();
}

function enableSizePos(enable) {
  disabled = !enable;
  let names = [
    'launch_width_input', 'launch_height_input'
  ];
  for (let name of names) {
    document.getElementById(name).disabled = disabled;
  }
}

function onFullScreenChanged()  {
  enableSizePos(!document.getElementById('full_screen_cb').checked);
}

function prepareLaunchAppParams(command) {
  command.width = document.getElementById('launch_width_input').value;
  command.height = document.getElementById('launch_height_input').value;
    let node = getNodeByUrl(command.url);
    command.pos_x = appsDescription[node].pos_x.toString();
    command.pos_y = appsDescription[node].pos_y.toString();
  command.full_screen =
    document.getElementById('full_screen_cb').checked.toString();
  command.frameless_window =
    document.getElementById('frameless_window_cb').checked.toString();
  command.injections =
    document.getElementById('injections_input').value;
  command.viewport_width = document.getElementById('viewport_width_input').value;
  command.viewport_height = document.getElementById('viewport_height_input').value;
}

function runDirectCommand(cmd, appId) {
  return runDirectCommandWithArgs(cmd, {}, appId);
}

function runDirectCommandWithArgs(cmd, args, appId) {
  let url;

  if (typeof(appId) == 'undefined')
    appId = getFocusedElementAppId();

  for (let node in appsDescription) {
    if (appsDescription[node].app_id == appId) {
      url = appsDescription[node].url;
    }
  }

  let command = args;
  command.app_id = appId;
  command.url = url;
  command.cmd = cmd;

  if (cmd == 'launchApp' || cmd == 'launchHiddenApp') {
    prepareLaunchAppParams(command);
  }
  console.log('command = ', command);
  console.log('JL:: command = ', command);
  console.log('JL:: JSON.stringify = ', JSON.stringify(command));
  createExpectation(kWAM_commandSet, JSON.stringify(command));
  checkChangedURL();
}

function initStopReloadButton() {
  updateStopReloadButton(BUTTON_STATE_STOP);
  setStopReloadButtonDisabledState(true);
}

function onLoad() {
  initEmulatorPage();
  initStopReloadButton();
  setTimeout(periodicPoll, poll_period);
  onClick('bing');
  document.getElementById("url_data_value_input").value = getFocusedElementURL();
  document.getElementById('zoom_factor').value = appsDescription[activeNode].zoom + '%';
  document.getElementById('render_process_pid').value = appsDescription[activeNode].pid;
  document.getElementById('keyboard_visibility').value = appsDescription[activeNode].keyboard_visibility;

  var xhr = new XMLHttpRequest();
  xhr.open('GET', kEmulatorBaseURL + "__IP__", true);

  xhr.onload = function() {
    if (this.status == 200) {
      var localIPAddress = this.responseText + ":" + kPort;
      for (let node in appsDescription) {
        if(appsDescription[node].default_url.indexOf("@@EMULATOR_SERVER_ADDRESS@@") != -1) {
          appsDescription[node].url = appsDescription[node].default_url =
            appsDescription[node].default_url.replace("@@EMULATOR_SERVER_ADDRESS@@", localIPAddress);
        }
      }
    }
  };

  xhr.onerror = function() {
    reject(new Error("Network Error"));
  };

  xhr.send();
}

function groupCmd(cmd) {
  for (let node in appsDescription) {
    runDirectCommand(cmd, appsDescription[node].app_id);
  }
}

function onClick(id) {
  document.getElementById(activeNode).style.border = NO_FRAME;
  activeNode = id;
  activeApp = appsDescription[id].control_name;
  document.getElementById(activeNode).style.border = BLUE_FRAME;
  document.getElementById('launch_width_input').value =
      appsDescription[activeNode].width;
  document.getElementById('launch_height_input').value =
      appsDescription[activeNode].height;
  console.log("new element focused:" + id);
  document.getElementById("url_data_value_input").value = getFocusedElementURL();
  document.getElementById('zoom_factor').value = appsDescription[activeNode].zoom + '%';
  document.getElementById('render_process_pid').value = appsDescription[activeNode].pid;
  document.getElementById('keyboard_visibility').value = appsDescription[activeNode].keyboard_visibility;
  document.getElementById('get_render_process_pid').value = '';
  document.getElementById('red_channel_input').value = 29;
  document.getElementById('green_channel_input').value = 29;
  document.getElementById('blue_channel_input').value = 29;
  document.getElementById('injections_input').value =
      appsDescription[activeNode].injections;
  document.getElementById('viewport_width_input').value =
      appsDescription[activeNode].viewport_width;
  document.getElementById('viewport_height_input').value =
      appsDescription[activeNode].viewport_height;
}

function onUpdateWidth() {
  appsDescription[activeNode].width = document.getElementById('launch_width_input').value;
}

function onUpdateHeight() {
  appsDescription[activeNode].height = document.getElementById('launch_height_input').value;
}

function onZoomDown() {
  if (appsDescription[activeNode].zoom >= 40) {
    appsDescription[activeNode].zoom -= 20;
    document.getElementById('zoom_factor').value = appsDescription[activeNode].zoom + '%';
    let zoom_factor = document.getElementById('zoom_factor').value;
    let command = {
      app_id: getFocusedElementAppId(),
      url: appsDescription[activeNode].url,
      zoom: zoom_factor.slice(0, -1),
      cmd: 'updateZoom'
    };

    console.log('command = ', command);
    createExpectation(kWAM_commandSet, JSON.stringify(command));
  }
}

function onZoomUp() {
  if (appsDescription[activeNode].zoom <= 280) {
    appsDescription[activeNode].zoom += 20;
    document.getElementById('zoom_factor').value = appsDescription[activeNode].zoom + '%';
    let zoom_factor = document.getElementById('zoom_factor').value;
    let command = {
      app_id: getFocusedElementAppId(),
      url: appsDescription[activeNode].url,
      zoom: zoom_factor.slice(0, -1),
      cmd: 'updateZoom'
    };

    console.log('command = ', command);
    createExpectation(kWAM_commandSet, JSON.stringify(command));
  }
}

function loadInjections() {
  if (getFocusedElementURL() == document.getElementById("injections_input").value) {
    return;
  }

  appsDescription[activeNode].injections = document.getElementById("injections_input").value;
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    injections: appsDescription[activeNode].injections,
    cmd: 'loadInjections'
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function runJavaScript() {
  runDirectCommandWithArgs('runJavaScript', {
    'javascript_code': document.getElementById('javascript_code').value
  })
}

function setInspectable(enable) {
  runDirectCommandWithArgs('setInspectable', { 'enable': enable.toString() });

  if (!enable) {
    document.getElementById('dev_tools_port').value = 'none';
    document.getElementById('dev_tools_link').href = '#';
    document.getElementById('dev_tools_link').text = '...';
  }
}

function setWindowState(state) {
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    window_state: state,
    cmd: 'setWindowState'
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function setProxyServer() {
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    server: document.getElementById("proxy_server_name").value,
    port: document.getElementById("proxy_server_port").value,
    cmd: 'setProxyServer'
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function setAdditionalHeader() {
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    header: document.getElementById("header").value,
    value: document.getElementById("value").value,
    cmd: 'setAdditionalHeader'
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function setHardwareResolution() {
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    resolution_width: document.getElementById('resolution_width_input').value,
    resolution_height: document.getElementById('resolution_height_input').value,
    cmd: 'setHardwareResolution'
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function setBoardType() {
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    board_type: document.getElementById("set_board_type_input").value,
    cmd: 'setBoardType'
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function addUserStyleSheet() {
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    user_stylesheet: document.getElementById("user_style_sheet").value,
    cmd: 'addUserStyleSheet'
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function resizeWindow() {
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    window_width: document.getElementById('window_width_input').value,
    window_height: document.getElementById('window_height_input').value,
    cmd: 'resizeWindow'
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function onXInputActivate() {
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    cmd: 'xinputActivate'
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function onXInputDeactivate() {
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    cmd: 'xinputDeactivate'
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function onXInputInvokeAction() {
  let key = document.getElementById('XInputSel').value;
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    cmd: 'xinputInvokeAction',
    key_sym: key
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function setOpacity() {
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    opacity: document.getElementById('opacity_input').value,
    cmd: 'setOpacity'
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function setScaleFactor() {
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    scale_factor: document.getElementById('scale_factor_input').value,
    cmd: 'setScaleFactor'
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function setMediaCodecCapability() {
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    media_codec_capability: document.getElementById('media_codec_capability_json').value,
    cmd: 'setMediaCodecCapability'
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function setGroupKeyMask() {
  let selector = document.getElementById('group_key_mask_selector'), selector_option;
  let masks = 0;

  for (let i = 0; i < selector.length; i++) {
    selector_option = selector.options[i];
    if (selector_option.selected) {
      masks = masks | selector_option.value;
    }
  }

  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    cmd: 'setGroupKeyMask',
    key_mask: masks.toString()
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function setKeyMask(set) {
  let mask = document.getElementById('key_mask_selector').value;
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    cmd: 'setKeyMask',
    key_mask: mask,
    set: set
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}

function setInputRegion() {
  let region = document.getElementById('input_region_selector').value;
  let command = {
    app_id: getFocusedElementAppId(),
    url: appsDescription[activeNode].url,
    cmd: 'setInputRegion',
    input_region: region
  };

  console.log('command = ', command);
  createExpectation(kWAM_commandSet, JSON.stringify(command));
}
