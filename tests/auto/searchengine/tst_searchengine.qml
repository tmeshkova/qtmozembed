import QtQuickTest 1.0
import QtQuick 1.0
import Sailfish.Silica 1.0
import QtMozilla 1.0
import "../componentCreation.js" as MyScript

ApplicationWindow {
    id: appWindow

    property string currentPageName: pageStack.currentPage != null
            ? pageStack.currentPage.objectName
            : ""

    property bool mozViewInitialized : false
    property variant testResult : null

    QmlMozContext {
        id: mozContext
    }
    Connections {
        target: mozContext.instance
        onOnInitialized: {
            // Gecko does not switch to SW mode if gl context failed to init
            // and qmlmoztestrunner does not build in GL mode
            // Let's put it here for now in SW mode always
            mozContext.instance.setIsAccelerated(true);
            mozContext.instance.addComponentManifest(mozContext.getenv("QTTESTPATH") + "/components/TestHelpers.manifest");
        }
        onRecvObserve: {
            if (message == "embed:search") {
                switch (data.msg) {
                    case "init": {
                        print("Received: search:" + message, ", msg: ", data.msg, data.defaultEngine);
                        if (!data.defaultEngine) {
                            mozContext.instance.sendObserve("embedui:search", {msg:"loadxml", uri:"chrome://embedlite/content/google.xml", confirm: false})
                        }
                    }
                    case "pluginslist": {
                        print("Received: search:" + message, ", msg: ", data.msg, data.list[0].name, data.list[0].isDefault, data.list[0].isCurrent);
                        appWindow.testResult = data.list[0].name;
                    }
                }
            } else if (message == "browser-search-engine-modified") {
                if (data == "engine-loaded") {
                    appWindow.testResult = "loaded";
                }
                print("Received: search mod:", data);
            }
        }
    }

    QmlMozView {
        id: webViewport
        visible: true
        focus: true
        anchors.fill: parent
        Connections {
            target: webViewport.child
            onViewInitialized: {
                appWindow.mozViewInitialized = true
            }
            onRecvAsyncMessage: {
                print("onRecvAsyncMessage:" + message + ", data:" + data)
            }
        }
    }

    resources: TestCase {
        id: testcaseid
        name: "mozContextPage"
        when: windowShown

        function cleanup() {
            mozContext.dumpTS("tst_searchengine cleanup")
        }

        function test_TestCheckDefaultSearch()
        {
            mozContext.dumpTS("TestCheckDefaultSearch start")
            verify(MyScript.waitMozContext())
            mozContext.instance.addObserver("browser-search-engine-modified");
            mozContext.instance.addObserver("embed:search");
            mozContext.instance.setPref("keyword.enabled", true);
            verify(MyScript.waitMozView())
            mozContext.instance.sendObserve("embedui:search", {msg:"loadxml", uri:"chrome://embedlite/content/google.xml", confirm: false})
            while (appWindow.testResult !== "loaded") {
                wait();
            }
            mozContext.instance.sendObserve("embedui:search", {msg:"getlist"})
            while (appWindow.testResult !== "Google") {
                wait();
            }
            webViewport.child.load("linux home");
            verify(MyScript.waitLoadFinished(webViewport))
            compare(webViewport.child.loadProgress, 100);
            while (!webViewport.child.painted) {
                wait();
            }
            compare(webViewport.child.url.toString().substr(0, 32), "https://www.google.com/search?q=")
            mozContext.dumpTS("TestCheckDefaultSearch end");
        }
    }
}
