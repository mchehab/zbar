/*------------------------------------------------------------------------
 *  Copyright 2008-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
 *  Copyright 2022 (c) Pier Angelo Vendrame <vogliadifarniente@gmail.com>
 *
 *  This file is part of the ZBar Bar Code Reader.
 *
 *  The ZBar Bar Code Reader is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The ZBar Bar Code Reader is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with the ZBar Bar Code Reader; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301  USA
 *
 *  http://sourceforge.net/projects/zbar
 *------------------------------------------------------------------------*/

#include "Python.h"
#include "pygobject.h"
#include "zbar/zbargtk.h"

static PyObject *create_widget(PyObject *self, PyObject *args)
{
    GtkWidget *widget = zbar_gtk_new();
    if (!widget) {
        return PyErr_NoMemory();
    }
    return pygobject_new(G_OBJECT(widget));
}

static PyMethodDef ZbargtkMethods[] = {
    { "create_widget", create_widget, METH_NOARGS, "Create a new barcode reader widget instance without any associated video device or image." },
    { NULL, NULL, 0, NULL }
};

static struct PyModuleDef zbargtkmozule = {
    PyModuleDef_HEAD_INIT, "zbargtk", "", -1, ZbargtkMethods
};

PyMODINIT_FUNC PyInit_zbargtk(void)
{
    PyObject *module = PyModule_Create(&zbargtkmozule);
    if (!module)
        return NULL;
    pygobject_init(-1, -1, -1);
    return module;
}
