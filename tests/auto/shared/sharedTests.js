var component;

// Helper functions
function wrtWait(conditionFunc, waitIter, timeout)
{
    timeout = typeof timeout !== 'undefined' ? timeout : -1;
    waitIter = typeof waitIter !== 'undefined' ? waitIter : 5000;
    var tick = 0;
    while (conditionFunc() && tick++ < waitIter) {
        if (timeout == -1) {
            testcaseid.wait()
        }
        else {
            testcaseid.wait(timeout)
        }
    }
    return tick < waitIter;
}

// Shared Tests
function shared_context1Init()
{
    mozContext.dumpTS("test_context1Init start")
    testcaseid.verify(mozContext.instance !== undefined)
    testcaseid.verify(wrtWait(function() { return (mozContext.instance.initialized() === false); }, 10, 500))
    testcaseid.verify(mozContext.instance.initialized())
    mozContext.dumpTS("test_context1Init end")
}
function shared_context2AcceleratedAPI()
{
    mozContext.dumpTS("test_context2AcceleratedAPI start")
    mozContext.instance.setIsAccelerated(true);
    testcaseid.verify(mozContext.instance.isAccelerated() === true)
    mozContext.dumpTS("test_context2AcceleratedAPI end")
}
function shared_context3PrefAPI()
{
    mozContext.dumpTS("test_context3PrefAPI start")
    mozContext.instance.setPref("test.embedlite.pref", "result");
    mozContext.dumpTS("test_context3PrefAPI end")
}
function shared_context4ObserveAPI()
{
    mozContext.dumpTS("test_context4ObserveAPI start")
    mozContext.instance.sendObserve("memory-pressure", null);
    mozContext.instance.addObserver("test-observe-message");
    mozContext.instance.sendObserve("test-observe-message", {msg: "testMessage", val: 1});
    testcaseid.verify(wrtWait(function() { return (lastObserveMessage === undefined); }))
    testcaseid.compare(lastObserveMessage.msg, "test-observe-message");
    testcaseid.compare(lastObserveMessage.data.val, 1);
    testcaseid.compare(lastObserveMessage.data.msg, "testMessage");
    mozContext.dumpTS("test_context4ObserveAPI end")
}
function shared_Test1LoadInputPage()
{
    mozContext.dumpTS("test_Test1LoadInputPage start")
    testcaseid.verify(MyScript.waitMozContext())
    testcaseid.verify(MyScript.waitMozView())
    webViewport.child.url = "data:text/html,<input id=myelem value=''>";
    testcaseid.verify(MyScript.waitLoadFinished(webViewport))
    testcaseid.compare(webViewport.child.loadProgress, 100);
    testcaseid.verify(wrtWait(function() { return (!webViewport.child.painted); }))
    testcaseid.mouseClick(webViewport, 10, 10)
    testcaseid.verify(wrtWait(function() { return (!appWindow.isState(1, 0, 3)); }))
    appWindow.inputState = false;
    testcaseid.keyClick(Qt.Key_K);
    testcaseid.keyClick(Qt.Key_O);
    testcaseid.keyClick(Qt.Key_R);
    testcaseid.keyClick(Qt.Key_P);
    webViewport.child.sendAsyncMessage("embedtest:getelementprop", {
                                        name: "myelem",
                                        property: "value"
                                       })
    testcaseid.verify(wrtWait(function() { return (appWindow.inputContent == ""); }))
    testcaseid.compare(appWindow.inputContent, "korp");
    mozContext.dumpTS("test_Test1LoadInputPage end");
}

function shared_Test1LoadInputURLPage()
{
    mozContext.dumpTS("test_Test1LoadInputPage start")
    testcaseid.verify(MyScript.waitMozContext())
    testcaseid.verify(MyScript.waitMozView())
    appWindow.inputContent = ""
    appWindow.inputType = ""
    webViewport.child.url = "data:text/html,<input type=number id=myelem value=''>";
    testcaseid.verify(MyScript.waitLoadFinished(webViewport))
    testcaseid.compare(webViewport.child.loadProgress, 100);
    testcaseid.verify(wrtWait(function() { return (!webViewport.child.painted); }))
    testcaseid.mouseClick(webViewport, 10, 10)
    testcaseid.verify(wrtWait(function() { return (!appWindow.isState(1, 0, 3)); }))
    appWindow.inputState = false;
    testcaseid.keyClick(Qt.Key_1);
    testcaseid.keyClick(Qt.Key_2);
    testcaseid.keyClick(Qt.Key_3);
    testcaseid.keyClick(Qt.Key_4);
    webViewport.child.sendAsyncMessage("embedtest:getelementprop", {
                                        name: "myelem",
                                        property: "value"
                                       })
    testcaseid.verify(wrtWait(function() { return (appWindow.inputContent == ""); }))
    testcaseid.verify(wrtWait(function() { return (appWindow.inputType == ""); }))
    testcaseid.compare(appWindow.inputContent, "1234");
    mozContext.dumpTS("test_Test1LoadInputPage end");
}

function shared_TestScrollPaintOperations()
{
    mozContext.dumpTS("test_TestScrollPaintOperations start")
    testcaseid.verify(MyScript.waitMozContext())
    testcaseid.verify(MyScript.waitMozView())
    webViewport.child.url = "data:text/html,<body bgcolor=red leftmargin=0 topmargin=0 marginwidth=0 marginheight=0><input style='position:absolute; left:0px; top:1200px;'>";
    testcaseid.verify(MyScript.waitLoadFinished(webViewport))
    testcaseid.compare(webViewport.child.loadProgress, 100);
    testcaseid.verify(wrtWait(function() { return (!webViewport.child.painted); }))
    while (appWindow.scrollY === 0) {
        MyScript.scrollBy(100, 301, 0, -200, 100, false);
        testcaseid.wait(100);
    }
    testcaseid.verify(appWindow.scrollX === 0)
    while (appWindow.clickX === 0) {
        testcaseid.wait();
    }
    testcaseid.verify(appWindow.clickX === 100)
    testcaseid.verify(appWindow.clickY === 101)
    appWindow.clickX = 0
    testcaseid.mouseClick(webViewport, 10, 20)
    testcaseid.verify(wrtWait(function() { return (appWindow.clickX === 0); }))
    testcaseid.verify(appWindow.clickX === 10)
    testcaseid.verify(appWindow.clickY === 20)
    mozContext.dumpTS("test_TestScrollPaintOperations end");
}

function shared_1contextPrepareViewContext()
{
    mozContext.dumpTS("test_1contextPrepareViewContext start")
    testcaseid.verify(mozContext.instance !== undefined)
    testcaseid.verify(wrtWait(function() { return (mozContext.instance.initialized() === false); }, 10, 500))
    testcaseid.verify(mozContext.instance.initialized())
    mozContext.dumpTS("test_1contextPrepareViewContext end")
}

function shared_2viewInit(isQt5)
{
    mozContext.dumpTS("test_2viewInit start")
    testcaseid.verify(mozContext.instance.initialized())
    appWindow.createParentID = 0;
    if (isQt5)
        MyScript.createSpriteObjectsQt5();
    else
        MyScript.createSpriteObjects();
    testcaseid.verify(wrtWait(function() { return (mozView === undefined); }))
    testcaseid.verify(wrtWait(function() { return (mozViewInitialized !== true); }))
    testcaseid.verify(mozView.child !== undefined)
    mozContext.dumpTS("test_2viewInit end")
}
function shared_3viewLoadURL()
{
    mozContext.dumpTS("test_3viewLoadURL start")
    testcaseid.verify(mozView.child !== undefined)
    mozView.child.url = "about:mozilla";
    testcaseid.verify(MyScript.waitLoadFinished(mozView))
    testcaseid.compare(mozView.child.url, "about:mozilla")
    testcaseid.verify(wrtWait(function() { return (!mozView.child.painted); }))
    mozContext.dumpTS("test_3viewLoadURL end")
}

function shared_TestDownloadMgrPage()
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
    testcaseid.verify(wrtWait(function() { return (!webViewport.child.painted); }))
    webViewport.child.url = mozContext.getenv("QTTESTSLOCATION") + "/downloadmgr/tt.bin";
    testcaseid.verify(MyScript.waitLoadFinished(webViewport))
    testcaseid.compare(webViewport.child.loadProgress, 100);
    testcaseid.verify(wrtWait(function() { return (!appWindow.promptReceived); }))
    mozContext.dumpTS("test_TestDownloadMgrPage end");
}

function shared_TestFaviconPage()
{
    mozContext.dumpTS("test_TestFaviconPage start")
    testcaseid.verify(MyScript.waitMozContext())
    testcaseid.verify(MyScript.waitMozView())
    webViewport.child.url = mozContext.getenv("QTTESTSLOCATION") + "/favicons/favicon.html";
    testcaseid.verify(MyScript.waitLoadFinished(webViewport))
    testcaseid.compare(webViewport.child.loadProgress, 100);
    testcaseid.verify(wrtWait(function() { return (!webViewport.child.painted); }))
    testcaseid.verify(wrtWait(function() { return (!appWindow.favicon); }))
    testcaseid.compare(appWindow.favicon, "data:image/x-icon;base64,AAABAAEAEBAAAAAAAABoBQAAFgAAACgAAAAQAAAAIAAAAAEACAAAAAAAAAEAAAAAAAAAAAAAAAEAAAAAAAAAAAAADPH1AAwM9QAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAQEBAQAAAAAAAAAAAAAAAQICAgEAAAAAAAAAAAAAAQECAgIBAAAAAAAAAAAAAQICAgICAQAAAAAAAAAAAAEBAgIBAQEAAAAAAAAAAAEBAQICAQAAAAAAAAAAAAABAgIBAQEAAAAAAAAAAAAAAAEBAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAP/gAAD/4AAA58AAAPuAAAC9gAAA/QMAAL0DAAD9jwAA+/8AAOf/AAD//wAAqIAAAKuqAACJqgAAq6oAAKioAAA=");
    mozContext.dumpTS("test_TestFaviconPage");
}

function shared_Test1MultiTouchPage()
{
    mozContext.dumpTS("test_Test1MultiTouchPage start")
    testcaseid.verify(MyScript.waitMozContext())
    testcaseid.verify(MyScript.waitMozView())
    webViewport.child.url = mozContext.getenv("QTTESTSLOCATION") + "/multitouch/touch.html";
    testcaseid.verify(MyScript.waitLoadFinished(webViewport))
    testcaseid.compare(webViewport.child.loadProgress, 100);
    testcaseid.verify(wrtWait(function() { return (!webViewport.child.painted); }))
    var params = [Qt.point(50,50), Qt.point(51,51), Qt.point(52,52)];
    webViewport.child.synthTouchBegin(params);
    params = [Qt.point(51,51), Qt.point(52,52), Qt.point(53,53)];
    webViewport.child.synthTouchMove(params);
    params = [Qt.point(52,52), Qt.point(53,53), Qt.point(54,54)];
    webViewport.child.synthTouchEnd(params);
    webViewport.child.sendAsyncMessage("embedtest:getelementinner", {
                                        name: "result" })
    testcaseid.verify(wrtWait(function() { return (appWindow.testResult == ""); }))
    testcaseid.compare(appWindow.testResult, "ok");
    mozContext.dumpTS("test_Test1MultiTouchPage end");
}

function shared_1newcontextPrepareViewContext()
{
    mozContext.dumpTS("test_1newcontextPrepareViewContext start")
    testcaseid.verify(mozContext.instance !== undefined)
    testcaseid.verify(wrtWait(function() { return (mozContext.instance.initialized() === false); }, 10, 500))
    testcaseid.verify(mozContext.instance.initialized())
    mozContext.dumpTS("test_1newcontextPrepareViewContext end")
}
function shared_2newviewInit(isQt5)
{
    mozContext.dumpTS("test_2newviewInit start")
    testcaseid.verify(mozContext.instance.initialized())
    if (isQt5)
        MyScript.createSpriteObjectsQt5();
    else
        MyScript.createSpriteObjects();
    testcaseid.verify(wrtWait(function() { return (mozView === null); }, 10, 500))
    testcaseid.verify(wrtWait(function() { return (mozViewInitialized !== true); }, 10, 500))
    testcaseid.verify(mozView.child !== undefined)
    mozContext.dumpTS("test_2newviewInit end")
}
function shared_viewTestNewWindowAPI()
{
    mozContext.dumpTS("test_viewTestNewWindowAPI start")
    testcaseid.verify(mozView.child !== undefined)
    mozView.child.url = mozContext.getenv("QTTESTSLOCATION") + "/newviewrequest/newwin.html";
    testcaseid.verify(MyScript.waitLoadFinished(mozView))
    testcaseid.compare(mozView.child.title, "NewWinExample")
    testcaseid.verify(wrtWait(function() { return (!mozView.child.painted); }))
    mozViewInitialized = false;
    testcaseid.mouseClick(mozView, 10, 10)
    testcaseid.verify(wrtWait(function() { return (!mozView || !oldMozView); }))
    testcaseid.verify(wrtWait(function() { return (mozViewInitialized !== true); }))
    testcaseid.verify(mozView.child !== undefined)
    testcaseid.verify(MyScript.waitLoadFinished(mozView))
    testcaseid.verify(wrtWait(function() { return (!mozView.child.painted); }))
    testcaseid.compare(mozView.child.url, "about:mozilla")
    mozContext.dumpTS("test_viewTestNewWindowAPI end")
}

function shared_TestLoginMgrPage()
{
    mozContext.dumpTS("test_TestLoginMgrPage start")
    testcaseid.verify(MyScript.waitMozContext())
    testcaseid.verify(MyScript.waitMozView())
    appWindow.promptReceived = null
    webViewport.child.url = mozContext.getenv("QTTESTSLOCATION") + "/passwordmgr/subtst_notifications_1.html";
    testcaseid.verify(MyScript.waitLoadFinished(webViewport))
    testcaseid.compare(webViewport.child.loadProgress, 100);
    testcaseid.verify(wrtWait(function() { return (!webViewport.child.painted); }))
    testcaseid.verify(wrtWait(function() { return (!appWindow.promptReceived); }))
    mozContext.dumpTS("test_TestLoginMgrPage end");
}

function shared_TestPromptPage()
{
    mozContext.dumpTS("test_TestPromptPage start")
    testcaseid.verify(MyScript.waitMozContext())
    testcaseid.verify(MyScript.waitMozView())
    appWindow.testCaseNum = 0
    appWindow.promptReceived = null
    appWindow.testResult = null
    webViewport.child.url = mozContext.getenv("QTTESTSLOCATION") + "/promptbasic/prompt.html";
    testcaseid.verify(MyScript.waitLoadFinished(webViewport))
    testcaseid.compare(webViewport.child.loadProgress, 100);
    testcaseid.verify(wrtWait(function() { return (!webViewport.child.painted); }))
    testcaseid.verify(wrtWait(function() { return (!appWindow.promptReceived); }))
    webViewport.child.sendAsyncMessage("embedtest:getelementinner", {
                                        name: "result" })
    testcaseid.verify(wrtWait(function() { return (!appWindow.testResult); }))
    testcaseid.compare(appWindow.testResult, "ok");
    mozContext.dumpTS("test_TestPromptPage end");
}

function shared_TestPromptWithBadResponse()
{
    mozContext.dumpTS("test_TestPromptWithBadResponse start")
    testcaseid.verify(MyScript.waitMozContext())
    testcaseid.verify(MyScript.waitMozView())
    appWindow.testCaseNum = 1
    appWindow.promptReceived = null
    appWindow.testResult = null
    webViewport.child.url = mozContext.getenv("QTTESTSLOCATION") + "/promptbasic/prompt.html";
    testcaseid.verify(MyScript.waitLoadFinished(webViewport))
    testcaseid.compare(webViewport.child.loadProgress, 100);
    testcaseid.verify(wrtWait(function() { return (!webViewport.child.painted); }))
    testcaseid.verify(wrtWait(function() { return (!appWindow.promptReceived); }))
    webViewport.child.sendAsyncMessage("embedtest:getelementinner", {
                                        name: "result" })
    testcaseid.verify(wrtWait(function() { return (!appWindow.testResult); }))
    testcaseid.compare(appWindow.testResult, "failed");
    mozContext.dumpTS("test_TestPromptWithBadResponse end");
}

function shared_TestPromptWithoutResponse()
{
    mozContext.dumpTS("test_TestPromptWithoutResponse start")
    testcaseid.verify(MyScript.waitMozContext())
    testcaseid.verify(MyScript.waitMozView())
    appWindow.testCaseNum = 2
    appWindow.promptReceived = null
    appWindow.testResult = null
    webViewport.child.url = mozContext.getenv("QTTESTSLOCATION") + "/promptbasic/prompt.html";
    testcaseid.verify(MyScript.waitLoadFinished(webViewport))
    testcaseid.compare(webViewport.child.loadProgress, 100);
    testcaseid.verify(wrtWait(function() { return (!webViewport.child.painted); }))
    testcaseid.verify(wrtWait(function() { return (!appWindow.promptReceived); }))
    webViewport.child.sendAsyncMessage("embedtest:getelementinner", {
                                        name: "result" })
    testcaseid.verify(wrtWait(function() { return (!appWindow.testResult); }))
    testcaseid.compare(appWindow.testResult, "unknown");
    mozContext.dumpTS("test_TestPromptWithoutResponse end");
}

function shared_TestCheckDefaultSearch()
{
    mozContext.dumpTS("TestCheckDefaultSearch start")
    testcaseid.verify(MyScript.waitMozContext())
    mozContext.instance.setPref("browser.search.log", true);
    mozContext.instance.addObserver("browser-search-engine-modified");
    mozContext.instance.addObserver("embed:search");
    mozContext.instance.setPref("keyword.enabled", true);
    testcaseid.verify(MyScript.waitMozView())
    mozContext.instance.sendObserve("embedui:search", {msg:"remove", name: "QMOZTest"})
    mozContext.instance.sendObserve("embedui:search", {msg:"loadxml", uri: "file://" + mozContext.getenv("QTTESTSLOCATION") + "/searchengine/test.xml", confirm: false})
    testcaseid.verify(wrtWait(function() { return (appWindow.testResult !== "loaded"); }))
    mozContext.instance.sendObserve("embedui:search", {msg:"getlist"})
    testcaseid.verify(wrtWait(function() { return (appWindow.testResult !== "QMOZTest"); }))
    webViewport.child.load("linux home");
    testcaseid.verify(MyScript.waitLoadFinished(webViewport))
    testcaseid.compare(webViewport.child.loadProgress, 100);
    testcaseid.verify(wrtWait(function() { return (!webViewport.child.painted); }))
    testcaseid.compare(webViewport.child.url.toString().substr(0, 34), "https://webhook/?search=linux+home")
    mozContext.dumpTS("TestCheckDefaultSearch end");
}

function shared_SelectionInit()
{
    mozContext.dumpTS("test_SelectionInit start")
    testcaseid.verify(MyScript.waitMozContext())
    mozContext.instance.addObserver("clipboard:setdata");
    testcaseid.verify(MyScript.waitMozView())
    webViewport.child.url = "data:text/html,hello test selection";
    testcaseid.verify(MyScript.waitLoadFinished(webViewport))
    testcaseid.compare(webViewport.child.loadProgress, 100);
    testcaseid.verify(wrtWait(function() { return (!webViewport.child.painted); }))
    webViewport.child.sendAsyncMessage("Browser:SelectionStart", {
                                        xPos: 56,
                                        yPos: 16
                                      })
    webViewport.child.sendAsyncMessage("Browser:SelectionMoveStart", {
                                        change: "start"
                                      })
    webViewport.child.sendAsyncMessage("Browser:SelectionCopy", {
                                        xPos: 56,
                                        yPos: 16
                                      })
    testcaseid.verify(wrtWait(function() { return (appWindow.selectedContent == ""); }))
    testcaseid.compare(appWindow.selectedContent, "test");
    mozContext.dumpTS("test_SelectionInit end")
}

function shared_Test1LoadSimpleBlank()
{
    mozContext.dumpTS("test_Test1LoadSimpleBlank start")
    testcaseid.verify(MyScript.waitMozContext())
    testcaseid.verify(MyScript.waitMozView())
    webViewport.child.url = "about:blank";
    testcaseid.verify(MyScript.waitLoadFinished(webViewport))
    testcaseid.compare(webViewport.child.loadProgress, 100)
    testcaseid.verify(wrtWait(function() { return (!webViewport.child.painted); }))
    mozContext.dumpTS("test_Test1LoadSimpleBlank end")
}
function shared_Test2LoadAboutMozillaCheckTitle()
{
    mozContext.dumpTS("test_Test2LoadAboutMozillaCheckTitle start")
    webViewport.child.url = "about:mozilla";
    testcaseid.verify(MyScript.waitLoadFinished(webViewport))
    testcaseid.compare(webViewport.child.title, "The Book of Mozilla, 15:1")
    testcaseid.verify(wrtWait(function() { return (!webViewport.child.painted); }))
    mozContext.dumpTS("test_Test2LoadAboutMozillaCheckTitle end")
}
