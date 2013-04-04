var component;
var sprite;

function createSpriteObjects() {
     component = Qt.createComponent("../ViewComponent.qml");
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
         if (sprite == null) {
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

function waitLoadStarted(view) {
    if (view.child.loading) {
        return true;
    }
    while (!view.child.loading) {
        mozContext.waitLoop();
    }
    return true;
}

function waitLoadFinished(view) {
    if (!view.child.loading) {
        return true;
    }
    while (view.child.loading) {
        mozContext.waitLoop();
    }
    return true;
}

function dumpTs(message) {
    print("TimeStamp:" + message + ", " + Qt.formatTime(new Date(), "hh:mm:ss::ms") + "\n");
}
