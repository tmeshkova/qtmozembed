var component;
var sprite;

function createSpriteObjects() {
    component = Qt.createComponent(mozContext.getenv("QTTESTPATH") + "/auto/ViewComponent.qml");
    if (component.status == Component.Ready) {
        finishCreation();
    }
    else {
        component.statusChanged.connect(finishCreation);
    }
}

function finishCreation() {
    if (component.status == Component.Ready) {
        appWindow.mozView = component.createObject(appWindow, {"x": 0, "y": 0});
        if (appWindow.mozView == null) {
            // Error Handling
            console.log("Error creating object");
        }
    } else if (component.status == Component.Error) {
        // Error Handling
        console.log("Error loading component:", component.errorString());
    }
}

function waitMozContext() {
    if (mozContext.instance === undefined) {
        return false;
    }
    if (mozContext.instance.initialized()) {
        return true;
    }
    while (!mozContext.instance.initialized()) {
        mozContext.waitLoop();
    }
    return true;
}

function waitMozView() {
    if (appWindow.mozViewInitialized) {
        return true;
    }
    while (!appWindow.mozViewInitialized) {
        mozContext.waitLoop();
    }
    return true;
}

function waitLoadFinished(view) {
    if (!view.child.loading && view.child.loadProgress !== 0) {
        return true;
    }
    while (view.child.loading || view.child.loadProgress === 0) {
        mozContext.waitLoop();
    }
    return true;
}

function dumpTs(message) {
    print("TimeStamp:" + message + ", " + Qt.formatTime(new Date(), "hh:mm:ss::ms") + "\n");
}

function scrollBy(startX, startY, dx, dy, timeMs, isKinetic)
{
    var frameMs = 16;
    var timeMsStep = timeMs / frameMs;
    var stepRX = dx / timeMsStep;
    var stepRY = dy / timeMsStep;
    var curRX = startX;
    var curRY = startY;
    var endRX = curRX + dx;
    var endRY = curRY + dy;
    testcaseid.mousePress(webViewport, curRX, curRY, 1);
    while (curRX != endRX || curRY != endRY) {
        curRX = stepRX > 0 ? Math.min(curRX + stepRX, endRX) : Math.max(curRX + stepRX, endRX);
        curRY = stepRY > 0 ? Math.min(curRY + stepRY, endRY) : Math.max(curRY + stepRY, endRY);
        testcaseid.mouseMove(webViewport, curRX, curRY, -1, 1);
    }
    testcaseid.mouseRelease(webViewport, curRX, curRY, 1);
    testcaseid.mousePress(webViewport, curRX, curRY, 1);
    testcaseid.mouseRelease(webViewport, curRX, curRY, 1);
}
