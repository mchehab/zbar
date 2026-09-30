//------------------------------------------------------------------------
//  Copyright 2026 (c) Mauro Carvalho Chehab <mchehab@kernel.org>
//
//  Parts of the code came from QZBar.cpp with:
//     Copyright 2008-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
//
//  This file is part of the ZBar Bar Code Reader.
//------------------------------------------------------------------------

#ifndef _QZBARRENDERER_H_
#define _QZBARRENDERER_H_

#include <QPaintEngine>
#include <QWidget>
#include <zbar.h>

using namespace zbar;

class QZBarRenderer
{
public:
    virtual ~QZBarRenderer() {}

    static QZBarRenderer *create(int verbosity);

    virtual void configure(QWidget *widget) = 0;
    virtual bool attach(QWidget *widget) = 0;
    virtual QPaintEngine *paintEngine(const QWidget *widget) const = 0;
    virtual void paint(QWidget *widget) = 0;
    virtual void resize(QWidget *widget, int width, int height) = 0;
    virtual void negotiate(Video &video) = 0;
    virtual void draw(Image &image, Image &preview) = 0;
    virtual void clear() = 0;
};

#endif
