/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef qmozview_templated_wrapper_h
#define qmozview_templated_wrapper_h

class QPoint;
class QString;
class QMozReturnValue;
class IMozQViewIface
{
public:
    virtual ~IMozQViewIface() {}
    // Methods
    virtual bool Invalidate() = 0;
    virtual void CompositingFinished() = 0;
    virtual void setInputMethodHints(Qt::InputMethodHints hints) = 0;
    virtual void forceViewActiveFocus() = 0;
    virtual void createGeckoGLContext() = 0;
    virtual void requestGLContext(bool& hasContext, QSize& viewPortSize) = 0;
    virtual void startMoveMonitoring() = 0;
    // Signals
    virtual void viewInitialized() = 0;
    virtual void urlChanged() = 0;
    virtual void titleChanged() = 0;
    virtual void loadProgressChanged() = 0;
    virtual void navigationHistoryChanged() = 0;
    virtual void loadingChanged() = 0;
    virtual void viewDestroyed() = 0;
    virtual void recvAsyncMessage(const QString message, const QVariant data) = 0;
    virtual bool recvSyncMessage(const QString message, const QVariant data, QMozReturnValue* response) = 0;
    virtual void loadRedirect() = 0;
    virtual void securityChanged(QString status, uint state) = 0;
    virtual void firstPaint(int offx, int offy) = 0;
    virtual void contentLoaded(QString docuri) = 0;
    virtual void contentWidthChanged() = 0;
    virtual void contentHeightChanged() = 0;
    virtual void viewAreaChanged() = 0;
    virtual void scrollableOffsetChanged() = 0;
    virtual void chromeChanged() = 0;
    virtual void handleLongTap(QPoint point, QMozReturnValue* retval) = 0;
    virtual void handleSingleTap(QPoint point, QMozReturnValue* retval) = 0;
    virtual void handleDoubleTap(QPoint point, QMozReturnValue* retval) = 0;
    virtual void imeNotification(int state, bool open, int cause, int focusChange, const QString& type) = 0;
    virtual void bgColorChanged() = 0;
    virtual void useQmlMouse(bool value) = 0;
    virtual void draggingChanged() = 0;
    virtual void movingChanged() = 0;
    virtual void pinchingChanged() = 0;
};

template<class TMozQView>
class IMozQView : public IMozQViewIface
{
public:
    IMozQView(TMozQView& aView) : view(aView) {}

    bool Invalidate()
    {
        return view.Invalidate();
    }

    void CompositingFinished()
    {
        view.CompositingFinished();
    }

    void setInputMethodHints(Qt::InputMethodHints hints)
    {
        view.setInputMethodHints(hints);
    }

    void forceViewActiveFocus()
    {
        view.forceViewActiveFocus();
    }
    void createGeckoGLContext()
    {
        view.createGeckoGLContext();
    }
    void startMoveMonitoring() {
        view.startMoveMonitoring();
    }
    void viewInitialized()
    {
        Q_EMIT view.viewInitialized();
    }
    void urlChanged()
    {
        Q_EMIT view.urlChanged();
    }
    void titleChanged()
    {
        Q_EMIT view.titleChanged();
    }
    void loadProgressChanged()
    {
        Q_EMIT view.loadProgressChanged();
    }
    void navigationHistoryChanged()
    {
        Q_EMIT view.navigationHistoryChanged();
    }
    void loadingChanged()
    {
        Q_EMIT view.loadingChanged();
    }
    void viewDestroyed()
    {
        Q_EMIT view.viewDestroyed();
    }
    void recvAsyncMessage(const QString message, const QVariant data)
    {
        Q_EMIT view.recvAsyncMessage(message, data);
    }
    bool recvSyncMessage(const QString message, const QVariant data, QMozReturnValue* response)
    {
        return Q_EMIT view.recvSyncMessage(message, data, response);
    }
    void loadRedirect()
    {
        Q_EMIT view.loadRedirect();
    }
    void securityChanged(QString status, uint state)
    {
        Q_EMIT view.securityChanged(status, state);
    }
    void firstPaint(int offx, int offy)
    {
        Q_EMIT view.firstPaint(offx, offy);
    }
    void contentLoaded(QString docuri)
    {
        Q_EMIT view.contentLoaded(docuri);
    }
    void viewAreaChanged()
    {
        Q_EMIT view.viewAreaChanged();
    }
    void scrollableOffsetChanged()
    {
        Q_EMIT view.scrollableOffsetChanged();
    }
    void chromeChanged()
    {
        Q_EMIT view.chromeChanged();
    }
    void handleLongTap(QPoint point, QMozReturnValue* retval)
    {
        Q_EMIT view.handleLongTap(point, retval);
    }
    void handleSingleTap(QPoint point, QMozReturnValue* retval)
    {
        Q_EMIT view.handleSingleTap(point, retval);
    }
    void handleDoubleTap(QPoint point, QMozReturnValue* retval)
    {
        Q_EMIT view.handleDoubleTap(point, retval);
    }
    void imeNotification(int state, bool open, int cause, int focusChange, const QString& type)
    {
        Q_EMIT view.imeNotification(state, open, cause, focusChange, type);
    }
    void bgColorChanged()
    {
        Q_EMIT view.bgColorChanged();
    }
    void requestGLContext(bool& hasContext, QSize& viewPortSize)
    {
        view.requestGLContext(hasContext, viewPortSize);
    }
    void useQmlMouse(bool value)
    {
        Q_EMIT view.useQmlMouse(value);
    }

    void draggingChanged()
    {
        Q_EMIT view.draggingChanged();
    }

    void movingChanged()
    {
        Q_EMIT view.movingChanged();
    }

    void pinchingChanged()
    {
        Q_EMIT view.pinchingChanged();
    }

    void contentWidthChanged()
    {
        Q_EMIT view.contentWidthChanged();
    }

    void contentHeightChanged()
    {
        Q_EMIT view.contentHeightChanged();
    }

    TMozQView& view;
};

#endif /* qmozview_templated_wrapper_h */
