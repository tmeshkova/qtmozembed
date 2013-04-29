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
    property string inputContent : ""
    property int inputState : -1
    property bool changed : false
    property int focusChange : -1
    property int cause : -1

    function isState(state, focus, cause)
    {
        return appWindow.changed === true && appWindow.inputState === state && appWindow.focusChange === focus && appWindow.cause === cause;
    }

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
    }

    QmlMozView {
        id: webViewport
        visible: true
        focus: true
        anchors.fill: parent
        Connections {
            target: webViewport.child
            onViewInitialized: {
                webViewport.child.loadFrameScript("chrome://tests/content/testHelper.js");
                appWindow.mozViewInitialized = true
                webViewport.child.addMessageListener("testembed:elementpropvalue");
            }
            onHandleSingleTap: {
                print("HandleSingleTap: [",point.x,",",point.y,"]");
            }
            onRecvAsyncMessage: {
                // print("onRecvAsyncMessage:" + message + ", data:" + data)
                switch (message) {
                case "testembed:elementpropvalue": {
                    // print("testembed:elementpropvalue value:" + data.value);
                    appWindow.inputContent = data.value;
                    break;
                }
                default:
                    break;
                }
            }
            onImeNotification: {
                // print("onImeNotification: state:" + state + ", open:" + open + ", cause:" + cause + ", focChange:" + focusChange + ", type:" + type)
                appWindow.changed = true
                appWindow.inputState = state
                appWindow.cause = cause
                appWindow.focusChange = focusChange
            }
        }
    }

    resources: TestCase {
        id: testcaseid
        name: "mozContextPage"
        when: windowShown

        function cleanup() {
            mozContext.dumpTS("tst_inputtest cleanup")
        }

        function test_Test1LoadInputPage()
        {
            mozContext.dumpTS("test_Test1LoadInputPage start")
            verify(MyScript.waitMozContext())
            verify(MyScript.waitMozView())
            webViewport.child.url = "data:text/html,<input id=myelem value=''>";
            verify(MyScript.waitLoadFinished(webViewport))
            compare(webViewport.child.loadProgress, 100);
            while (!webViewport.child.painted) {
                wait();
            }
            mouseClick(webViewport, 10, 10)
            while (!appWindow.isState(1, 0, 3)) {
                wait();
            }
            appWindow.inputState = false;
            keyClick(Qt.Key_K);
            keyClick(Qt.Key_O);
            keyClick(Qt.Key_R);
            keyClick(Qt.Key_P);
            webViewport.child.sendAsyncMessage("embedtest:getelementprop", {
                                                name: "myelem",
                                                property: "value"
                                               })
            while (appWindow.inputContent == "") {
                wait();
            }
            compare(appWindow.inputContent, "korp");
            mozContext.dumpTS("test_Test1LoadInputPage end");
        }
    }
}
