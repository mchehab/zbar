/*------------------------------------------------------------------------
 *  Copyright 2012 (c) Klaus Triendl <klaus@triendl.eu>
 *  Copyright 2012 (c) Jarek Czekalski <jarekczek@poczta.onet.pl>
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
 *  http://sourceforge.net/projects/zbarw
 *------------------------------------------------------------------------*/

#include "video.h"
#include "thread.h"
#include "misc.h"
#include <objbase.h>
#include <strmif.h>
#include <control.h>
#include <qedit.h>
#include <amvideo.h>    // include after ddraw.h has been included from any dshow header
#include <assert.h>
#include <initguid.h>

#define ZBAR_DEFINE_STATIC_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        static const GUID name; \
        static const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } };

// define a special guid that can be used for fourcc formats
// 00000000-0000-0010-8000-00AA00389B71   == MEDIASUBTYPE_FOURCC_PLACEHOLDER
ZBAR_DEFINE_STATIC_GUID(MEDIASUBTYPE_FOURCC_PLACEHOLDER,
0x00000000, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

#define OUR_GUID_ENTRY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    ZBAR_DEFINE_STATIC_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8);

#include <uuids.h>

DEFINE_GUID(IID_IUnknown, 0x00000000, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(IID_ISampleGrabber, 0x6b652fff, 0x11fe, 0x4fce, 0x92,0xad, 0x02,0x66,0xb5,0xd7,0xc7,0x8f);
DEFINE_GUID(IID_ISampleGrabberCB,0x0579154a,0x2b53,0x4994,0xb0,0xd0,0xe7,0x73,0x14,0x8e,0xff,0x85);
DEFINE_GUID(IID_IBaseFilter,0x56a86895,0x0ad4,0x11ce,0xb0,0x3a,0x00,0x20,0xaf,0x0b,0xa7,0x70);
DEFINE_GUID(IID_ICreateDevEnum, 0x29840822, 0x5b84, 0x11d0, 0xbd,0x3b, 0x00,0xa0,0xc9,0x11,0xce,0x86);
DEFINE_GUID(IID_IGraphBuilder, 0x56a868a9, 0x0ad4, 0x11ce, 0xb0,0x3a, 0x00,0x20,0xaf,0x0b,0xa7,0x70);
DEFINE_GUID(IID_IMediaControl, 0x56a868b1, 0x0ad4, 0x11ce, 0xb0,0x3a, 0x00,0x20,0xaf,0x0b,0xa7,0x70);
DEFINE_GUID(IID_IPropertyBag, 0x55272a00, 0x42cb, 0x11ce, 0x81,0x35, 0x00,0xaa,0x00,0x4b,0xb8,0x51);
DEFINE_GUID(IID_IAMStreamConfig, 0xc6e13340, 0x30ac, 0x11d0, 0xa1,0x8c, 0x00,0xa0,0xc9,0x11,0x89,0x56);
DEFINE_GUID(IID_ICaptureGraphBuilder2, 0x93e5a4e0, 0x2d50, 0x11d2, 0xab,0xfa, 0x00,0xa0,0xc9,0xc6,0xe3,0x8d);
DEFINE_GUID(CLSID_NullRenderer,0xc1f400a4,0x3f08,0x11d3,0x9f,0x0b,0x00,0x60,0x08,0x03,0x9e,0x37);
DEFINE_GUID(CLSID_SampleGrabber,0xc1f400a0,0x3f08,0x11d3,0x9f,0x0b,0x00,0x60,0x08,0x03,0x9e,0x37);

#define BIH_FMT "%ldx%ld @%dbpp (%lx) cmp=%.4s(%08lx) res=%ldx%ld clr=%ld/%ld (%lx)"
#define BIH_FIELDS(bih)                                                 \
    (bih)->biWidth, (bih)->biHeight, (bih)->biBitCount, (bih)->biSizeImage, \
        (char*)&(bih)->biCompression, (bih)->biCompression,             \
        (bih)->biXPelsPerMeter, (bih)->biYPelsPerMeter,                 \
        (bih)->biClrImportant, (bih)->biClrUsed, (bih)->biSize

// taken from Capturing an Image winapi sample
#define BMP_SIZE(cx, cy, bitsPerPix) \
        ((((cx) * (bitsPerPix) + 31) / 32) * 4 * (cy))

#define COM_SAFE_RELEASE(ppIface) \
    if (*ppIface)\
        (*ppIface)->lpVtbl->Release(*ppIface)

#define CHECK_COM_ERROR(hr, msg, stmt)\
    if (FAILED(hr))\
    {\
        LPSTR sysmsg = NULL; \
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER \
                      | FORMAT_MESSAGE_FROM_SYSTEM \
                      | FORMAT_MESSAGE_IGNORE_INSERTS, \
                      NULL, hr, \
                      0 /* MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US) */, \
                      (LPSTR)&sysmsg, 0, NULL); \
        zprintf(6, "%s, hresult: 0x%lx %s", msg, hr, sysmsg); \
        LocalFree(sysmsg); \
        stmt; \
    }

static const struct uuid_desc_s {
    const GUID *guid;
    const char *name;
} known_uuids[] = {
    #define OUR_GUID_ENTRY(m_name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)\
    { &m_name, #m_name },
    #include <ksuuids.h>
    { NULL, NULL }
};

static const REFERENCE_TIME _100ns_unit = 1*1000*1000*1000 / 100;

// format to which we convert MJPG streams
static const REFGUID mjpg_conversion_mediatype = &MEDIASUBTYPE_RGB32;
static const int mjpg_conversion_fmt = fourcc('B','G','R','4');
static const int mjpg_conversion_fmt_bpp = 32;

static long grabbed_count = 0;

// Destroy (Release) the format block for a media type.
static void DestroyMediaType(AM_MEDIA_TYPE* mt)
{
    if (mt->cbFormat != 0)
        CoTaskMemFree(mt->pbFormat);

    if (mt->pUnk)
        IUnknown_Release(mt->pUnk);
}

// Destroy the format block for a media type and free the media type
static void DeleteMediaType(AM_MEDIA_TYPE* mt)
{
    if (!mt)
        return;
    DestroyMediaType(mt);
    CoTaskMemFree(mt);
}

static void make_fourcc_subtype(GUID* subtype, uint32_t fmt)
{
    *subtype = MEDIASUBTYPE_FOURCC_PLACEHOLDER;
    subtype->Data1 = fmt;
}

static int dshow_is_fourcc_guid(REFGUID subtype)
{
    // make up a fourcc guid in spe
    GUID clsid;
    make_fourcc_subtype(&clsid, subtype->Data1);

    return IsEqualGUID(subtype, &clsid);
}

/// Checks whether the given AM_MEDIA_TYPE contains a
/// VIDEOINFOHEADER block.
static inline int dshow_has_vih(AM_MEDIA_TYPE* mt)
{
    // documentation for AM_MEDIA_TYPE emphasizes to do a thorough check
    int isvih = (IsEqualGUID(&mt->formattype, &FORMAT_VideoInfo) &&
                 mt->cbFormat >= sizeof(VIDEOINFOHEADER) &&
                 mt->pbFormat);
    return isvih;
}

/// Access the BITMAPINFOHEADER in the given AM_MEDIA_TYPE,
/// access is non-const
static inline BITMAPINFOHEADER* dshow_access_bih(AM_MEDIA_TYPE* mt)
{
    VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*) mt->pbFormat;
    return &vih->bmiHeader;
}

/// Access the BITMAPINFOHEADER in the given AM_MEDIA_TYPE,
/// access is const
static inline const BITMAPINFOHEADER* dshow_caccess_bih(const AM_MEDIA_TYPE* mt)
{
    VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*) mt->pbFormat;
    return &vih->bmiHeader;
}

/// Flips the image vertically copying it from srcBuf to img.
/** @param bpp Bits Per Pixel */
static void flip_vert(zbar_image_t* const img, void* const srcBuf, int bpp)
{
    // The formula below works only if bpp%8==0
    long bytesPerLine = 1L * img->width * bpp / 8;
    assert(img->datalen >= img->height * bytesPerLine);
    void *dst = (void*)img->data;
    void *src = srcBuf + (img->height - 1) * bytesPerLine;
    int i, n;
    for (i=0, n = img->height; i < n; i++) {
        memcpy(dst, src, bytesPerLine);
        dst += bytesPerLine;
        src -= bytesPerLine;
    }
    assert(src + bytesPerLine == srcBuf);
}

/// Internal format information
struct int_format_s
{
    uint32_t fourcc;
    /// index for IAMStreamConfig::GetStreamCaps
    /// @note compression in bih structure may differ from one used by zbar
    int idx_caps;
    resolution_list_t resolutions;
};
typedef struct int_format_s int_format_t;

struct video_state_s
{
    zbar_thread_t thread;             /* capture message pump */
    HANDLE captured;
    HANDLE notify;                    /* capture thread status change */
    int bi_size;                      /* size of bih */
    BITMAPINFOHEADER* bih;            /* video format details of grabbed samples;
                                         format might be among fourcc or BI_RGB */
    int do_flip_bitmap;               /* whether uncompressed bitmap images are
                                         bottom-up and need to be vertically
                                         flipped */
    zbar_image_t* image;              /* current capturing frame */

    IGraphBuilder* graph;             /* dshow graph manager */
    IMediaControl* mediacontrol;      /* dshow graph control */
    IBaseFilter* camera;              /* dshow source filter */
    ISampleGrabber* samplegrabber;    /* dshow intermediate filter */
    IBaseFilter* grabberbase;         /* samplegrabber's IBaseFilter interface */
    IBaseFilter* nullrenderer;
    ICaptureGraphBuilder2* builder;
    IAMStreamConfig* camstreamconfig; /* dshow stream configuration interface */
    int caps_size;                    /* length of stream config caps */
    /// 0 terminated list of supported internal formats.
    /** The size of this
      * array matches the size of {@link zbar_video_s#formats} array and
      * the consecutive entries correspond to each other, making a mapping
      * between internal (camera) formats and zbar formats
      * (presented to zbar processor). */
    int_format_t *int_formats;
    resolution_t def_resolution;    /* initial resolution read
                                       from the camera */
};

static void dshow_destroy_video_state_t(video_state_t* state)
{
    COM_SAFE_RELEASE(&state->camstreamconfig);
    COM_SAFE_RELEASE(&state->builder);
    COM_SAFE_RELEASE(&state->nullrenderer);
    COM_SAFE_RELEASE(&state->grabberbase);
    COM_SAFE_RELEASE(&state->samplegrabber);
    COM_SAFE_RELEASE(&state->camera);
    COM_SAFE_RELEASE(&state->mediacontrol);
    COM_SAFE_RELEASE(&state->graph);

    if (state->captured)
        CloseHandle(state->captured);

    free(state->bih);

    if (state->int_formats)
    {
        int_format_t *fmt;
        for (fmt = state->int_formats; !is_struct_null(fmt); fmt++) {
            resolution_list_cleanup(&fmt->resolutions);
        }
    }
    free(state->int_formats);
}

/// Returns the index of the given format in formats array.
/** If not found, returns -1. The array is constructed like
  * <code>vdo->formats</code>, with the terminating null. */
static int get_format_index(uint32_t* fmts, uint32_t fmt0)
{
    uint32_t *fmt;
    int i = 0;
    for (fmt = fmts; *fmt; fmt++) {
        if (*fmt == fmt0)
            break;
        i++;
    }
    if (*fmt)
        return i;
    else
        return -1;
}

/// Returns the index of the given format in internal formats array.
/** If not found, returns -1. The array is constructed like
  * <code>vdo->state->int_formats</code>, with the terminating zeroed
  * element. */
/** @param fmt0 A fourcc format code */
static int get_int_format_index(int_format_t *fmts, uint32_t fmt0)
{
    int_format_t *fmt;
    int i = 0;
    for (fmt = fmts; !is_struct_null(fmt); fmt++) {
        if (fmt->fourcc == fmt0)
            break;
        i++;
    }
    if (!is_struct_null(fmt))
        return i;
    else
        return -1;
}

/// Gets the fourcc code for the given media type.
/** This is necessary because of non-standard coding of video
  * formats by Windows, for example BGR3/4 = BI_RGB . */
static uint32_t get_fourcc_for_mt(const AM_MEDIA_TYPE* mt)
{
    if IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_RGB24)
        return fourcc('B','G','R','3');
    else if IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_RGB32)
        return fourcc('B','G','R','4');
    else if (dshow_is_fourcc_guid(&mt->subtype))
        return mt->subtype.Data1;
    else
        return 0;
}

/// Dumps the mapping of internal and external formats, if the debug level
/// is sufficient.
static void dump_formats(zbar_video_t* vdo)
{
    video_state_t* state = vdo->state;
    uint32_t *fmt;
    int_format_t *int_fmt;
    zprintf(8, "Detected formats: (internal) / (translated for zbar)\n");
    fmt = vdo->formats;
    int_fmt = state->int_formats;
    while (*fmt) {
        zprintf(8, "  %.4s / %.4s, resolutions: %lu\n",
            (char*)&int_fmt->fourcc, (char*)fmt, int_fmt->resolutions.cnt);
        fmt++; int_fmt++;
    }
}

/// Allocates a string containing given guid.
/** If it's among known CLSID's, its name is returned.
  * Returned string should be freed with `free`. */
static char* get_clsid_string(REFGUID guid)
{
    char *msg = NULL;

    // first try to determine guid name from the list
    int i;
    for (i = 0; known_uuids[i].name; i++)
    {
        if (IsEqualGUID(known_uuids[i].guid, guid))
        {
            msg = malloc(strlen(known_uuids[i].name) + 1);
            strcpy(msg, known_uuids[i].name);
            break;
        }
    }

    // if that failed, give the ugly string
    if (msg == NULL)
    {
        LPOLESTR str = L"ERROR";
        HRESULT hr = StringFromCLSID(guid, &str);
        int c = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
        msg = malloc(c);
        WideCharToMultiByte(CP_UTF8, 0, str, -1, msg, c, NULL, NULL);
        if (!FAILED(hr))
            CoTaskMemFree(str);
    }

    return msg;
}

/// Maps internal format MJPG (if found) to a format known to
/// zbar.
/** The conversion will be done by dshow mjpeg decompressor
  * before passing the image to zbar. */
static void prepare_mjpg_format_mapping(zbar_video_t* vdo)
{
    video_state_t* state = vdo->state;
    /// The format we will convert MJPG to
    uint32_t fmtConv = mjpg_conversion_fmt;
    int iMjpg = get_int_format_index(state->int_formats,
                                     fourcc('M','J','P','G'));
    if (iMjpg < 0)
        return;
    assert(vdo->formats[iMjpg] == fourcc('M','J','P','G'));

    // If we already have fmtConv, it will lead to duplicating it in
    // external formats.
    // We can't leave it this way, because when zbar wants to use fmtConv
    // we must have only one internal format for that.
    // It's better to drop MJPG then, as it's a compressed format and
    // we prefer better quality images.

    // The index of fmtConv before mapping mjpg to fmtConv
    int iMjpgConv = get_int_format_index(state->int_formats, fmtConv);

    if (iMjpgConv >= 0) {
        // remove the iMjpgConv entry by moving the following entries by 1
        int i;
        for (i=iMjpgConv; vdo->formats[i]; i++) {
            vdo->formats[i] = vdo->formats[i+1];
            state->int_formats[i] = state->int_formats[i+1];
        }
        // The number of formats is reduced by 1 now, but to realloc just
        // to save 2 times 4 bytes? Too much fuss.
    }
    else {
        vdo->formats[iMjpg] = fmtConv;
    }
}


/// sample grabber callback implementation (derived from ISampleGrabberCB)
typedef struct zbar_samplegrabber_cb
{
    // baseclass
    const struct ISampleGrabberCB _;
    // COM refcount
    ULONG refcount;

    zbar_video_t* vdo;

} zbar_samplegrabber_cb;

// ISampleGrabber methods (implementation below)

HRESULT __stdcall zbar_samplegrabber_cb_QueryInterface(ISampleGrabberCB* _This, REFIID riid, void** ppvObject);
ULONG __stdcall zbar_samplegrabber_cb_AddRef(ISampleGrabberCB* _This);
ULONG __stdcall zbar_samplegrabber_cb_Release(ISampleGrabberCB* _This);
HRESULT __stdcall zbar_samplegrabber_cb_SampleCB(ISampleGrabberCB* _This, double sampletime, IMediaSample* sample);
// note: original MS version expects a long for the buffer length, wine version a LONG
//HRESULT __stdcall zbar_samplegrabber_cb_BufferCB(ISampleGrabberCB* _This, double sampletime, BYTE* buffer, long bufferlen);
HRESULT __stdcall zbar_samplegrabber_cb_BufferCB(ISampleGrabberCB* _This, double sampletime, BYTE* buffer, LONG bufferlen);

static struct ISampleGrabberCBVtbl SampleGrabberCBVtbl =
{
    &zbar_samplegrabber_cb_QueryInterface,
    &zbar_samplegrabber_cb_AddRef,
    &zbar_samplegrabber_cb_Release,
    &zbar_samplegrabber_cb_SampleCB,
    &zbar_samplegrabber_cb_BufferCB
};

static zbar_samplegrabber_cb* new_zbar_samplegrabber_cb(zbar_video_t* vdo)
{
    // allocate memory
    zbar_samplegrabber_cb* o = calloc(1, sizeof(zbar_samplegrabber_cb));

    // construct parent
    ISampleGrabberCB* base = (ISampleGrabberCB*)o;
    base->lpVtbl = &SampleGrabberCBVtbl;

    // construct object
    o->refcount = 1;
    o->vdo = vdo;

    return o;
}

static void delete_zbar_samplegrabber_cb(zbar_samplegrabber_cb* o)
{
    zprintf(16, "thr=%04lx\n", _zbar_thread_self());

    // no destruction necessary

    free(o);
}

HRESULT __stdcall zbar_samplegrabber_cb_QueryInterface(ISampleGrabberCB* _This, REFIID riid, void** ppvObject)
{
    if (IsEqualIID(riid, &IID_IUnknown)            ||
        IsEqualIID(riid, &IID_ISampleGrabberCB)    )
    {
        *ppvObject = _This;
        return NOERROR;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
}

ULONG __stdcall zbar_samplegrabber_cb_AddRef(ISampleGrabberCB* _This)
{
    zbar_samplegrabber_cb* This = (zbar_samplegrabber_cb*)_This;

    return ++This->refcount;
}

ULONG __stdcall zbar_samplegrabber_cb_Release(ISampleGrabberCB* _This)
{
    zbar_samplegrabber_cb* This = (zbar_samplegrabber_cb*)_This;

    ULONG refcnt = --This->refcount;
    if (!refcnt)
        delete_zbar_samplegrabber_cb(This);

    return refcnt;
}

HRESULT __stdcall zbar_samplegrabber_cb_SampleCB(ISampleGrabberCB* _This, double sampletime, IMediaSample* sample)
{
    return E_NOTIMPL;
}

//HRESULT __stdcall zbar_samplegrabber_cb_BufferCB(ISampleGrabberCB* _This, double sampletime, BYTE* buffer, long bufferlen)
HRESULT __stdcall zbar_samplegrabber_cb_BufferCB(ISampleGrabberCB* _This, double sampletime, BYTE* buffer, LONG bufferlen)
{
    if (!buffer || !bufferlen)
        return S_OK;


    grabbed_count++;
    zbar_samplegrabber_cb* This = (zbar_samplegrabber_cb*)_This;
    zbar_video_t* vdo = This->vdo;

    _zbar_mutex_lock(&vdo->qlock);

    zprintf(16, "got sample no %ld: %p (%ld), thr=%04lx\n", grabbed_count,
            buffer, bufferlen, _zbar_thread_self());

    zbar_image_t* img = vdo->state->image;
    if (!img)
    {
        _zbar_mutex_lock(&vdo->qlock);
        img = video_dq_image(vdo);
        // note: video_dq_image() has unlocked the mutex
    }
    if (img)
    {
        zprintf(16, "copying into img: %p (srcidx: %d, data: %p, len: %ld)\n", img, img->srcidx, img->data, img->datalen);

        assert(img->datalen == bufferlen);

        // The image needs to be copied now. Usually memcpy is ok,
        // but in case of MJPG the picture is upside-down.
        if (vdo->state->do_flip_bitmap)
            flip_vert(img, buffer, vdo->state->bih->biBitCount);
        else
            memcpy((void*)img->data, buffer, img->datalen);

        vdo->state->image = img;
        SetEvent(vdo->state->captured);
    }

    _zbar_mutex_unlock(&vdo->qlock);

    return S_OK;
}


static ZTHREAD dshow_capture_thread(void* arg)
{
    zbar_video_t* vdo = arg;
    video_state_t* state = vdo->state;
    zbar_thread_t* thr = &state->thread;


    _zbar_mutex_lock(&vdo->qlock);

    _zbar_thread_init(thr);
    zprintf(4, "spawned dshow capture thread (thr=%04lx)\n",
            _zbar_thread_self());

    MSG msg;
    int rc = 0;
    while(thr->started && rc >= 0 && rc <= 1) {
        _zbar_mutex_unlock(&vdo->qlock);

        rc = MsgWaitForMultipleObjects(1, &thr->notify, 0,
                                       INFINITE, QS_ALLINPUT);
        if (rc == 1)
            while(PeekMessage(&msg, NULL, 0, 0, PM_NOYIELD | PM_REMOVE))
                if (rc > 0) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

        _zbar_mutex_lock(&vdo->qlock);
    }

//done:
    thr->running = 0;
    _zbar_event_trigger(&thr->activity);
    _zbar_mutex_unlock(&vdo->qlock);
    return 0;
}

static int dshow_nq(zbar_video_t* vdo, zbar_image_t* img)
{
    zprintf(16, "img: %p (srcidx: %d, data: %p, len: %ld), thr=%04lx\n", img, img->srcidx, img->data, img->datalen, _zbar_thread_self());
    return video_nq_image(vdo, img);
}

/// Platform dependent part of #zbar_video_next_image, which blocks
/// until an image is available.
/** Must be called with video lock held and returns
  * with the lock released.
  * <p>Waits for the image from `vdo->state->image`. If available,
  * this field is nulled. Releases the lock temporarily when waiting for
  * the `vdo->state->captured` signal. */
static zbar_image_t* dshow_dq(zbar_video_t* vdo)
{
    zbar_image_t* img = vdo->state->image;
    if (!img) {
        _zbar_mutex_unlock(&vdo->qlock);
        DWORD rc = WaitForSingleObject(vdo->state->captured, INFINITE);
            // note: until we get the lock again the grabber thread might
            // already provide the next sample (which is fine)
        _zbar_mutex_lock(&vdo->qlock);

        switch (rc)
        {
        case WAIT_OBJECT_0:
            img = vdo->state->image;
            break;
        case WAIT_ABANDONED:
            err_capture(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                        "event handle abandoned");
            break;
        case WAIT_FAILED:
            err_capture(vdo, SEV_ERROR, ZBAR_ERR_WINAPI, __func__,
                        "Waiting for image failed");
            break;
        }
    }

    zprintf(16, "img: %p (srcidx: %d, data: %p, len: %ld), thr=%04lx\n", img, img ? img->srcidx : 0, img ? img->data : NULL, img ? img->datalen : 0, _zbar_thread_self());

    vdo->state->image = NULL;
    ResetEvent(vdo->state->captured);
    video_unlock(vdo);

    return img;
}

static int dshow_start(zbar_video_t* vdo)
{
    video_state_t* state = vdo->state;
    ResetEvent(state->captured);

    zprintf(16, "thr=%04lx\n", _zbar_thread_self());

    HRESULT hr = IMediaControl_Run(state->mediacontrol);
    CHECK_COM_ERROR(hr, "couldn't start video stream", (void)0);
    if (FAILED(hr))
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                           "starting video stream"));
    return 0;
}

static int dshow_stop(zbar_video_t* vdo)
{
    video_state_t* state = vdo->state;

    zprintf(16, "thr=%04lx\n", _zbar_thread_self());

    HRESULT hr = IMediaControl_Stop(state->mediacontrol);
    CHECK_COM_ERROR(hr, "couldn't stop video stream", (void)0);
    if (FAILED(hr))
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                           "stopping video stream"));

    _zbar_mutex_lock(&vdo->qlock);
    if (state->image)
        state->image = NULL;
    SetEvent(state->captured);
    _zbar_mutex_unlock(&vdo->qlock);
    return 0;
}

static int dshow_set_format (zbar_video_t* vdo,
                           uint32_t fmt)
{
    int rc = 0; // return code

    const zbar_format_def_t* fmtdef = _zbar_format_lookup(fmt);
    int fmt_ind = get_format_index(vdo->formats, fmt);
    if (!fmtdef->format || fmt_ind < 0)
        return(err_capture_int(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                               "unsupported vfw format: %x", fmt));

    video_state_t* state = vdo->state;
    int_format_t *int_fmt = &state->int_formats[fmt_ind];


    // prepare media type structure as read from GetStreamCaps
    BYTE* caps = malloc(state->caps_size);
    if (!caps) err_capture(vdo, SEV_FATAL, ZBAR_ERR_NOMEM, __func__, "");
    AM_MEDIA_TYPE* mt = NULL;
    AM_MEDIA_TYPE* currentmt = NULL;  // used later, but must be here
                                      // due to possible "goto cleanup"
    HRESULT hr = IAMStreamConfig_GetStreamCaps(state->camstreamconfig,
        int_fmt->idx_caps, &mt, caps);
    free(caps);
    CHECK_COM_ERROR(hr, "querying chosen stream caps failed", goto cleanup)
    BITMAPINFOHEADER* bih = dshow_access_bih(mt);

    // then, adjust the format
    if (!vdo->width || !vdo->height)
    {
        // video size not requested, take camera default
        resolution_t resolution = vdo->state->def_resolution;
        // the initial resolution may be not suitable for the current format
        get_closest_resolution(&resolution, &int_fmt->resolutions);
        vdo->width = resolution.cx;
        vdo->height = resolution.cy;
    }
    bih->biWidth = vdo->width;
    bih->biHeight = vdo->height;

    zprintf(4, "setting camera format: %.4s(%08x) " BIH_FMT "\n",
            (char*)&int_fmt->fourcc, int_fmt->fourcc, BIH_FIELDS(bih));

    hr = IAMStreamConfig_SetFormat(state->camstreamconfig, mt);
    CHECK_COM_ERROR(hr, "setting camera format failed", goto cleanup)

    // re-read format, image data size might have changed
    hr = IAMStreamConfig_GetFormat(state->camstreamconfig, &currentmt);
    CHECK_COM_ERROR(hr, "queried currentmt", goto cleanup);

    bih = dshow_access_bih(currentmt);

    if (get_fourcc_for_mt(currentmt) != int_fmt->fourcc)
    {
        rc = err_capture(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                         "video format set ignored");
        goto cleanup;
    }

    vdo->format = fmt;
    vdo->width = bih->biWidth;
    vdo->height = bih->biHeight;
    vdo->datalen = bih->biSizeImage;

    // datalen was set based on the internal format, but sometimes it's
    // different from the format reported to zbar processor
    if (vdo->formats[fmt_ind] != state->int_formats[fmt_ind].fourcc) {
        // See prepare_mjpg_format_mapping for possible differences
        // between internal and zbar format.
        vdo->datalen = BMP_SIZE(vdo->width, vdo->height,
                                mjpg_conversion_fmt_bpp);
    }

    zprintf(4, "set camera format: %.4s(%08x) " BIH_FMT "\n",
            (char*)&int_fmt->fourcc, int_fmt->fourcc, BIH_FIELDS(bih));

cleanup:
    DeleteMediaType(mt);
    DeleteMediaType(currentmt);

    if (FAILED(hr))
        return err_capture(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                           "setting dshow format failed");
    else
        // note: if an error happened it was already captured and reported
        return rc;
}

static int dshow_init(zbar_video_t* vdo, uint32_t fmt)
{
    if (dshow_set_format(vdo, fmt))
        return -1;


    HRESULT hr;
    video_state_t* state = vdo->state;
    int fmt_ind = get_format_index(vdo->formats, fmt);


    // install sample grabber callback
    ISampleGrabberCB* grabbercb = (ISampleGrabberCB*)new_zbar_samplegrabber_cb(vdo);
    hr = ISampleGrabber_SetCallback(vdo->state->samplegrabber, grabbercb, 1);
    ISampleGrabberCB_Release(grabbercb);
    if (FAILED(hr))
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_BUSY, __func__,
                           "setting capture callbacks"));



    // set up directshow graph


    // special handling for MJPG streams:
    // we use the stock mjpeg decompressor filter
    if (state->int_formats[fmt_ind].fourcc == fourcc('M','J','P','G'))
    {
        IBaseFilter* mjpgdecompressor = NULL;
        hr = CoCreateInstance(&CLSID_MjpegDec, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (void**)&mjpgdecompressor);
        CHECK_COM_ERROR(hr, "failed to create mjpeg decompressor filter", (void)0)
        if (FAILED(hr))
        {
            return err_capture(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                               "failed to create mjpeg decompressor filter");
        }

        // add mjpeg decompressor to graph
        hr = IGraphBuilder_AddFilter(state->graph, mjpgdecompressor, L"MJPEG decompressor");
        CHECK_COM_ERROR(hr, "adding MJPEG decompressor", goto mjpg_cleanup)

        // explicitly convert MJPG to RGB32
        // (sample grabber will only accept this format)
        //
        // note: the mjpeg decompressor's output seems to be RGB32 (BGR4)
        // by default.
        // This fact has been a decisive factor to choosing the mapping
        // (BGR4 [zbar input] -> MJPG [camera output]).
        // Because zbar requests BGR4 we have to ensure that we really do
        // provide it.
        AM_MEDIA_TYPE conv_mt = {};
        conv_mt.majortype = MEDIATYPE_Video;
        conv_mt.subtype = *mjpg_conversion_mediatype;
        conv_mt.formattype = FORMAT_VideoInfo;
        hr = ISampleGrabber_SetMediaType(state->samplegrabber, &conv_mt);
        CHECK_COM_ERROR(hr, "setting mjpg conversion media type", goto mjpg_cleanup)
        // no need to destroy conv_mt


        hr = ICaptureGraphBuilder2_RenderStream(state->builder, &PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, (IUnknown*)state->camera, NULL, mjpgdecompressor);
        CHECK_COM_ERROR(hr, "rendering filter graph 1", goto mjpg_cleanup)

        hr = ICaptureGraphBuilder2_RenderStream(state->builder, NULL, &MEDIATYPE_Video, (IUnknown*)mjpgdecompressor, state->grabberbase, state->nullrenderer);
        CHECK_COM_ERROR(hr, "rendering filter graph 2", goto mjpg_cleanup)

mjpg_cleanup:
        IBaseFilter_Release(mjpgdecompressor);

        if (FAILED(hr))
        {
            return err_capture(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                               "rendering filter graph failed");
        }
    }
    else
    {
        // ensure (again) that the sample grabber gets only
        // video media types with VIDEOINFOHEADER
        AM_MEDIA_TYPE grab_mt = {};
        grab_mt.majortype = MEDIATYPE_Video;
        grab_mt.formattype = FORMAT_VideoInfo;
        hr = ISampleGrabber_SetMediaType(state->samplegrabber, &grab_mt);
        CHECK_COM_ERROR(hr, "setting sample grabber media type", goto render_cleanup)
        // no need to destroy grab_mt


        hr = ICaptureGraphBuilder2_RenderStream(state->builder, &PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, (IUnknown*)state->camera, state->grabberbase, state->nullrenderer);
        CHECK_COM_ERROR(hr, "rendering filter graph", goto render_cleanup)

render_cleanup:
        if (FAILED(hr))
        {
            return err_capture(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                               "rendering filter graph failed");
        }
    }

    // scope: after the graph is built (and the pins connected) we query the
    // final media type from sample grabber's input pin;
    {
        AM_MEDIA_TYPE input_mt = {};

        hr = ISampleGrabber_GetConnectedMediaType(state->samplegrabber, &input_mt);
        CHECK_COM_ERROR(hr, "couldn't query input media type from sample grabber", goto cleanup1)


        assert(dshow_has_vih(&input_mt));
        const BITMAPINFOHEADER* bih = dshow_caccess_bih(&input_mt);

        // adjust state->bih
        state->bi_size = bih->biSize;
        state->bih = realloc(state->bih, state->bi_size);
        memcpy(state->bih, bih, state->bi_size);

        if (bih->biCompression == BI_RGB)
        {
            // check bitmap orientation for native BI_RGB:
            // positive biHeight: bottom-up bitmap -> flip
            // negative biHeight: top-down bitmap
            state->do_flip_bitmap = (bih->biHeight > 0);
        }

cleanup1:
        DestroyMediaType(&input_mt);

        if (FAILED(hr))
            return -1;
    }

    // query camera stream parameters;
    REFERENCE_TIME avgtime_perframe = 0;

    // scope: query avgtime_perframe from camera
    {
        AM_MEDIA_TYPE* currentmt = NULL;
        hr = IAMStreamConfig_GetFormat(state->camstreamconfig, &currentmt);
        CHECK_COM_ERROR(hr, "querying camera format failed", goto cleanup2)

        assert(dshow_has_vih(currentmt));
        VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*) currentmt->pbFormat;
        avgtime_perframe = vih->AvgTimePerFrame;


cleanup2:
        DeleteMediaType(currentmt);
    }

    // directshow keeps ownership of the image passed to the callback,
    // hence, keeping a pointer to the original data (iomode VIDEO_MMAP) is a bad idea
    // in a multi-threaded environment.
    // real case szenario on a windows 7 tablet with intel atom:
    // with vdo->iomode=VIDEO_MMAP and vdo->num_images=1:
    // 1) directshow graph provides a sample, sets the data pointer of vdo->state->img
    // 2) dshow_dq (called from proc_video_thread()/zbar_video_next_image()) fetches the image and makes a copy of it
    // 3) directshow graph provides the next sample, sets the data pointer of vdo->state->img
    // 4) dshow_nq (called from ) resets the data pointer of vdo->state->img (nullptr)
    // 5) dshow_dq returns vdo->state->img without data (nullptr)
    //
    // now, we could deal with this special case, but zbar_video_next_image() makes a copy of the sample anyway when
    // vdo->num_images==1 (thus VIDEO_MMAP won't save us anything); therefore rather use image buffers provided
    // by zbar (see video_init_images())
    vdo->iomode = VIDEO_USERPTR;
    // keep zbar's default
    //vdo->num_images = ZBAR_VIDEO_IMAGES_MAX;

    // note: vdo->format and vdo->datalen have been set accordingly in dshow_set_format()

    zprintf(3, "initialized video capture: %.4s(%08x), %"PRId64" frames/s\n",
            (char*)&fmt, fmt, _100ns_unit / avgtime_perframe);


    return 0;
}

static int dshow_cleanup (zbar_video_t* vdo)
{
    zprintf(16, "thr=%04lx\n", _zbar_thread_self());

    /* close open device */
    video_state_t* state = vdo->state;

    _zbar_thread_stop(&state->thread, &vdo->qlock);

    dshow_destroy_video_state_t(state);
    free(state);
    vdo->state = NULL;

    CoUninitialize();

    return 0;
}

static int dshow_determine_formats(zbar_video_t* vdo)
{
    video_state_t* state = vdo->state;

    // collect formats
    int resolutions;
    HRESULT hr = IAMStreamConfig_GetNumberOfCapabilities(
        state->camstreamconfig, &resolutions, &state->caps_size);
    CHECK_COM_ERROR(hr, "couldn't query camera capabilities", return -1)

    zprintf(6, "number of formats/resolutions supported by the camera as reported by directshow: %d\n", resolutions);
    zprintf(6, "list of those with a VIDEOINFOHEADER:\n");

    vdo->formats = calloc(resolutions+1, sizeof(uint32_t));
    state->int_formats = calloc(resolutions+1, sizeof(int_format_t));

    // this is actually a VIDEO_STREAM_CONFIG_CAPS structure, which is mostly deprecated anyway,
    // so we just reserve enough buffer but treat it as opaque otherwise
    BYTE* caps = malloc(state->caps_size);
    int n = 0, i;
    for (i = 0; i < resolutions; ++i)
    {
        AM_MEDIA_TYPE* mt;
        HRESULT hr = IAMStreamConfig_GetStreamCaps(state->camstreamconfig, i, &mt, caps);
        CHECK_COM_ERROR(hr, "querying stream capability failed", continue)
        int is_supported = 0;

        if (dshow_has_vih(mt))
        {
            const BITMAPINFOHEADER* bih = dshow_caccess_bih(mt);

            zprintf(6, BIH_FMT "\n",
                    BIH_FIELDS(bih));
            uint32_t fmt = get_fourcc_for_mt(mt);
            // This is actually a check if the format is recognized.
            // TODO: Check if the format is really supported by zbar.
            is_supported = (fmt != 0);

            if (is_supported)
            {
                // first search for existing fourcc format
                int j;
                for (j = 0; j < n; ++j)
                {
                    if (state->int_formats[i].fourcc == fmt)
                        break;
                }
                // push back if not found
                if (j == n)
                {
                    state->int_formats[n].fourcc = fmt;
                    state->int_formats[n].idx_caps = i;
                    resolution_list_init(&state->int_formats[n].resolutions);
                    vdo->formats[n] = fmt;
                    ++n;
                }
                resolution_t resolution = { bih->biWidth, bih->biHeight };
                resolution_list_add(&state->int_formats[j].resolutions,
                                    &resolution);
            }
        }
        // note: other format types could be possible, e.g. VIDEOINFOHEADER2 ...

        if (!is_supported)
        {
            char *formattype = get_clsid_string(&mt->formattype);
            char *subtype = get_clsid_string(&mt->subtype);
            zprintf(6, "unsupported format: %s / %s\n",
                formattype, subtype);
            free(formattype);
            free(subtype);
        }

        DeleteMediaType(mt);
    }
    free(caps);

    zprintf(6, "number of supported fourcc formats: %d\n", n);

    vdo->formats = realloc(vdo->formats, (n + 1) * sizeof(uint32_t));
    state->int_formats = realloc(state->int_formats,
                                 (n + 1) * sizeof(int_format_t));
    prepare_mjpg_format_mapping(vdo);
    dump_formats(vdo);

    if (n == 0)
        return -1;

    return 0;
}

static int dshow_probe(zbar_video_t* vdo)
{
    if (dshow_determine_formats(vdo))
        return -1;


    video_state_t* state = vdo->state;


    // query current format

    AM_MEDIA_TYPE* currentmt;
    HRESULT hr = IAMStreamConfig_GetFormat(state->camstreamconfig, &currentmt);
    CHECK_COM_ERROR(hr, "couldn't query current camera format", return -1)

    if (!dshow_has_vih(currentmt))
    {
        char *formattype = get_clsid_string(&currentmt->formattype);
        char *subtype = get_clsid_string(&currentmt->subtype);
        zprintf(1, "encountered unsupported initial format, "
                   "no VIDEOINFOHEADER video format type: %s / %s\n",
                formattype, subtype);
        free(formattype);
        free(subtype);

        // if we cannot read the current resolution, we need sane defaults
        vdo->state->def_resolution.cx = 640;
        vdo->state->def_resolution.cy = 480;
    }
    else
    {
        const BITMAPINFOHEADER* current_bih = dshow_caccess_bih(currentmt);
        assert(current_bih);
        zprintf(3, "initial format: %.4s(%08lx) " BIH_FMT "\n",
               (char*)&current_bih->biCompression, current_bih->biCompression,
                BIH_FIELDS(current_bih));

        // store initial size
        vdo->state->def_resolution.cx = current_bih->biWidth;
        vdo->state->def_resolution.cy = current_bih->biHeight;
    }

    DeleteMediaType(currentmt);


    vdo->intf = VIDEO_DSHOW;
    vdo->init = dshow_init;
    vdo->start = dshow_start;
    vdo->stop = dshow_stop;
    vdo->cleanup = dshow_cleanup;
    vdo->nq = dshow_nq;
    vdo->dq = dshow_dq;

    return 0;
}

// search camera by index or by device path
static IBaseFilter* dshow_search_camera(const char* dev)
{
    IBaseFilter* camera = NULL;
    ICreateDevEnum* devenumerator = NULL;
    IEnumMoniker* enummoniker = NULL;

    int reqid = -1;
    if ((!strncmp(dev, "/dev/video", 10) ||
        !strncmp(dev, "\\dev\\video", 10)) &&
       dev[10] >= '0' && dev[10] <= '9' && !dev[11])
        reqid = dev[10] - '0';
    else if (strlen(dev) == 1 &&
            dev[0] >= '0' && dev[0] <= '9')
        reqid = dev[0] - '0';

    zprintf(6, "searching for camera (#%d): %s\n", reqid, dev);

    HRESULT hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, &IID_ICreateDevEnum, (void**)&devenumerator);
    CHECK_COM_ERROR(hr, "failed to create system device enumerator", goto done)

    hr = ICreateDevEnum_CreateClassEnumerator(devenumerator, &CLSID_VideoInputDeviceCategory, &enummoniker, 0);
    CHECK_COM_ERROR(hr, "failed to create enumerator moniker", goto done)

    if (hr != S_OK)
    {
        zprintf(6, "no video devices available");
        goto done;
    }


    // turn device name (the GUID) from char to wide char
    BSTR wdev = SysAllocStringLen(NULL, strlen(dev));
    if (!wdev)
        goto done;
    MultiByteToWideChar(CP_UTF8, 0, dev, -1, wdev, strlen(dev) + 1);

    // Go through and find capture device
    int docontinue;
    int devid;
    for (devid = 0, docontinue = 1; docontinue; ++devid)
    {
        IMoniker* moniker = NULL;
        IPropertyBag* propbag = NULL;
        VARIANT devpath_variant;
        VARIANT friendlyname_variant;

        hr = IEnumMoniker_Next(enummoniker, 1, &moniker, NULL);
        // end of monikers
        if (hr != S_OK)
            break;

        VariantInit(&devpath_variant);
        VariantInit(&friendlyname_variant);

        hr = IMoniker_BindToStorage(moniker, NULL, NULL, &IID_IPropertyBag, (void**)&propbag);
        CHECK_COM_ERROR(hr, "failed to get property bag from moniker", goto breakout)

        hr = IPropertyBag_Read(propbag, L"DevicePath", &devpath_variant, NULL);
        CHECK_COM_ERROR(hr, "failed to read DevicePath from camera device", goto breakout)
        hr = IPropertyBag_Read(propbag, L"FriendlyName", &friendlyname_variant, NULL);
        CHECK_COM_ERROR(hr, "failed to read FriendlyName from camera device", goto breakout)

        if ((reqid >= 0)
           ? devid == reqid
           : !wcscmp(wdev, V_BSTR(&devpath_variant)))
        {
            // create camera from moniker
            hr = IMoniker_BindToObject(moniker, NULL, NULL, &IID_IBaseFilter, (void**)&camera);
            CHECK_COM_ERROR(hr, "failed to get camera device", goto breakout)
        }

        if (camera)
        {
            zwprintf(1, L"using camera #%d: %s (%s)\n", devid, V_BSTR(&friendlyname_variant), V_BSTR(&devpath_variant));
            goto breakout;
        }
        else
            goto cleanup;


breakout:
        docontinue = 0;
cleanup:
        VariantClear(&friendlyname_variant);
        VariantClear(&devpath_variant);
        COM_SAFE_RELEASE(&propbag);
        COM_SAFE_RELEASE(&moniker);
    }
    SysFreeString(wdev);

done:
    COM_SAFE_RELEASE(&enummoniker);
    COM_SAFE_RELEASE(&devenumerator);

    if (!camera)
        zprintf(6, "no camera found\n");

    return camera;
}

int _zbar_video_open (zbar_video_t* vdo, const char* dev)
{
    // assume failure
    int ret = -1;


    video_state_t* state;
    state = vdo->state = calloc(1, sizeof(video_state_t));

    state->camera = dshow_search_camera(dev);
    if (!state->camera)
        goto done;


    if (!state->captured)
        // create manual reset event
        state->captured = CreateEvent(NULL, TRUE, FALSE, NULL);
    else
        ResetEvent(state->captured);

    if (_zbar_thread_start(&state->thread, dshow_capture_thread, vdo, NULL))
        return -1;


    // create filter graph instance
    HRESULT hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IGraphBuilder, (void**)&state->graph);
    CHECK_COM_ERROR(hr, "graph builder creation", goto done)

    // query media control from filter graph
    hr = IGraphBuilder_QueryInterface(state->graph, &IID_IMediaControl, (void**)&state->mediacontrol);
    CHECK_COM_ERROR(hr, "querying media control", goto done)

    // create sample grabber instance
    hr = CoCreateInstance(&CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, &IID_ISampleGrabber, (void**)&state->samplegrabber);
    CHECK_COM_ERROR(hr, "samplegrabber creation", goto done)

    // query base filter interface from sample grabber
    hr = ISampleGrabber_QueryInterface(state->samplegrabber, &IID_IBaseFilter, (void**)&state->grabberbase);
    CHECK_COM_ERROR(hr, "grabberbase query", goto done)

    // capture graph without preview window
    hr = CoCreateInstance(&CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (void**)&state->nullrenderer);
    CHECK_COM_ERROR(hr, "null renderer creation", goto done)


    // add camera to graph
    hr = IGraphBuilder_AddFilter(state->graph, state->camera, L"Camera");
    CHECK_COM_ERROR(hr, "adding camera", goto done)

    // add sample grabber to graph
    hr = IGraphBuilder_AddFilter(state->graph, state->grabberbase, L"Sample Grabber");
    CHECK_COM_ERROR(hr, "adding samplegrabber", goto done)

    // add nullrenderer to graph
    hr = IGraphBuilder_AddFilter(state->graph, state->nullrenderer, L"Null Renderer");
    CHECK_COM_ERROR(hr, "adding null renderer", goto done)


    // Create the Capture Graph Builder.
    hr = CoCreateInstance(&CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, &IID_ICaptureGraphBuilder2, (void**)&state->builder);
    CHECK_COM_ERROR(hr, "capturegraph builder creation", goto done)

    // tell graph builder about the filter graph
    hr = ICaptureGraphBuilder2_SetFiltergraph(state->builder, state->graph);
    CHECK_COM_ERROR(hr, "setting filtergraph", goto done)

    // TODO:
    // finding the streamconfig interface on the camera's preview output pin (specifying PIN_CATEGORY_PREVIEW, MEDIATYPE_Video)
    // should work according to the documentation but it doesn't.
    // Because devices may have separate pins for capture and preview or have a video port pin (PIN_CATEGORY_VIDEOPORT)
    // instead of a preview pin, I do hope that we get the streamconfig interface for the correct pin
    hr = ICaptureGraphBuilder2_FindInterface(state->builder, &LOOK_DOWNSTREAM_ONLY, NULL, state->camera, &IID_IAMStreamConfig, (void**)&state->camstreamconfig);
    CHECK_COM_ERROR(hr, "querying camera's streamconfig interface", goto done)


    if (dshow_probe(vdo))
        goto done;


    // success
    ret = 0;

done:
    if (ret)
    {
        if (state->camera)
            _zbar_thread_stop(&state->thread, NULL);
        dshow_destroy_video_state_t(state);
        free(state);
        vdo->state = NULL;
        CoUninitialize();

        return err_capture_str(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                               "failed to connect to camera '%s'", dev);
    }

    return ret;
}
