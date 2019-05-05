/*------------------------------------------------------------------------
 *  Copyright 2009-2010 (c) Jeff Brown <spadix@users.sourceforge.net>
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

#include "zbarmodule.h"

typedef struct enumdef {
    const char *strval;
    int intval;
} enumdef;

static char *exc_names[] = {
    "zbar.Exception",
    NULL,
    "zbar.InternalError",
    "zbar.UnsupportedError",
    "zbar.InvalidRequestError",
    "zbar.SystemError",
    "zbar.LockingError",
    "zbar.BusyError",
    "zbar.X11DisplayError",
    "zbar.X11ProtocolError",
    "zbar.WindowClosed",
    "zbar.WinAPIError",
};

static const enumdef symbol_defs[] = {
    { "NONE",           ZBAR_NONE },
    { "PARTIAL",        ZBAR_PARTIAL },
    { "EAN8",           ZBAR_EAN8 },
    { "UPCE",           ZBAR_UPCE },
    { "ISBN10",         ZBAR_ISBN10 },
    { "UPCA",           ZBAR_UPCA },
    { "EAN13",          ZBAR_EAN13 },
    { "ISBN13",         ZBAR_ISBN13 },
    { "DATABAR",        ZBAR_DATABAR },
    { "DATABAR_EXP",    ZBAR_DATABAR_EXP },
    { "I25",            ZBAR_I25 },
    { "CODABAR",        ZBAR_CODABAR },
    { "CODE39",         ZBAR_CODE39 },
    { "PDF417",         ZBAR_PDF417 },
    { "QRCODE",         ZBAR_QRCODE },
    { "SQCODE",         ZBAR_SQCODE },
    { "CODE93",         ZBAR_CODE93 },
    { "CODE128",        ZBAR_CODE128 },
    { NULL, }
};

static const enumdef config_defs[] = {
    { "ENABLE",         ZBAR_CFG_ENABLE },
    { "ADD_CHECK",      ZBAR_CFG_ADD_CHECK },
    { "EMIT_CHECK",     ZBAR_CFG_EMIT_CHECK },
    { "ASCII",          ZBAR_CFG_ASCII },
    { "MIN_LEN",        ZBAR_CFG_MIN_LEN },
    { "MAX_LEN",        ZBAR_CFG_MAX_LEN },
    { "UNCERTAINTY",    ZBAR_CFG_UNCERTAINTY },
    { "POSITION",       ZBAR_CFG_POSITION },
    { "X_DENSITY",      ZBAR_CFG_X_DENSITY },
    { "Y_DENSITY",      ZBAR_CFG_Y_DENSITY },
    { NULL, }
};

static const enumdef modifier_defs[] = {
    { "GS1",            ZBAR_MOD_GS1 },
    { "AIM",            ZBAR_MOD_AIM },
    { NULL, }
};

static const enumdef orient_defs[] = {
    { "UNKNOWN",        ZBAR_ORIENT_UNKNOWN },
    { "UP",             ZBAR_ORIENT_UP },
    { "RIGHT",          ZBAR_ORIENT_RIGHT },
    { "DOWN",           ZBAR_ORIENT_DOWN },
    { "LEFT",           ZBAR_ORIENT_LEFT },
    { NULL, }
};


#if PY_MAJOR_VERSION < 3
struct module_state zbar_state;
#endif


int
object_to_bool (PyObject *obj,
                int *val)
{
    int tmp = PyObject_IsTrue(obj);
    if(tmp < 0)
        return(0);
    *val = tmp;
    return(1);
}

int
parse_dimensions (PyObject *seq,
                  int *dims,
                  int n)
{
    if(!PySequence_Check(seq) ||
       PySequence_Size(seq) != n)
        return(-1);

    int i;
    for(i = 0; i < n; i++, dims++) {
        PyObject *dim = PySequence_GetItem(seq, i);
        if(!dim)
            return(-1);
#if PY_MAJOR_VERSION >= 3
        *dims = PyLong_AsSsize_t(dim);
#else
        *dims = PyInt_AsSsize_t(dim);
#endif
        Py_DECREF(dim);
        if(*dims == -1 && PyErr_Occurred())
            return(-1);
    }
    return(0);
}


static PyObject*
version (PyObject *self,
         PyObject *args)
{
    if(!PyArg_ParseTuple(args, ""))
        return(NULL);

    unsigned int major, minor;
    zbar_version(&major, &minor, NULL);

    return(Py_BuildValue("II", major, minor));
}

static PyObject*
set_verbosity (PyObject *self,
               PyObject *args)
{
    int verbosity;
    if(!PyArg_ParseTuple(args, "i", &verbosity))
        return(NULL);

    zbar_set_verbosity(verbosity);

    Py_INCREF(Py_None);
    return(Py_None);
}

static PyObject*
increase_verbosity (PyObject *self,
                    PyObject *args)
{
    if(!PyArg_ParseTuple(args, ""))
        return(NULL);

    zbar_increase_verbosity();

    Py_INCREF(Py_None);
    return(Py_None);
}

static PyMethodDef zbar_functions[] = {
    { "version",            version,            METH_VARARGS, NULL },
    { "set_verbosity",      set_verbosity,      METH_VARARGS, NULL },
    { "increase_verbosity", increase_verbosity, METH_VARARGS, NULL },
    { NULL, },
};


#if PY_MAJOR_VERSION >= 3

static int zbar_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(GETSTATE(m)->zbar_exc[0]);
    return 0;
}

static int zbar_clear(PyObject *m) {
    Py_CLEAR(GETSTATE(m)->zbar_exc[0]);
    return 0;
}


struct PyModuleDef zbar_moduledef = {
    PyModuleDef_HEAD_INIT,
    "zbar",
    NULL,
    sizeof(struct module_state),
    zbar_functions,
    NULL,
    zbar_traverse,
    zbar_clear,
    NULL
};

#define INITERROR return NULL

PyMODINIT_FUNC
PyInit_zbar(void)

#else
#define INITERROR return

PyMODINIT_FUNC
initzbar(void)
#endif
{
#if PY_MAJOR_VERSION >= 3
    /* initialize types */
    zbarEnumItem_Type.tp_base = &PyLong_Type;
#else
    zbarEnumItem_Type.tp_base = &PyInt_Type;
    zbarException_Type.tp_base = (PyTypeObject*)PyExc_Exception;

    if(PyType_Ready(&zbarException_Type) < 0)
        INITERROR;
#endif

    if(PyType_Ready(&zbarEnumItem_Type) < 0 ||
       PyType_Ready(&zbarEnum_Type) < 0 ||
       PyType_Ready(&zbarImage_Type) < 0 ||
       PyType_Ready(&zbarSymbol_Type) < 0 ||
       PyType_Ready(&zbarSymbolSet_Type) < 0 ||
       PyType_Ready(&zbarSymbolIter_Type) < 0 ||
       PyType_Ready(&zbarProcessor_Type) < 0 ||
       PyType_Ready(&zbarImageScanner_Type) < 0 ||
       PyType_Ready(&zbarDecoder_Type) < 0 ||
       PyType_Ready(&zbarScanner_Type) < 0)
        INITERROR;

    /* initialize module */
#if PY_MAJOR_VERSION >= 3
    PyObject *mod = PyModule_Create(&zbar_moduledef);
#else
    PyObject *mod = Py_InitModule("zbar", zbar_functions);
#endif
    if(!mod)
        INITERROR;

#if PY_MAJOR_VERSION >= 3
    if(PyState_AddModule(mod, &zbar_moduledef)) {
        Py_DECREF(mod);
        INITERROR;
    }
#endif

    struct module_state *st = GETSTATE(mod);

    /* initialize constant containers */
    st->config_enum = zbarEnum_New();
    st->modifier_enum = zbarEnum_New();
    st->symbol_enum = PyDict_New();
    st->orient_enum = zbarEnum_New();
    if(!st->config_enum || !st->modifier_enum || !st->symbol_enum || !st->orient_enum) {
        Py_DECREF(mod);
        INITERROR;
    }

    /* internally created/read-only type overrides */
    zbarEnum_Type.tp_new = NULL;
    zbarEnum_Type.tp_setattr = NULL;
    zbarEnum_Type.tp_setattro = NULL;

    /* zbar internal exception objects */
#if PY_MAJOR_VERSION >= 3
    st->zbar_exc[0] = PyErr_NewException("zbar.Exception", NULL, NULL);
#else
    st->zbar_exc[0] = (PyObject*)&zbarException_Type;
#endif
    if (st->zbar_exc[0] == NULL) {
        Py_DECREF(mod);
        INITERROR;
    }

    st->zbar_exc[ZBAR_ERR_NOMEM] = NULL;
    zbar_error_t ei;
    for(ei = ZBAR_ERR_INTERNAL; ei < ZBAR_ERR_NUM; ei++) {
        st->zbar_exc[ei] = PyErr_NewException(exc_names[ei], st->zbar_exc[0], NULL);
        if (!st->zbar_exc[ei]) {
            Py_DECREF(mod);
            INITERROR;
        }
    }

    /* add types to module */
    PyModule_AddObject(mod, "EnumItem", (PyObject*)&zbarEnumItem_Type);
    PyModule_AddObject(mod, "Image", (PyObject*)&zbarImage_Type);
    PyModule_AddObject(mod, "Config", (PyObject*)st->config_enum);
    PyModule_AddObject(mod, "Modifier", (PyObject*)st->modifier_enum);
    PyModule_AddObject(mod, "Orient", (PyObject*)st->orient_enum);
    PyModule_AddObject(mod, "Symbol", (PyObject*)&zbarSymbol_Type);
    PyModule_AddObject(mod, "SymbolSet", (PyObject*)&zbarSymbolSet_Type);
    PyModule_AddObject(mod, "SymbolIter", (PyObject*)&zbarSymbolIter_Type);
    PyModule_AddObject(mod, "Processor", (PyObject*)&zbarProcessor_Type);
    PyModule_AddObject(mod, "ImageScanner", (PyObject*)&zbarImageScanner_Type);
    PyModule_AddObject(mod, "Decoder", (PyObject*)&zbarDecoder_Type);
    PyModule_AddObject(mod, "Scanner", (PyObject*)&zbarScanner_Type);

    for(ei = 0; ei < ZBAR_ERR_NUM; ei++)
        if(st->zbar_exc[ei])
            PyModule_AddObject(mod, exc_names[ei] + 5, st->zbar_exc[ei]);

    /* add constants */
    PyObject *dict = PyModule_GetDict(mod);
    st->color_enum[ZBAR_SPACE] =
        zbarEnumItem_New(dict, NULL, ZBAR_SPACE, "SPACE");
    st->color_enum[ZBAR_BAR] =
        zbarEnumItem_New(dict, NULL, ZBAR_BAR, "BAR");

    const enumdef *item;
    for(item = config_defs; item->strval; item++)
        zbarEnum_Add(st->config_enum, item->intval, item->strval);
    for(item = modifier_defs; item->strval; item++)
        zbarEnum_Add(st->modifier_enum, item->intval, item->strval);
    for(item = orient_defs; item->strval; item++)
        zbarEnum_Add(st->orient_enum, item->intval, item->strval);

    PyObject *tp_dict = zbarSymbol_Type.tp_dict;
    for(item = symbol_defs; item->strval; item++)
        zbarEnumItem_New(tp_dict, st->symbol_enum, item->intval, item->strval);
    st->symbol_NONE = zbarSymbol_LookupEnum(ZBAR_NONE);

#if PY_MAJOR_VERSION >= 3
    return mod;
#endif
}


PyObject*
zbarErr_Set (PyObject *self)
{
    const void *zobj = ((zbarProcessor*)self)->zproc;
    zbar_error_t err = _zbar_get_error_code(zobj);

    struct module_state *st = GETMODSTATE();

    if(err == ZBAR_ERR_NOMEM)
        PyErr_NoMemory();
    else if(err < ZBAR_ERR_NUM) {
        PyObject *type = st->zbar_exc[err];
        assert(type);
        PyErr_SetObject(type, self);
    }
    else
        PyErr_SetObject(st->zbar_exc[0], self);
    return(NULL);
}
