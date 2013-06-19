import QtQuickTest 1.0
import QtQuick 1.0
import Sailfish.Silica 1.0
import QtMozilla 1.0
import "../../shared/componentCreation.js" as MyScript
import "../../shared/sharedTests.js" as SharedTests

ApplicationWindow {
    id: appWindow

    property bool mozViewInitialized : false
    property string testResult : ""

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
            mozContext.instance.addComponentManifest(mozContext.getenv("QTTESTSROOT") + "/components/TestHelpers.manifest");
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
                webViewport.child.addMessageListener("testembed:elementinnervalue");
            }
            onHandleSingleTap: {
                print("HandleSingleTap: [",point.x,",",point.y,"]");
            }
            onRecvAsyncMessage: {
                // print("onRecvAsyncMessage:" + message + ", data:" + data)
                switch (message) {
                case "testembed:elementinnervalue": {
                    // print("testembed:elementpropvalue value:" + data.value);
                    appWindow.testResult = data.value;
                    break;
                }
                default:
                    break;
                }

            }
        }
    }

    resources: TestCase {
        id: testcaseid
        name: "mozContextPage"
        when: windowShown

        function cleanup() {
            mozContext.dumpTS("tst_multitouch cleanup")
        }

        function test_Test1MultiTouchPage()
        {
            mozContext.dumpTS("test_Test1MultiTouchPage start")
            testcaseid.verify(MyScript.waitMozContext())
            testcaseid.verify(MyScript.waitMozView())
            webViewport.child.url = mozContext.getenv("QTTESTSLOCATION") + "/multitouch/touch.html";
            testcaseid.verify(MyScript.waitLoadFinished(webViewport))
            testcaseid.compare(webViewport.child.loadProgress, 100);
            while (!webViewport.child.painted) {
                testcaseid.wait();
            }
            var params = [Qt.point(50,50), Qt.point(51,51), Qt.point(52,52)];
            webViewport.child.synthTouchBegin(params);
            params = [Qt.point(51,51), Qt.point(52,52), Qt.point(53,53)];
            webViewport.child.synthTouchMove(params);
            params = [Qt.point(52,52), Qt.point(53,53), Qt.point(54,54)];
            webViewport.child.synthTouchEnd(params);
            webViewport.child.sendAsyncMessage("embedtest:getelementinner", {
                                                name: "result" })
            while (appWindow.testResult == "") {
                testcaseid.wait();
            }
            testcaseid.compare(appWindow.testResult, "ok");
            mozContext.dumpTS("test_Test1MultiTouchPage end");
        }
    }
}
