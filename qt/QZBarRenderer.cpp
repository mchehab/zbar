//------------------------------------------------------------------------
//  Copyright 2026 (c) Mauro Carvalho Chehab <mchehab@kernel.org>
//
//  Parts of the code came from QZBar.cpp with:
//     Copyright 2008-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
//
//  This file is part of the ZBar Bar Code Reader.
//------------------------------------------------------------------------

#include <QPainter>
#include <QMutex>
#include <QVector>
#include <QX11Info>
#include <iostream>
#include "QZBarRenderer.h"

using namespace zbar;

/*
 * X11 video renderer
 */
class X11Renderer : public QZBarRenderer
{
public:
    void configure(QWidget *widget)
    {
	widget->setAttribute(Qt::WA_OpaquePaintEvent);
	widget->setAttribute(Qt::WA_PaintOnScreen);
#if QT_VERSION >= 0x040400
	widget->setAttribute(Qt::WA_NativeWindow);
	widget->setAttribute(Qt::WA_DontCreateNativeAncestors);
#endif
    }

    bool attach(QWidget *widget)
    {
	void *display = x11Display(widget);

	if (!display)
	    return false;

	window.attach(display, widget->winId());
	return true;
    }

    QPaintEngine *paintEngine(const QWidget *) const { return NULL; }

    void paint(QWidget *)
    {
	try {
	    window.redraw();
	} catch (Exception &) {
	    /* ignore (FIXME do something w/error) */
	}
    }

    void resize(QWidget *, int width, int height)
    {
	try {
	    window.resize(width, height);
	} catch (Exception &) {
	    /* ignore (FIXME do something w/error) */
	}
    }

    void negotiate(Video &video) { negotiate_format(video, window); }
    void draw(Image &image, Image &) { window.draw(image); }
    void clear() { window.clear(); }

private:
    static void *x11Display(QWidget *widget)
    {
#if QT_VERSION >= 0x050000
	if (!QX11Info::isPlatformX11())
	    return NULL;

	return QX11Info::display();
#else
	return widget->x11Info().display();
#endif
    }

    Window window;
};

/*
 * Qpaint video renderer - Works with Wayland
 */

class QtRenderer : public QZBarRenderer
{
public:
    QtRenderer(int verbosity)
	: debugFrames(verbosity > 0), frameLogged(false), paintLogged(false)
    {
    }

    void configure(QWidget *widget)
    {
	widget->setAttribute(Qt::WA_OpaquePaintEvent);
	widget->setAttribute(Qt::WA_PaintOnScreen, false);
#if QT_VERSION >= 0x040400
	widget->setAttribute(Qt::WA_NativeWindow);
	widget->setAttribute(Qt::WA_DontCreateNativeAncestors);
#endif
    }

    bool attach(QWidget *) {
	return true;
    }

    QPaintEngine *paintEngine(const QWidget *widget) const {
	return widget->QWidget::paintEngine();
    }

    void paint(QWidget *widget)
    {
	QPainter painter(widget);
	painter.fillRect(widget->rect(), Qt::black);
	QImage current;

	{
	    QMutexLocker locker(&mutex);

	    current = frame;
	    if (debugFrames && !paintLogged) {
		std::cerr << "QZBar: Qt paint received "
			    << (current.isNull() ? "no preview frame" :
				"a preview frame") << std::endl;
		paintLogged = true;
	    }
	}

	if (!current.isNull()) {
	    QSize scaled = current.size();
	    scaled.scale(widget->size(), Qt::KeepAspectRatio);
	    QRect target(QPoint((widget->width() - scaled.width()) / 2,
			(widget->height() - scaled.height()) / 2), scaled);
	    painter.drawImage(target, current);
	}
    }

    void resize(QWidget *, int, int) {}

    void negotiate(Video &video)
    {
	if (zbar_negotiate_format(video, NULL))
	    throw_exception(video);
    }

    void draw(Image &, Image &preview)
    {
	unsigned width, height;
	preview.get_size(width, height);
	QImage current((const uchar *)preview.get_data(), width, height, width,
		       QImage::Format_Indexed8);
	QVector<QRgb> grayscale;

	grayscale.reserve(256);
	for (int i = 0; i < 256; i++)
	    grayscale.append(qRgb(i, i, i));
	current.setColorTable(grayscale);
	{
	    QMutexLocker locker(&mutex);

	    frame = current.copy();
	    if (debugFrames && !frameLogged) {
		std::cerr << "QZBar: published grayscale preview " << width
			  << " x " << height << std::endl;
		frameLogged = true;
	    }
	}
    }

    void clear()
    {
	QMutexLocker locker(&mutex);
	frame = QImage();
    }

private:
    QMutex mutex;
    QImage frame;
    bool debugFrames;
    bool frameLogged;
    bool paintLogged;
};

QZBarRenderer *QZBarRenderer::create(int verbosity)
{
#if QT_VERSION >= 0x050000
    if (!QX11Info::isPlatformX11())
	return new QtRenderer(verbosity);
#endif
    return new X11Renderer;
}
