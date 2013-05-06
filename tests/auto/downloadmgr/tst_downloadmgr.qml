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
            mozContext.instance.addComponentManifest(mozContext.getenv("QTTESTPATH") + "/components/TestHelpers.manifest");
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
            verify(MyScript.waitMozContext())
            mozContext.instance.setPref("browser.download.folderList", 2); // 0 - Desktop, 1 - Downloads, 2 - Custom
            mozContext.instance.setPref("browser.download.useDownloadDir", false); // Invoke filepicker instead of immediate download to ~/Downloads
            mozContext.instance.setPref("browser.download.manager.retention", 2);
            mozContext.instance.setPref("browser.helperApps.deleteTempFileOnExit", false);
            mozContext.instance.setPref("browser.download.manager.quitBehavior", 1);
            mozContext.instance.addObserver("embed:download");
            verify(MyScript.waitMozView())
            appWindow.promptReceived = null
            webViewport.child.url = "about:mozilla";
            verify(MyScript.waitLoadFinished(webViewport))
            compare(webViewport.child.loadProgress, 100);
            while (!webViewport.child.painted) {
                wait();
            }
            webViewport.child.url = mozContext.getenv("QTTESTPATH") + "/auto/downloadmgr/tt.bin";
            verify(MyScript.waitLoadFinished(webViewport))
            compare(webViewport.child.loadProgress, 100);
            while (!appWindow.promptReceived) {
                wait();
            }
            mozContext.dumpTS("test_TestDownloadMgrPage end");
        }
    }
}
