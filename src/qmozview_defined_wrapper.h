#ifndef qmozview_defined_wrapper_h
#define qmozview_defined_wrapper_h

#include <QVariant>

class QMozVerticalScrollDecorator;
class QMozHorizontalScrollDecorator;

class QMozReturnValue : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant message READ getMessage WRITE setMessage FINAL)

public:
    QMozReturnValue(QObject* parent = 0) : QObject(parent) {}
    QMozReturnValue(const QMozReturnValue& aMsg) : QObject(NULL) { mMessage = aMsg.mMessage; }
    virtual ~QMozReturnValue() {}

    QVariant getMessage() const { return mMessage; }
    void setMessage(const QVariant& msg) { mMessage = msg; }

private:
    QVariant mMessage;
};

Q_DECLARE_METATYPE(QMozReturnValue)

#define Q_MOZ_RETURN_VALUE \
class QMozReturnValue : public QObject \
{ \
    Q_OBJECT \
    Q_PROPERTY(QVariant message READ getMessage WRITE setMessage FINAL) \
public: \
    QMozReturnValue(QObject* parent = 0) : QObject(parent) {} \
    QMozReturnValue(const QMozReturnValue& aMsg) : QObject(NULL) { mMessage = aMsg.mMessage; } \
    virtual ~QMozReturnValue() {} \
    QVariant getMessage() const { return mMessage; } \
    void setMessage(const QVariant& msg) { mMessage = msg; } \
private: \
    QVariant mMessage; \
}; \
Q_DECLARE_METATYPE(QMozReturnValue) \

#define Q_MOZ_VIEW_PRORERTIES \
    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged) \
    Q_PROPERTY(QString title READ title NOTIFY titleChanged) \
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY navigationHistoryChanged FINAL) \
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY navigationHistoryChanged FINAL) \
    Q_PROPERTY(int loadProgress READ loadProgress NOTIFY loadProgressChanged) \
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged FINAL) \
    Q_PROPERTY(QRectF contentRect READ contentRect NOTIFY viewAreaChanged FINAL) \
    Q_PROPERTY(qreal contentWidth READ contentWidth NOTIFY contentWidthChanged FINAL) \
    Q_PROPERTY(qreal contentHeight READ contentHeight NOTIFY contentHeightChanged FINAL) \
    Q_PROPERTY(QSizeF scrollableSize READ scrollableSize FINAL) \
    Q_PROPERTY(QPointF scrollableOffset READ scrollableOffset NOTIFY scrollableOffsetChanged FINAL) \
    Q_PROPERTY(float resolution READ resolution) \
    Q_PROPERTY(bool painted READ isPainted NOTIFY firstPaint FINAL) \
    Q_PROPERTY(QColor bgcolor READ bgcolor NOTIFY bgColorChanged FINAL) \
    Q_PROPERTY(bool useQmlMouse READ getUseQmlMouse WRITE setUseQmlMouse) \
    Q_PROPERTY(bool dragging READ dragging NOTIFY draggingChanged FINAL) \
    Q_PROPERTY(bool moving READ moving NOTIFY movingChanged FINAL) \
    Q_PROPERTY(bool pinching READ pinching NOTIFY pinchingChanged FINAL) \
    Q_PROPERTY(QMozVerticalScrollDecorator* verticalScrollDecorator READ verticalScrollDecorator FINAL) \
    Q_PROPERTY(QMozHorizontalScrollDecorator* horizontalScrollDecorator READ horizontalScrollDecorator FINAL) \
    Q_PROPERTY(bool chrome READ chrome WRITE setChrome NOTIFY chromeChanged FINAL) \
    Q_PROPERTY(bool chromeGestureEnabled READ chromeGestureEnabled WRITE setChromeGestureEnabled NOTIFY chromeGestureEnabledChanged FINAL) \
    Q_PROPERTY(qreal chromeGestureThreshold READ chromeGestureThreshold WRITE setChromeGestureThreshold NOTIFY chromeGestureThresholdChanged FINAL)

#define Q_MOZ_VIEW_PUBLIC_METHODS \
    QUrl url() const; \
    void setUrl(const QUrl&); \
    QString title() const; \
    int loadProgress() const; \
    bool canGoBack() const; \
    bool canGoForward() const; \
    bool loading() const; \
    QRectF contentRect() const; \
    qreal contentWidth() const; \
    qreal contentHeight() const; \
    QSizeF scrollableSize() const; \
    QPointF scrollableOffset() const; \
    float resolution() const; \
    bool isPainted() const; \
    QColor bgcolor() const; \
    bool getUseQmlMouse(); \
    void setUseQmlMouse(bool value); \
    void forceViewActiveFocus(); \
    void createGeckoGLContext(); \
    bool dragging() const; \
    bool moving() const; \
    bool pinching() const; \
    QMozVerticalScrollDecorator* verticalScrollDecorator() const; \
    QMozHorizontalScrollDecorator* horizontalScrollDecorator() const; \
    bool chromeGestureEnabled() const; \
    void setChromeGestureEnabled(bool value); \
    bool chrome() const; \
    void setChrome(bool value); \
    qreal chromeGestureThreshold() const; \
    void setChromeGestureThreshold(qreal value);

#define Q_MOZ_VIEW_PUBLIC_SLOTS \
    void loadHtml(const QString& html, const QUrl& baseUrl = QUrl()); \
    void goBack(); \
    void goForward(); \
    void stop(); \
    void reload(); \
    void load(const QString&); \
    void sendAsyncMessage(const QString& name, const QVariant& variant); \
    void addMessageListener(const QString& name); \
    void addMessageListeners(const QStringList& messageNamesList); \
    void loadFrameScript(const QString& name); \
    void newWindow(const QString& url = "about:blank"); \
    quint32 uniqueID() const; \
    void setParentID(unsigned aParentID); \
    void synthTouchBegin(const QVariant& touches); \
    void synthTouchMove(const QVariant& touches); \
    void synthTouchEnd(const QVariant& touches); \
    void scrollTo(const QPointF& position); \
    void suspendView(); \
    void resumeView(); \
    void recvMouseMove(int posX, int posY); \
    void recvMousePress(int posX, int posY); \
    void recvMouseRelease(int posX, int posY); \
    bool Invalidate(); \
    void CompositingFinished(); \
    void requestGLContext(bool& hasContext, QSize& viewPortSize);

#define Q_MOZ_VIEW_SIGNALS \
    void viewInitialized(); \
    void urlChanged(); \
    void titleChanged(); \
    void loadProgressChanged(); \
    void navigationHistoryChanged(); \
    void loadingChanged(); \
    void viewDestroyed(); \
    void recvAsyncMessage(const QString message, const QVariant data); \
    bool recvSyncMessage(const QString message, const QVariant data, QMozReturnValue* response); \
    void loadRedirect(); \
    void securityChanged(QString status, uint state); \
    void firstPaint(int offx, int offy); \
    void contentLoaded(QString docuri); \
    void viewAreaChanged(); \
    void scrollableOffsetChanged(); \
    void handleLongTap(QPoint point, QMozReturnValue* retval); \
    void handleSingleTap(QPoint point, QMozReturnValue* retval); \
    void handleDoubleTap(QPoint point, QMozReturnValue* retval); \
    void imeNotification(int state, bool open, int cause, int focusChange, const QString& type); \
    void bgColorChanged(); \
    void useQmlMouse(bool value); \
    void draggingChanged(); \
    void movingChanged(); \
    void pinchingChanged(); \
    void contentWidthChanged(); \
    void contentHeightChanged(); \
    void chromeGestureEnabledChanged(); \
    void chromeChanged(); \
    void chromeGestureThresholdChanged();

#endif /* qmozview_defined_wrapper_h */
