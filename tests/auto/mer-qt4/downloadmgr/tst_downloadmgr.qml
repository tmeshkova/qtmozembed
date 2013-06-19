import QtQuickTest 1.0
import QtQuick 1.0
import Sailfish.Silica 1.0
import QtMozilla 1.0
import "../../shared/componentCreation.js" as MyScript
import "../../shared/sharedTests.js" as SharedTests

ApplicationWindow {
    id: appWindow

    property bool mozViewInitialized : false
    property variant promptReceived : null

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
        onRecvObserve: {
            if (message == "embed:download") {
                // print("onRecvObserve: msg:" + message + ", dmsg:" + data.msg);
                if (data.msg == "dl-done") {
                    appWindow.promptReceived = true;
                }
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
                webViewport.child.addMessageListener("embed:filepicker");
                appWindow.mozViewInitialized = true
            }
            onRecvAsyncMessage: {
                // print("onRecvAsyncMessage:" + message + ", data:" + data)
                if (message == "embed:filepicker") {
                    webViewport.child.sendAsyncMessage("filepickerresponse", {
                                                     winid: data.winid,
                                                     accepted: true,
                                                     items: ["/tmp/tt.bin"]
                                                 })
                }
            }
        }
    }

    resources: TestCase {
        id: testcaseid
        name: "mozContextPage"
        when: windowShown

        function cleanup() {
            mozContext.dumpTS("tst_downloadmgr cleanup")
        }

        function test_TestDownloadMgrPage()
        {
            mozContext.dumpTS("test_TestLoginMgrPage start")
            testcaseid.verify(MyScript.waitMozContext())
            mozContext.instance.setPref("browser.download.folderList", 2); // 0 - Desktop, 1 - Downloads, 2 - Custom
            mozContext.instance.setPref("browser.download.useDownloadDir", false); // Invoke filepicker instead of immediate download to ~/Downloads
            mozContext.instance.setPref("browser.download.manager.retention", 2);
            mozContext.instance.setPref("browser.helperApps.deleteTempFileOnExit", false);
            mozContext.instance.setPref("browser.download.manager.quitBehavior", 1);
            mozContext.instance.addObserver("embed:download");
            testcaseid.verify(MyScript.waitMozView())
            appWindow.promptReceived = null
            webViewport.child.url = "about:mozilla";
            testcaseid.verify(MyScript.waitLoadFinished(webViewport))
            testcaseid.compare(webViewport.child.loadProgress, 100);
            while (!webViewport.child.painted) {
                testcaseid.wait();
            }
            webViewport.child.url = mozContext.getenv("QTTESTSLOCATION") + "/downloadmgr/tt.bin";
            testcaseid.verify(MyScript.waitLoadFinished(webViewport))
            testcaseid.compare(webViewport.child.loadProgress, 100);
            while (!appWindow.promptReceived) {
                testcaseid.wait();
            }
            mozContext.dumpTS("test_TestDownloadMgrPage end");
        }
    }
}
