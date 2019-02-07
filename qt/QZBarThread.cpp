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

#include <iostream>
#include "QZBarThread.h"

using namespace zbar;

static const QString textFormat("%1:%2");

QZBarThread::QZBarThread (int verbosity)
    : _videoOpened(false),
      reqWidth(DEFAULT_WIDTH),
      reqHeight(DEFAULT_HEIGHT),
      video(NULL),
      image(NULL),
      running(true),
      videoRunning(false),
      videoEnabled(false)
{
    zbar_set_verbosity(verbosity);
    scanner.set_handler(*this);
}

void QZBarThread::image_callback (Image &image)
{
    for(Image::SymbolIterator sym = image.symbol_begin();
        sym != image.symbol_end();
        ++sym)
        if(!sym->get_count()) {
            QString data = QString::fromStdString(sym->get_data());
            emit decoded(sym->get_type(), data);

            emit decodedText(textFormat.arg(
                QString::fromStdString(sym->get_type_name()),
                data));
        }
}

void QZBarThread::processImage (Image &image)
{
    {
        scanner.recycle_image(image);
        Image tmp = image.convert(*(long*)"Y800");
        scanner.scan(tmp);
        image.set_symbols(tmp.get_symbols());
    }
    window.draw(image);
    if(this->image && this->image != &image) {
        delete this->image;
        this->image = NULL;
    }
    emit update();
}

void QZBarThread::enableVideo (bool enable)
{
    if(!video) {
        videoRunning = videoEnabled = false;
        return;
    }
    try {
        scanner.enable_cache(enable);
        video->enable(enable);
        videoRunning = enable;
    }
    catch(std::exception &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
    }
    if(!enable) {
        // release video image and revert to logo
        clear();
        emit update();
    }
}

void QZBarThread::openVideo (const QString &device)
{
    if(videoRunning)
        enableVideo(false);

    {
        QMutexLocker locker(&mutex);
        videoEnabled = _videoOpened = false;
        reqWidth = DEFAULT_WIDTH;
        reqHeight = DEFAULT_HEIGHT;
    }

    // ensure old video doesn't have image ref
    // (FIXME handle video destroyed w/images outstanding)
    clear();
    emit update();

    if(video) {
        delete video;
        video = NULL;
        emit videoOpened(false);
    }

    if(device.isEmpty())
        return;

    try {
        std::string devstr = device.toStdString();
        video = new Video(devstr);
        negotiate_format(*video, window);
        {
            QMutexLocker locker(&mutex);
            videoEnabled = _videoOpened = true;
            reqWidth = video->get_width();
            reqHeight = video->get_height();
        }
        emit videoOpened(true);
    }
    catch(std::exception &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        emit videoOpened(false);
    }
}


void QZBarThread::videoDeviceEvent (VideoDeviceEvent *e)
{
    openVideo(e->device);
}

void QZBarThread::videoEnabledEvent (VideoEnabledEvent *e)
{
    if(videoRunning && !e->enabled)
        enableVideo(false);
    videoEnabled = e->enabled;
}

void QZBarThread::scanImageEvent (ScanImageEvent *e)
{
    if(videoRunning)
        enableVideo(false);

    try {
        image = new QZBarImage(e->image);
        processImage(*image);
    }
    catch(std::exception &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        clear();
    }
}

bool QZBarThread::event (QEvent *e)
{
    switch((EventType)e->type()) {
    case VideoDevice:
        videoDeviceEvent((VideoDeviceEvent*)e);
        break;
    case VideoEnabled:
        videoEnabledEvent((VideoEnabledEvent*)e);
        break;
    case ScanImage:
        scanImageEvent((ScanImageEvent*)e);
        break;
    case Exit:
        if(videoRunning)
            enableVideo(false);
        running = false;
        break;
    default:
        return(false);
    }
    return(true);
}


void QZBarThread::run ()
{
    QEvent *e = NULL;
    while(running) {
        if(!videoEnabled) {
            QMutexLocker locker(&mutex);
            while(queue.isEmpty())
                newEvent.wait(&mutex);
            e = queue.takeFirst();
        }
        else {
            // release reference to any previous QImage
            clear();
            enableVideo(true);

            while(videoRunning && !e) {
                try {
                    Image image = video->next_image();
                    processImage(image);
                }
                catch(std::exception &e) {
                    std::cerr << "ERROR: " << e.what() << std::endl;
                    enableVideo(false);
                    openVideo("");
                }
                QMutexLocker locker(&mutex);
                if(!queue.isEmpty())
                    e = queue.takeFirst();
            }

            if(videoRunning)
                enableVideo(false);
        }
        if(e) {
            event(e);
            delete e;
            e = NULL;
        }
    }
    clear();
    openVideo("");
}

QVector< QPair< int , QString > > QZBarThread::get_menu(int index)
{
    QVector< QPair< int , QString > > vector;
    struct video_controls_s *ctrl;

    if(!video)
        return vector;

    ctrl = video->get_controls(index);
    if (!ctrl)
        return vector;

    for (unsigned int i = 0; i < ctrl->menu_size; i++)
        vector.append(qMakePair((int)ctrl->menu[i].value,
                                QString::fromUtf8(ctrl->menu[i].name)));

    return vector;
}

int QZBarThread::get_controls(int index, char **name, char **group,
                              enum QZBar::ControlType *type,
                              int *min, int *max, int *def, int *step)
{
    struct video_controls_s *ctrl;

    if(!video)
        return 0;

    ctrl = video->get_controls(index);
    if (!ctrl)
        return 0;

    if (name)
        *name = ctrl->name;
    if (group)
        *group = ctrl->group;
    if (min)
        *min = ctrl->min;
    if (max)
        *max = ctrl->max;
    if (def)
        *def = ctrl->def;
    if (step)
        *step = ctrl->step;

    if (type) {
        switch (ctrl->type) {
        case VIDEO_CNTL_INTEGER:
            *type = QZBar::Integer;
            break;
        case VIDEO_CNTL_MENU:
            *type = QZBar::Menu;
            break;
        case VIDEO_CNTL_BUTTON:
            *type = QZBar::Button;
            break;
        case VIDEO_CNTL_INTEGER64:
            *type = QZBar::Integer64;
            break;
        case VIDEO_CNTL_STRING:
            *type = QZBar::String;
            break;
        case VIDEO_CNTL_BOOLEAN:
            *type = QZBar::Boolean;
            break;
        default:
            *type = QZBar::Unknown;
            break;
        }
    }

    return 1;
}

int QZBarThread::set_control(char *name, bool value)
{
    if(!video)
        return 0;

    return video->set_control(name, value);
}

int QZBarThread::set_control(char *name, int value)
{
    if(!video)
        return 0;

    return video->set_control(name, value);
}

int QZBarThread::get_control(char *name, bool *value)
{
    if(!video)
        return 0;

    return video->get_control(name, value);
}

int QZBarThread::get_control(char *name, int *value)
{
    if(!video)
        return 0;

    return video->get_control(name, value);
}
