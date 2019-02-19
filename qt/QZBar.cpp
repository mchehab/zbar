//------------------------------------------------------------------------
//  Copyright 2008-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
//
//  This file is part of the ZBar Bar Code Reader.
//
//  The ZBar Bar Code Reader is free software; you can redistribute it
//  and/or modify it under the terms of the GNU Lesser Public License as
//  published by the Free Software Foundation; either version 2.1 of
//  the License, or (at your option) any later version.
//
//  The ZBar Bar Code Reader is distributed in the hope that it will be
//  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
//  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser Public License for more details.
//
//  You should have received a copy of the GNU Lesser Public License
//  along with the ZBar Bar Code Reader; if not, write to the Free
//  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
//  Boston, MA  02110-1301  USA
//
//  http://sourceforge.net/projects/zbar
//------------------------------------------------------------------------

#include <qevent.h>
#include <qurl.h>
#include <QX11Info>
#include <zbar/QZBar.h>
#include "QZBarThread.h"

using namespace zbar;

QZBar::QZBar (QWidget *parent, int verbosity)
    : QWidget(parent),
      thread(NULL),
      _videoDevice(),
      _videoEnabled(false),
      _attached(false)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_PaintOnScreen);
#if QT_VERSION >= 0x040400
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_DontCreateNativeAncestors);
#endif

    QSizePolicy sizing(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizing.setHeightForWidth(true);
    setSizePolicy(sizing);

    thread = new QZBarThread (verbosity);
    if(testAttribute(Qt::WA_WState_Created)) {
#if QT_VERSION >= 0x050000
        thread->window.attach(QX11Info::display(), winId());
#else
        thread->window.attach(x11Info().display(), winId());
#endif
        _attached = 1;
    }
    connect(thread, SIGNAL(videoOpened(bool)),
            this, SIGNAL(videoOpened(bool)));
    connect(this, SIGNAL(videoOpened(bool)),
            this, SLOT(sizeChange()));
    connect(thread, SIGNAL(update()),
            this, SLOT(update()));
    connect(thread, SIGNAL(decoded(int, const QString&)),
            this, SIGNAL(decoded(int, const QString&)));
    connect(thread, SIGNAL(decodedText(const QString&)),
            this, SIGNAL(decodedText(const QString&)));
    thread->start();
}

QZBar::~QZBar ()
{
    if(thread) {
        thread->pushEvent(new QEvent((QEvent::Type)QZBarThread::Exit));
        thread->wait();
        delete thread;
        thread = NULL;
    }
}

QPaintEngine *QZBar::paintEngine () const
{
    return(NULL);
}

QString QZBar::videoDevice () const
{
    return(_videoDevice);
}

void QZBar::setVideoDevice (const QString& videoDevice)
{
    if(!thread)
        return;
    if(_videoDevice != videoDevice) {
        _videoDevice = videoDevice;
        _videoEnabled = _attached && !videoDevice.isEmpty();
        if(_attached)
            thread->pushEvent(new QZBarThread::VideoDeviceEvent(videoDevice));
    }
}

int QZBar::get_controls(int index, char **name, char **group,
                        enum ControlType *type,
                        int *min, int *max, int *def, int *step)
{
    if(!thread)
        return 0;

    return thread->get_controls(index, name, group, type,
                                min, max, def, step);
}

void QZBar::request_size(unsigned width, unsigned height, bool trigger)
{
    if(!thread)
        return;

    thread->request_size(width, height);
    if (trigger)
        thread->pushEvent(new QEvent((QEvent::Type)QZBarThread::ReOpen));
}

int QZBar::get_resolution(int index, unsigned &width, unsigned &height, float &max_fps)
{
    if(!thread)
        return 0;

    return thread->get_resolution(index, width, height, max_fps);
}

unsigned QZBar::videoWidth()
{
    if(!thread)
        return 0;

    return thread->reqWidth;
}

unsigned QZBar::videoHeight()
{
    if(!thread)
        return 0;

    return thread->reqHeight;
}

QVector< QPair< int , QString > > QZBar::get_menu(int index)
{
    if(!thread) {
        QVector< QPair< int , QString > > empty;
        return empty;
    }

    return thread->get_menu(index);
}


int QZBar::set_control(char *name, bool value)
{
    if(!thread)
        return 0;

    return thread->set_control(name, value);
}

int QZBar::set_control(char *name, int value)
{
    if(!thread)
        return 0;

    return thread->set_control(name, value);
}

int QZBar::get_control(char *name, bool *value)
{
    if(!thread)
        return 0;

    return thread->get_control(name, value);
}

int QZBar::get_control(char *name, int *value)
{
    if(!thread)
        return 0;

    return thread->get_control(name, value);
}

int QZBar::set_config(std::string cfgstr)
{
    if(!thread)
        return 0;

    return thread->set_config(cfgstr);
}

int QZBar::set_config(zbar_symbol_type_t symbology,
                      zbar_config_t config,
                      int value)
{
    if(!thread)
        return 0;

    return thread->set_config(symbology, config, value);
}

int QZBar::get_config(zbar_symbol_type_t symbology,
                      zbar_config_t config,
                      int &value)
{
    if(!thread)
        return 0;

    return thread->get_config(symbology, config, value);
}

int QZBar::request_dbus(bool enabled)
{
    if(!thread)
        return 0;

    return thread->request_dbus(enabled);
}


bool QZBar::isVideoEnabled () const
{
    return(_videoEnabled);
}

void QZBar::setVideoEnabled (bool videoEnabled)
{
    if(!thread)
        return;
    if(_videoEnabled != videoEnabled) {
        _videoEnabled = videoEnabled;
        thread->pushEvent(new QZBarThread::VideoEnabledEvent(videoEnabled));
    }
}

bool QZBar::isVideoOpened () const
{
    if(!thread)
        return(false);
    QMutexLocker locker(&thread->mutex);
    return(thread->_videoOpened);
}

void QZBar::scanImage (const QImage &image)
{
    if(!thread)
        return;
    thread->pushEvent(new QZBarThread::ScanImageEvent(image));
}

void QZBar::dragEnterEvent (QDragEnterEvent *event)
{
    if(event->mimeData()->hasImage() ||
       event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void QZBar::dropEvent (QDropEvent *event)
{
    if(event->mimeData()->hasImage()) {
        QImage image = qvariant_cast<QImage>(event->mimeData()->imageData());
        scanImage(image);
        event->setDropAction(Qt::CopyAction);
        event->accept();
    }
    else {
        // FIXME TBD load URIs and queue for processing
#if 0
        std::cerr << "drop: "
                  << event->mimeData()->formats().join(", ").toStdString()
                  << std::endl;
        QList<QUrl> urls = event->mimeData()->urls();
        for(int i = 0; i < urls.size(); ++i)
            std::cerr << "[" << i << "] "
                      << urls.at(i).toString().toStdString()
                      << std::endl;
#endif
    }
}

QSize QZBar::sizeHint () const
{
    if(!thread)
        return(QSize(640, 480));
    QMutexLocker locker(&thread->mutex);
    return(QSize(thread->reqWidth, thread->reqHeight));
}

int QZBar::heightForWidth (int width) const
{
    if(thread) {
        QMutexLocker locker(&thread->mutex);
        int base_width = thread->reqWidth;
        int base_height = thread->reqHeight;
        if(base_width > 0 && base_height > 0)
            return(base_height * width / base_width);
    }
    return(width * 3 / 4);
}

void QZBar::paintEvent (QPaintEvent *event)
{
    try {
        if(thread)
            thread->window.redraw();
    }
    catch(Exception&) {
        // sometimes Qt attempts to paint the widget before it's parented(?)
        // just ignore this (can't throw from event anyway)
    }
}

void QZBar::resizeEvent (QResizeEvent *event)
{
    QSize size = event->size();
    try {
        if(thread)
            thread->window.resize(size.rwidth(), size.rheight());
    }
    catch(Exception&) { /* ignore */ }
}

void QZBar::changeEvent(QEvent *event)
{
    try {
        QMutexLocker locker(&thread->mutex);
        if(event->type() == QEvent::ParentChange)
#if QT_VERSION >= 0x050000
            thread->window.attach(QX11Info::display(), winId());
#else
            thread->window.attach(x11Info().display(), winId());
#endif

    }
    catch(Exception&) { /* ignore (FIXME do something w/error) */ }
}

void QZBar::attach ()
{
    if(_attached)
        return;

    try {
#if QT_VERSION >= 0x050000
        thread->window.attach(QX11Info::display(), winId());
#else
        thread->window.attach(x11Info().display(), winId());
#endif
        thread->window.resize(width(), height());
        _attached = 1;

        _videoEnabled = !_videoDevice.isEmpty();
        if(_videoEnabled)
            thread->pushEvent(new QZBarThread::VideoDeviceEvent(_videoDevice));
    }
    catch(Exception&) { /* ignore (FIXME do something w/error) */ }
}

void QZBar::showEvent (QShowEvent *event)
{
    if(thread && !_attached)
        attach();
}

void QZBar::sizeChange ()
{
    update();
    updateGeometry();
}
