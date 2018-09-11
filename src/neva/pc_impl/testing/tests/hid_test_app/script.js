var log = undefined,
    ui = undefined;

var displayTimeout = null;
function display(e, p) {
    msg = e.type + " @ " + p.clientX + ", " + p.clientY;

    log.textContent += msg + '\n';

    console.log(msg);

    e.preventDefault();     // HACK: most lazy place for it

    clearTimeout(displayTimeout);
    displayTimeout = setTimeout(function () {   // log a newline during pauses
        log.textContent += '\n';
    }, 500);
}

var pointerEls = Object.create(null);
function pointer(id, p) {
    var el = pointerEls[id];
    if (p && !el) {
        el = document.createElement('div');
        el.className = 'point';
        el.textContent = id;
        ui.appendChild(el);
        pointerEls[id] = el;
    } else if (!p && el) {
        delete pointerEls[id];
        el.className += ' fade';
        setTimeout(function () {
            el.parentNode.removeChild(el);
        }, 500);
        return;
    }
    el.style.top = (p.clientY - el.clientHeight/2) + 'px';
    el.style.left = (p.clientX - el.clientWidth/2) + 'px';
}

var displayMouse = false;
var displayPointer = false;
function pointerdone(e) {
    if (!displayPointer) return;
    display(e,e);
    pointer(e.pointerId, null);
    displayPointer = false;
}

function justDisplay(e) { display(e,e); }

// https://www.w3.org/TR/DOM-Level-3-Events/#events-keyboardevents
function display_keyboard(e) {
    msg = e.type + " " + e.key;

    log.textContent += msg + '\n';
}


function initTest() {
    log = document.getElementById('log'),
    ui = document.getElementById('ui');

    // http://www.w3.org/TR/DOM-Level-3-Events/#events-mouseevents
    ui.addEventListener('mousedown', function (e) {
        display(e,e);
        pointer('m', e);
        displayMouse = true;
    }, false);
    window.addEventListener('mousemove', function (e) {
        if (!displayMouse) return;
        display(e,e);
        pointer('m', e);
    }, false);
    window.addEventListener('mouseup', function (e) {
        if (!displayMouse) return;
        display(e,e);
        pointer('m', null);
        displayMouse = false;
    }, false);

    // http://www.w3.org/Submission/pointer-events/
    ui.addEventListener('MSPointerDown', function (e) {
        display(e,e);
        pointer(e.pointerId, e);
        displayPointer = true;
    }, false);
    window.addEventListener('MSPointerMove', function (e) {
        if (!displayPointer) return;
        display(e,e);
        pointer(e.pointerId, e);
    }, false);

    window.addEventListener('MSPointerCancel', pointerdone, false);
    window.addEventListener('MSPointerUp', pointerdone, false);

    // http://www.w3.org/TR/touch-events/
    ui.addEventListener('touchstart', function (e) {
        [].forEach.call(e.changedTouches, function (p) {
            display(e,p);
            pointer(p.identifier, p);

            var pid = p.identifier,
                tgt = e.target;
            tgt.addEventListener('touchmove', touchmove, false);
            tgt.addEventListener('touchcancel', touchdone, false);
            tgt.addEventListener('touchend', touchdone, false);
            function touchmove(e) {
                var p = [].filter.call(e.changedTouches, function (p) { return p.identifier === pid; })[0];
                if (!p) return;
                display(e,p);
                pointer(pid,p);
            }
            function touchdone(e) {
                var p = [].filter.call(e.changedTouches, function (p) { return p.identifier === pid; })[0];
                if (!p) return;
                display(e,p);
                pointer(pid,null);
                tgt.removeEventListener('touchmove', touchmove, false);
                tgt.removeEventListener('touchcancel', touchdone, false);
                tgt.removeEventListener('touchend', touchdone, false);
            }
        });
    }, false);

    ui.addEventListener('gesturestart', justDisplay, false);
    ui.addEventListener('gestureend', justDisplay, false);
    ui.addEventListener('MSPointerOver', justDisplay, false);
    ui.addEventListener('MSPointerOut', justDisplay, false);
    ui.addEventListener('click', justDisplay, false);

    document.addEventListener('keydown', display_keyboard, false);
    document.addEventListener('keyup', display_keyboard, false);
}

window.addEventListener('load', initTest);