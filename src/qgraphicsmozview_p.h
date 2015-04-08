/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef qgraphicsmozview_p_h
#define qgraphicsmozview_p_h

#include <QColor>
#include <QImage>
#include <QSize>
#include <QTime>
#include <QString>
#include <QPointF>
#include <QMap>
#include <QSGSimpleTextureNode>
#include "qmozscrolldecorator.h"
#include "mozilla/embedlite/EmbedLiteView.h"
#include "qmozview_templated_wrapper.h"
#include "qmozview_defined_wrapper.h"

class QGraphicsView;
class QTouchEvent;
class QMozContext;

class QGraphicsMozViewPrivate : public mozilla::embedlite::EmbedLiteViewListener
{
public:
    QGraphicsMozViewPrivate(IMozQViewIface* aViewIface, QObject* publicPtr);
    virtual ~QGraphicsMozViewPrivate();

    void ReceiveInputEvent(const mozilla::InputData& event);
    void touchEvent(QTouchEvent* event);
    void UpdateViewSize();
    bool RequestCurrentGLContext(QSize&);
    virtual bool RequestCurrentGLContext();
    virtual void ViewInitialized();
    virtual void SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    virtual void CompositingFinished();
    virtual void OnLocationChanged(const char* aLocation, bool aCanGoBack, bool aCanGoForward);
    virtual void OnLoadProgress(int32_t aProgress, int32_t aCurTotal, int32_t aMaxTotal);
    virtual void OnLoadStarted(const char* aLocation);
    virtual void OnLoadFinished(void);
    virtual void OnWindowCloseRequested();

    // View finally destroyed and deleted
    virtual void ViewDestroyed();
    virtual void RecvAsyncMessage(const char16_t* aMessage, const char16_t* aData);
    virtual char* RecvSyncMessage(const char16_t* aMessage, const char16_t*  aData);
    virtual void OnLoadRedirect(void);
    virtual void OnSecurityChanged(const char* aStatus, unsigned int aState);
    virtual void OnFirstPaint(int32_t aX, int32_t aY);
    virtual void GetIMEStatus(int32_t* aIMEEnabled, int32_t* aIMEOpen, intptr_t* aNativeIMEContext);
    virtual void IMENotification(int aIstate, bool aOpen, int aCause, int aFocusChange, const char16_t* inputType, const char16_t* inputMode);

    virtual void OnScrolledAreaChanged(unsigned int aWidth, unsigned int aHeight);
    virtual void OnScrollChanged(int32_t offSetX, int32_t offSetY);
    virtual void OnTitleChanged(const char16_t* aTitle);
    virtual void SetFirstPaintViewport(const nsIntPoint& aOffset, float aZoom,
                                       const nsIntRect& aPageRect, const gfxRect& aCssPageRect);
    virtual void SyncViewportInfo(const nsIntRect& aDisplayPort,
                                  float aDisplayResolution, bool aLayersUpdated,
                                  nsIntPoint& aScrollOffset, float& aScaleX, float& aScaleY);
    virtual void SetPageRect(const gfxRect& aCssPageRect);
    virtual bool SendAsyncScrollDOMEvent(const gfxRect& aContentRect, const gfxSize& aScrollableSize);
    virtual bool HandleLongTap(const nsIntPoint& aPoint);
    virtual bool HandleSingleTap(const nsIntPoint& aPoint);
    virtual bool HandleDoubleTap(const nsIntPoint& aPoint);
    virtual void SetIsFocused(bool aIsFocused);
    virtual void CompositorCreated();

    void UpdateScrollArea(unsigned int aWidth, unsigned int aHeight, float aPosX, float aPosY);
    void TestFlickingMode(QTouchEvent *event);
    void HandleTouchEnd(bool& draggingChanged, bool& pinchingChanged);
    void ResetState();
    void UpdateMoving(bool moving);
    void ResetPainted();

    void load(const QString &url);
    void loadFrameScript(const QString &frameScript);
    void addMessageListener(const QString &name);
    void addMessageListeners(const QStringList &messageNamesList);

    void startMoveMonitor();
    void timerEvent(QTimerEvent *event);

    IMozQViewIface* mViewIface;
    QScopedPointer<QObject> q;
    QMozContext* mContext;
    mozilla::embedlite::EmbedLiteView* mView;
    bool mViewInitialized;
    QColor mBgColor;
    QImage mTempBufferImage;
    QSGTexture* mTempTexture;
    bool mEnabled;
    bool mChromeGestureEnabled;
    qreal mChromeGestureThreshold;
    bool mChrome;
    qreal mMoveDelta;
    qreal mDragStartY;
    bool mMoving;
    bool mPinching;
    QSizeF mSize;
    qint64 mLastTimestamp;
    qint64 mLastStationaryTimestamp;
    QPointF mLastPos;
    QPointF mLastStationaryPos;
    QMap<int, QPointF> mActiveTouchPoints;
    bool mCanFlick;
    bool mPendingTouchEvent;
    QString mLocation;
    QString mTitle;
    int mProgress;
    bool mCanGoBack;
    bool mCanGoForward;
    bool mIsLoading;
    bool mLastIsGoodRotation;
    bool mIsPasswordField;
    bool mGraphicsViewAssigned;
    QRectF mContentRect;
    QSizeF mScrollableSize;
    QPointF mScrollableOffset;
    // Non visual
    QMozScrollDecorator mVerticalScrollDecorator;
    QMozScrollDecorator mHorizontalScrollDecorator;
    float mContentResolution;
    bool mIsPainted;
    Qt::InputMethodHints mInputMethodHints;
    bool mIsInputFieldFocused;
    bool mViewIsFocused;
    bool mHasContext;
    QSize mGLSurfaceSize;
    bool mPressed;
    bool mDragging;
    bool mFlicking;
    // Moving monitoring
    int mMovingTimerId;
    qreal mOffsetX;
    qreal mOffsetY;

    QString mPendingUrl;
    QStringList mPendingMessageListeners;
    QStringList mPendingFrameScripts;
};

qint64 current_timestamp(QTouchEvent*);

#endif /* qgraphicsmozview_p_h */
