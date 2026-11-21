#define WIN32_LEAN_AND_MEAN

#include <initguid.h>
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfvirtualcamera.h>
#include <mferror.h>
#include <ks.h>
#include <ksmedia.h>
#include <propvarutil.h>
#include <strsafe.h>
#include <wil/com.h>
#include <wil/resource.h>
#include <wil/result.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <mutex>
#include <string>
#include <vector>

#include <digitalcam/virtual_camera/windows/SharedFrame.h>

namespace {

using namespace digitalcam::virtual_camera::windows;

DEFINE_GUID(kVirtualCameraClsid, 0x7c2e5ad1, 0x9f43, 0x4b6e, 0xa1, 0x2d, 0x8c, 0x64, 0x3e, 0x51, 0xb7, 0x0a);
DEFINE_GUID(kPinNameVideoCapture, 0xfb6c4281, 0x0353, 0x11d1, 0x90, 0x5f, 0x00, 0x00, 0xc0, 0xcc, 0x16, 0xba);

std::atomic<ULONG> g_moduleReferences{0};
HMODULE g_module = nullptr;

class ComObject {
public:
    ComObject() {
        g_moduleReferences.fetch_add(1);
    }

    virtual ~ComObject() = default;

    STDMETHODIMP_(ULONG) AddRef() {
        g_moduleReferences.fetch_add(1);
        return references_.fetch_add(1) + 1;
    }

    STDMETHODIMP_(ULONG) Release() {
        g_moduleReferences.fetch_sub(1);
        const ULONG remaining = references_.fetch_sub(1) - 1;
        if (remaining == 0) {
            delete this;
        }
        return remaining;
    }

private:
    std::atomic<ULONG> references_{1};
};

template <class Interface>
HRESULT handoff(Interface* value, REFIID riid, void** result) {
    if (!result) {
        return E_POINTER;
    }
    *result = nullptr;
    if (IsEqualIID(riid, __uuidof(Interface))) {
        *result = static_cast<void*>(value);
        value->AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

class AttributeSink {
protected:
    wil::com_ptr_nothrow<IMFAttributes> attributes_;

    HRESULT initializeAttributes() {
        return MFCreateAttributes(attributes_.put(), 0);
    }

public:
    STDMETHODIMP GetItem(REFGUID key, PROPVARIANT* value) {
        return attributes_ ? attributes_->GetItem(key, value) : E_UNEXPECTED;
    }

    STDMETHODIMP GetItemType(REFGUID key, MF_ATTRIBUTE_TYPE* type) {
        return attributes_ ? attributes_->GetItemType(key, type) : E_UNEXPECTED;
    }

    STDMETHODIMP CompareItem(REFGUID key, REFPROPVARIANT value, BOOL* result) {
        return attributes_ ? attributes_->CompareItem(key, value, result) : E_UNEXPECTED;
    }

    STDMETHODIMP Compare(IMFAttributes* other, MF_ATTRIBUTES_MATCH_TYPE type, BOOL* result) {
        return attributes_ ? attributes_->Compare(other, type, result) : E_UNEXPECTED;
    }

    STDMETHODIMP GetUINT32(REFGUID key, UINT32* value) {
        return attributes_ ? attributes_->GetUINT32(key, value) : E_UNEXPECTED;
    }

    STDMETHODIMP GetUINT64(REFGUID key, UINT64* value) {
        return attributes_ ? attributes_->GetUINT64(key, value) : E_UNEXPECTED;
    }

    STDMETHODIMP GetDouble(REFGUID key, double* value) {
        return attributes_ ? attributes_->GetDouble(key, value) : E_UNEXPECTED;
    }

    STDMETHODIMP GetGUID(REFGUID key, GUID* value) {
        return attributes_ ? attributes_->GetGUID(key, value) : E_UNEXPECTED;
    }

    STDMETHODIMP GetStringLength(REFGUID key, UINT32* length) {
        return attributes_ ? attributes_->GetStringLength(key, length) : E_UNEXPECTED;
    }

    STDMETHODIMP GetString(REFGUID key, LPWSTR value, UINT32 size, UINT32* length) {
        return attributes_ ? attributes_->GetString(key, value, size, length) : E_UNEXPECTED;
    }

    STDMETHODIMP GetAllocatedString(REFGUID key, LPWSTR* value, UINT32* length) {
        return attributes_ ? attributes_->GetAllocatedString(key, value, length) : E_UNEXPECTED;
    }

    STDMETHODIMP GetBlobSize(REFGUID key, UINT32* size) {
        return attributes_ ? attributes_->GetBlobSize(key, size) : E_UNEXPECTED;
    }

    STDMETHODIMP GetBlob(REFGUID key, UINT8* value, UINT32 size, UINT32* length) {
        return attributes_ ? attributes_->GetBlob(key, value, size, length) : E_UNEXPECTED;
    }

    STDMETHODIMP GetAllocatedBlob(REFGUID key, UINT8** value, UINT32* length) {
        return attributes_ ? attributes_->GetAllocatedBlob(key, value, length) : E_UNEXPECTED;
    }

    STDMETHODIMP GetUnknown(REFGUID key, REFIID riid, LPVOID* value) {
        return attributes_ ? attributes_->GetUnknown(key, riid, value) : E_UNEXPECTED;
    }

    STDMETHODIMP SetItem(REFGUID key, REFPROPVARIANT value) {
        return attributes_ ? attributes_->SetItem(key, value) : E_UNEXPECTED;
    }

    STDMETHODIMP DeleteItem(REFGUID key) {
        return attributes_ ? attributes_->DeleteItem(key) : E_UNEXPECTED;
    }

    STDMETHODIMP DeleteAllItems() {
        return attributes_ ? attributes_->DeleteAllItems() : E_UNEXPECTED;
    }

    STDMETHODIMP SetUINT32(REFGUID key, UINT32 value) {
        return attributes_ ? attributes_->SetUINT32(key, value) : E_UNEXPECTED;
    }

    STDMETHODIMP SetUINT64(REFGUID key, UINT64 value) {
        return attributes_ ? attributes_->SetUINT64(key, value) : E_UNEXPECTED;
    }

    STDMETHODIMP SetDouble(REFGUID key, double value) {
        return attributes_ ? attributes_->SetDouble(key, value) : E_UNEXPECTED;
    }

    STDMETHODIMP SetGUID(REFGUID key, REFGUID value) {
        return attributes_ ? attributes_->SetGUID(key, value) : E_UNEXPECTED;
    }

    STDMETHODIMP SetString(REFGUID key, LPCWSTR value) {
        return attributes_ ? attributes_->SetString(key, value) : E_UNEXPECTED;
    }

    STDMETHODIMP SetBlob(REFGUID key, const UINT8* value, UINT32 size) {
        return attributes_ ? attributes_->SetBlob(key, value, size) : E_UNEXPECTED;
    }

    STDMETHODIMP SetUnknown(REFGUID key, IUnknown* value) {
        return attributes_ ? attributes_->SetUnknown(key, value) : E_UNEXPECTED;
    }

    STDMETHODIMP LockStore() {
        return attributes_ ? attributes_->LockStore() : E_UNEXPECTED;
    }

    STDMETHODIMP UnlockStore() {
        return attributes_ ? attributes_->UnlockStore() : E_UNEXPECTED;
    }

    STDMETHODIMP GetCount(UINT32* count) {
        return attributes_ ? attributes_->GetCount(count) : E_UNEXPECTED;
    }

    STDMETHODIMP GetItemByIndex(UINT32 index, GUID* key, PROPVARIANT* value) {
        return attributes_ ? attributes_->GetItemByIndex(index, key, value) : E_UNEXPECTED;
    }

    STDMETHODIMP CopyAllItems(IMFAttributes* destination) {
        return attributes_ ? attributes_->CopyAllItems(destination) : E_UNEXPECTED;
    }
};

class MediaStream final : public ComObject, public IMFMediaStream2, public IKsControl, public AttributeSink {
public:
    MediaStream() {
        initializeAttributes();
    }

    STDMETHODIMP QueryInterface(REFIID riid, void** result) {
        if (!result) {
            return E_POINTER;
        }
        if (IsEqualIID(riid, __uuidof(IUnknown))) {
            *result = static_cast<IUnknown*>(static_cast<IMFMediaStream2*>(this));
            AddRef();
            return S_OK;
        }
        if (IsEqualIID(riid, __uuidof(IMFMediaStream))) {
            *result = static_cast<IMFMediaStream*>(this);
            AddRef();
            return S_OK;
        }
        if (IsEqualIID(riid, __uuidof(IMFMediaStream2))) {
            *result = static_cast<IMFMediaStream2*>(this);
            AddRef();
            return S_OK;
        }
        if (IsEqualIID(riid, __uuidof(IMFMediaEventGenerator))) {
            *result = static_cast<IMFMediaEventGenerator*>(this);
            AddRef();
            return S_OK;
        }
        if (IsEqualIID(riid, __uuidof(IMFAttributes))) {
            *result = static_cast<IMFAttributes*>(static_cast<IMFMediaStream2*>(this));
            AddRef();
            return S_OK;
        }
        if (IsEqualIID(riid, __uuidof(IKsControl))) {
            *result = static_cast<IKsControl*>(this);
            AddRef();
            return S_OK;
        }
        *result = nullptr;
        return E_NOINTERFACE;
    }

    HRESULT initialize(IMFMediaSource* source, std::uint32_t index) {
        if (!source) {
            return E_POINTER;
        }
        source_ = source;
        index_ = index;
        RETURN_IF_FAILED(SetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, kPinNameVideoCapture));
        RETURN_IF_FAILED(SetUINT32(MF_DEVICESTREAM_STREAM_ID, index_));
        RETURN_IF_FAILED(SetUINT32(MF_DEVICESTREAM_FRAMESERVER_SHARED, 1));
        RETURN_IF_FAILED(SetUINT32(
            MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES,
            static_cast<UINT32>(MFFrameSourceTypes::MFFrameSourceTypes_Color)
        ));
        RETURN_IF_FAILED(MFCreateEventQueue(queue_.put()));

        FrameInfo info{};
        if (!reader_.open() || !reader_.info(info)) {
            info.width = 1280;
            info.height = 720;
            info.fpsNumerator = 30000;
            info.fpsDenominator = 1000;
            info.pixelFormat = kPixelFormatBgra;
        }

        wil::com_ptr_nothrow<IMFMediaType> type;
        RETURN_IF_FAILED(MFCreateMediaType(type.put()));
        RETURN_IF_FAILED(type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
        RETURN_IF_FAILED(type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32));
        RETURN_IF_FAILED(MFSetAttributeSize(
            type.get(),
            MF_MT_FRAME_SIZE,
            info.width,
            info.height
        ));
        RETURN_IF_FAILED(type->SetUINT32(MF_MT_DEFAULT_STRIDE, info.width * 4));
        RETURN_IF_FAILED(type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
        RETURN_IF_FAILED(type->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));
        RETURN_IF_FAILED(MFSetAttributeRatio(
            type.get(),
            MF_MT_FRAME_RATE,
            info.fpsNumerator,
            info.fpsDenominator
        ));
        RETURN_IF_FAILED(MFSetAttributeRatio(type.get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1));

        const std::uint64_t bitrate = static_cast<std::uint64_t>(info.width) *
            info.height * 4 * 8 * info.fpsNumerator / info.fpsDenominator;
        RETURN_IF_FAILED(type->SetUINT32(
            MF_MT_AVG_BITRATE,
            static_cast<UINT32>(std::min<std::uint64_t>(bitrate, 0xFFFFFFFFULL))
        ));

        IMFMediaType* types[] = {type.get()};
        RETURN_IF_FAILED(MFCreateStreamDescriptor(
            index_,
            1,
            types,
            descriptor_.put()
        ));

        wil::com_ptr_nothrow<IMFMediaTypeHandler> handler;
        RETURN_IF_FAILED(descriptor_->GetMediaTypeHandler(handler.put()));
        RETURN_IF_FAILED(handler->SetCurrentMediaType(type.get()));
        return S_OK;
    }

    HRESULT setAllocator(IUnknown* allocator) {
        if (!allocator) {
            return E_POINTER;
        }
        allocator_.reset();
        return allocator->QueryInterface(allocator_.put());
    }

    HRESULT setD3DManager(IUnknown* manager) {
        if (!manager) {
            return E_POINTER;
        }
        return allocator_ ? allocator_->SetDirectXManager(manager) : S_OK;
    }

    MFSampleAllocatorUsage allocatorUsage() const {
        return MFSampleAllocatorUsage_UsesProvidedAllocator;
    }

    HRESULT start(IMFMediaType* type) {
        std::lock_guard<std::recursive_mutex> lock(lock_);
        if (!queue_ || !allocator_) {
            return MF_E_SHUTDOWN;
        }
        if (type) {
            RETURN_IF_FAILED(type->GetGUID(MF_MT_SUBTYPE, &format_));
        }
        RETURN_IF_FAILED(allocator_->InitializeSampleAllocator(10, type));
        RETURN_IF_FAILED(queue_->QueueEventParamVar(MEStreamStarted, GUID_NULL, S_OK, nullptr));
        state_ = MF_STREAM_STATE_RUNNING;
        return S_OK;
    }

    HRESULT stop() {
        std::lock_guard<std::recursive_mutex> lock(lock_);
        if (!queue_) {
            return MF_E_SHUTDOWN;
        }
        if (allocator_) {
            allocator_->UninitializeSampleAllocator();
        }
        RETURN_IF_FAILED(queue_->QueueEventParamVar(MEStreamStopped, GUID_NULL, S_OK, nullptr));
        state_ = MF_STREAM_STATE_STOPPED;
        return S_OK;
    }

    void shutdown() {
        std::lock_guard<std::recursive_mutex> lock(lock_);
        if (queue_) {
            queue_->Shutdown();
            queue_.reset();
        }
        descriptor_.reset();
        source_.reset();
        allocator_.reset();
        reader_.close();
    }

    STDMETHODIMP BeginGetEvent(IMFAsyncCallback* callback, IUnknown* state) {
        std::lock_guard<std::recursive_mutex> lock(lock_);
        return queue_ ? queue_->BeginGetEvent(callback, state) : MF_E_SHUTDOWN;
    }

    STDMETHODIMP EndGetEvent(IMFAsyncResult* result, IMFMediaEvent** event) {
        std::lock_guard<std::recursive_mutex> lock(lock_);
        return queue_ ? queue_->EndGetEvent(result, event) : MF_E_SHUTDOWN;
    }

    STDMETHODIMP GetEvent(DWORD flags, IMFMediaEvent** event) {
        std::lock_guard<std::recursive_mutex> lock(lock_);
        return queue_ ? queue_->GetEvent(flags, event) : MF_E_SHUTDOWN;
    }

    STDMETHODIMP QueueEvent(
        MediaEventType type,
        REFGUID extendedType,
        HRESULT status,
        const PROPVARIANT* value
    ) {
        std::lock_guard<std::recursive_mutex> lock(lock_);
        return queue_ ? queue_->QueueEventParamVar(type, extendedType, status, value) : MF_E_SHUTDOWN;
    }

    STDMETHODIMP GetMediaSource(IMFMediaSource** result) {
        if (!result) {
            return E_POINTER;
        }
        *result = nullptr;
        return source_.copy_to(result);
    }

    STDMETHODIMP GetStreamDescriptor(IMFStreamDescriptor** result) {
        if (!result) {
            return E_POINTER;
        }
        std::lock_guard<std::recursive_mutex> lock(lock_);
        *result = nullptr;
        return descriptor_.copy_to(result);
    }

    STDMETHODIMP RequestSample(IUnknown* token) {
        std::lock_guard<std::recursive_mutex> lock(lock_);
        if (!queue_ || !allocator_) {
            return MF_E_SHUTDOWN;
        }

        wil::com_ptr_nothrow<IMFSample> sample;
        RETURN_IF_FAILED(allocator_->AllocateSample(sample.put()));
        RETURN_IF_FAILED(sample->SetSampleTime(MFGetSystemTime()));

        FrameInfo info{};
        reader_.info(info);
        const std::uint64_t duration = 10000000ULL * info.fpsDenominator / info.fpsNumerator;
        RETURN_IF_FAILED(sample->SetSampleDuration(static_cast<LONGLONG>(duration)));

        wil::com_ptr_nothrow<IMFMediaBuffer> buffer;
        RETURN_IF_FAILED(sample->GetBufferByIndex(0, buffer.put()));

        wil::com_ptr_nothrow<IMF2DBuffer2> buffer2d;
        if (SUCCEEDED(buffer->QueryInterface(buffer2d.put()))) {
            BYTE* scanline = nullptr;
            BYTE* start = nullptr;
            LONG stride = 0;
            DWORD length = 0;
            RETURN_IF_FAILED(buffer2d->Lock2DSize(
                MF2DBuffer_LockFlags_Write,
                &scanline,
                &stride,
                &start,
                &length
            ));
            reader_.copyTo(scanline, static_cast<std::size_t>(stride), length, info);
            buffer2d->Unlock2D();
        } else {
            BYTE* data = nullptr;
            DWORD maximum = 0;
            DWORD length = 0;
            RETURN_IF_FAILED(buffer->Lock(&data, &maximum, &length));
            reader_.copyTo(data, static_cast<std::size_t>(info.width) * 4, length, info);
            buffer->Unlock();
        }

        if (token) {
            RETURN_IF_FAILED(sample->SetUnknown(MFSampleExtension_Token, token));
        }
        RETURN_IF_FAILED(queue_->QueueEventParamUnk(
            MEMediaSample,
            GUID_NULL,
            S_OK,
            sample.get()
        ));
        return S_OK;
    }

    STDMETHODIMP SetStreamState(MF_STREAM_STATE value) {
        switch (value) {
            case MF_STREAM_STATE_RUNNING:
                return start(nullptr);
            case MF_STREAM_STATE_STOPPED:
                return stop();
            case MF_STREAM_STATE_PAUSED:
                return S_OK;
            default:
                return MF_E_INVALID_STATE_TRANSITION;
        }
    }

    STDMETHODIMP GetStreamState(MF_STREAM_STATE* value) {
        if (!value) {
            return E_POINTER;
        }
        *value = state_;
        return S_OK;
    }

    STDMETHODIMP_(NTSTATUS) KsProperty(
        PKSPROPERTY,
        ULONG,
        LPVOID,
        ULONG,
        ULONG* bytesReturned
    ) {
        if (!bytesReturned) {
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }
        *bytesReturned = 0;
        return HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
    }

    STDMETHODIMP_(NTSTATUS) KsMethod(
        PKSMETHOD,
        ULONG,
        LPVOID,
        ULONG,
        ULONG* bytesReturned
    ) {
        if (!bytesReturned) {
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }
        *bytesReturned = 0;
        return HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
    }

    STDMETHODIMP_(NTSTATUS) KsEvent(
        PKSEVENT,
        ULONG,
        LPVOID,
        ULONG,
        ULONG* bytesReturned
    ) {
        if (!bytesReturned) {
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }
        *bytesReturned = 0;
        return HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
    }

private:
    std::recursive_mutex lock_;
    wil::com_ptr_nothrow<MediaStream> self_;
    wil::com_ptr_nothrow<IMFMediaSource> source_;
    wil::com_ptr_nothrow<IMFStreamDescriptor> descriptor_;
    wil::com_ptr_nothrow<IMFMediaEventQueue> queue_;
    wil::com_ptr_nothrow<IMFVideoSampleAllocatorEx> allocator_;
    SharedFrameReader reader_;
    MF_STREAM_STATE state_ = MF_STREAM_STATE_STOPPED;
    GUID format_ = MFVideoFormat_RGB32;
    std::uint32_t index_ = 0;
};

class MediaSource final :
    public ComObject,
    public IMFMediaSourceEx,
    public IMFGetService,
    public IKsControl,
    public IMFSampleAllocatorControl,
    public AttributeSink {
public:
    MediaSource() {
        initializeAttributes();
        stream_.attach(new MediaStream());
    }

    STDMETHODIMP QueryInterface(REFIID riid, void** result) {
        if (!result) {
            return E_POINTER;
        }
        if (IsEqualIID(riid, __uuidof(IUnknown))) {
            *result = static_cast<IUnknown*>(static_cast<IMFMediaSourceEx*>(this));
            AddRef();
            return S_OK;
        }
        if (IsEqualIID(riid, __uuidof(IMFMediaSource))) {
            *result = static_cast<IMFMediaSource*>(this);
            AddRef();
            return S_OK;
        }
        if (IsEqualIID(riid, __uuidof(IMFMediaSource2))) {
            *result = static_cast<IMFMediaSource2*>(this);
            AddRef();
            return S_OK;
        }
        if (IsEqualIID(riid, __uuidof(IMFMediaSourceEx))) {
            *result = static_cast<IMFMediaSourceEx*>(this);
            AddRef();
            return S_OK;
        }
        if (IsEqualIID(riid, __uuidof(IMFMediaEventGenerator))) {
            *result = static_cast<IMFMediaEventGenerator*>(this);
            AddRef();
            return S_OK;
        }
        if (IsEqualIID(riid, __uuidof(IMFAttributes))) {
            *result = static_cast<IMFAttributes*>(static_cast<IMFMediaSourceEx*>(this));
            AddRef();
            return S_OK;
        }
        if (IsEqualIID(riid, __uuidof(IMFGetService))) {
            *result = static_cast<IMFGetService*>(this);
            AddRef();
            return S_OK;
        }
        if (IsEqualIID(riid, __uuidof(IKsControl))) {
            *result = static_cast<IKsControl*>(this);
            AddRef();
            return S_OK;
        }
        if (IsEqualIID(riid, __uuidof(IMFSampleAllocatorControl))) {
            *result = static_cast<IMFSampleAllocatorControl*>(this);
            AddRef();
            return S_OK;
        }
        *result = nullptr;
        return E_NOINTERFACE;
    }

    HRESULT initialize(IMFAttributes* sourceAttributes) {
        if (sourceAttributes) {
            RETURN_IF_FAILED(sourceAttributes->CopyAllItems(attributes_.get()));
        }

        wil::com_ptr_nothrow<IMFSensorProfileCollection> profiles;
        RETURN_IF_FAILED(MFCreateSensorProfileCollection(profiles.put()));

        wil::com_ptr_nothrow<IMFSensorProfile> legacy;
        RETURN_IF_FAILED(MFCreateSensorProfile(
            KSCAMERAPROFILE_Legacy,
            0,
            nullptr,
            legacy.put()
        ));
        RETURN_IF_FAILED(legacy->AddProfileFilter(0, L"((RES==;FRT<=30,1;SUT==))"));
        RETURN_IF_FAILED(profiles->AddProfile(legacy.get()));

        wil::com_ptr_nothrow<IMFSensorProfile> highRate;
        RETURN_IF_FAILED(MFCreateSensorProfile(
            KSCAMERAPROFILE_HighFrameRate,
            0,
            nullptr,
            highRate.put()
        ));
        RETURN_IF_FAILED(highRate->AddProfileFilter(0, L"((RES==;FRT>=60,1;SUT==))"));
        RETURN_IF_FAILED(profiles->AddProfile(highRate.get()));
        RETURN_IF_FAILED(SetUnknown(MF_DEVICEMFT_SENSORPROFILE_COLLECTION, profiles.get()));

        RETURN_IF_FAILED(stream_->initialize(this, 0));

        wil::com_ptr_nothrow<IMFStreamDescriptor> descriptor;
        RETURN_IF_FAILED(stream_->GetStreamDescriptor(descriptor.put()));
        IMFStreamDescriptor* descriptors[] = {descriptor.get()};
        RETURN_IF_FAILED(MFCreatePresentationDescriptor(1, descriptors, descriptor_.put()));
        RETURN_IF_FAILED(MFCreateEventQueue(queue_.put()));
        return S_OK;
    }

    STDMETHODIMP BeginGetEvent(IMFAsyncCallback* callback, IUnknown* state) {
        std::lock_guard<std::recursive_mutex> lock(lock_);
        return queue_ ? queue_->BeginGetEvent(callback, state) : MF_E_SHUTDOWN;
    }

    STDMETHODIMP EndGetEvent(IMFAsyncResult* result, IMFMediaEvent** event) {
        std::lock_guard<std::recursive_mutex> lock(lock_);
        return queue_ ? queue_->EndGetEvent(result, event) : MF_E_SHUTDOWN;
    }

    STDMETHODIMP GetEvent(DWORD flags, IMFMediaEvent** event) {
        std::lock_guard<std::recursive_mutex> lock(lock_);
        return queue_ ? queue_->GetEvent(flags, event) : MF_E_SHUTDOWN;
    }

    STDMETHODIMP QueueEvent(
        MediaEventType type,
        REFGUID extendedType,
        HRESULT status,
        const PROPVARIANT* value
    ) {
        std::lock_guard<std::recursive_mutex> lock(lock_);
        return queue_ ? queue_->QueueEventParamVar(type, extendedType, status, value) : MF_E_SHUTDOWN;
    }

    STDMETHODIMP CreatePresentationDescriptor(IMFPresentationDescriptor** result) {
        if (!result) {
            return E_POINTER;
        }
        std::lock_guard<std::recursive_mutex> lock(lock_);
        return descriptor_ ? descriptor_->Clone(result) : MF_E_SHUTDOWN;
    }

    STDMETHODIMP GetCharacteristics(DWORD* result) {
        if (!result) {
            return E_POINTER;
        }
        *result = MFMEDIASOURCE_IS_LIVE;
        return S_OK;
    }

    STDMETHODIMP Pause() {
        return MF_E_INVALID_STATE_TRANSITION;
    }

    STDMETHODIMP Shutdown() {
        std::lock_guard<std::recursive_mutex> lock(lock_);
        if (queue_) {
            queue_->Shutdown();
            queue_.reset();
        }
        if (stream_) {
            stream_->shutdown();
        }
        descriptor_.reset();
        return S_OK;
    }

    STDMETHODIMP Start(
        IMFPresentationDescriptor* descriptor,
        const GUID* timeFormat,
        const PROPVARIANT* startPosition
    ) {
        if (!descriptor || !startPosition) {
            return E_POINTER;
        }
        if (timeFormat && *timeFormat != GUID_NULL) {
            return E_INVALIDARG;
        }

        std::lock_guard<std::recursive_mutex> lock(lock_);
        if (!queue_ || !descriptor_) {
            return MF_E_SHUTDOWN;
        }

        DWORD count = 0;
        RETURN_IF_FAILED(descriptor->GetStreamDescriptorCount(&count));
        if (count != 1) {
            return E_INVALIDARG;
        }

        wil::com_ptr_nothrow<IMFStreamDescriptor> streamDescriptor;
        BOOL selected = FALSE;
        RETURN_IF_FAILED(descriptor->GetStreamDescriptorByIndex(0, &selected, streamDescriptor.put()));

        wil::com_ptr_nothrow<IMFMediaTypeHandler> handler;
        wil::com_ptr_nothrow<IMFMediaType> type;
        if (selected) {
            RETURN_IF_FAILED(streamDescriptor->GetMediaTypeHandler(handler.put()));
            RETURN_IF_FAILED(handler->GetCurrentMediaType(type.put()));
        }

        MF_STREAM_STATE state = MF_STREAM_STATE_STOPPED;
        RETURN_IF_FAILED(stream_->GetStreamState(&state));
        if (selected && state != MF_STREAM_STATE_RUNNING) {
            if (!announced_) {
                wil::com_ptr_nothrow<IUnknown> unknown;
                RETURN_IF_FAILED(stream_.copy_to(&unknown));
                RETURN_IF_FAILED(queue_->QueueEventParamUnk(
                    MENewStream,
                    GUID_NULL,
                    S_OK,
                    unknown.get()
                ));
                announced_ = true;
            }
            RETURN_IF_FAILED(descriptor_->SelectStream(0));
            RETURN_IF_FAILED(stream_->start(type.get()));
        } else if (!selected && state != MF_STREAM_STATE_STOPPED) {
            RETURN_IF_FAILED(descriptor_->DeselectStream(0));
            RETURN_IF_FAILED(stream_->stop());
        }

        wil::unique_prop_variant time;
        RETURN_IF_FAILED(InitPropVariantFromInt64(MFGetSystemTime(), &time));
        RETURN_IF_FAILED(queue_->QueueEventParamVar(
            MESourceStarted,
            GUID_NULL,
            S_OK,
            &time
        ));
        return S_OK;
    }

    STDMETHODIMP Stop() {
        std::lock_guard<std::recursive_mutex> lock(lock_);
        if (!queue_ || !descriptor_) {
            return MF_E_SHUTDOWN;
        }
        RETURN_IF_FAILED(stream_->stop());
        RETURN_IF_FAILED(descriptor_->DeselectStream(0));

        wil::unique_prop_variant time;
        RETURN_IF_FAILED(InitPropVariantFromInt64(MFGetSystemTime(), &time));
        RETURN_IF_FAILED(queue_->QueueEventParamVar(
            MESourceStopped,
            GUID_NULL,
            S_OK,
            &time
        ));
        return S_OK;
    }

    STDMETHODIMP GetSourceAttributes(IMFAttributes** result) {
        return handoff(
            static_cast<IMFAttributes*>(static_cast<IMFMediaSourceEx*>(this)),
            __uuidof(IMFAttributes),
            result
        );
    }

    STDMETHODIMP GetStreamAttributes(DWORD streamId, IMFAttributes** result) {
        if (!result) {
            return E_POINTER;
        }
        if (streamId != 0) {
            return E_INVALIDARG;
        }
        return stream_->QueryInterface(IID_PPV_ARGS(result));
    }

    STDMETHODIMP SetD3DManager(IUnknown* manager) {
        if (!manager) {
            return E_POINTER;
        }
        return stream_->setD3DManager(manager);
    }

    STDMETHODIMP SetMediaType(DWORD, IMFMediaType*) {
        return S_OK;
    }

    STDMETHODIMP GetService(REFGUID, REFIID, LPVOID*) {
        return MF_E_UNSUPPORTED_SERVICE;
    }

    STDMETHODIMP SetDefaultAllocator(DWORD streamId, IUnknown* allocator) {
        if (streamId != 0) {
            return E_INVALIDARG;
        }
        return stream_->setAllocator(allocator);
    }

    STDMETHODIMP GetAllocatorUsage(
        DWORD streamId,
        DWORD* inputStreamId,
        MFSampleAllocatorUsage* usage
    ) {
        if (!inputStreamId || !usage || streamId != 0) {
            return E_INVALIDARG;
        }
        *inputStreamId = streamId;
        *usage = stream_->allocatorUsage();
        return S_OK;
    }

    STDMETHODIMP_(NTSTATUS) KsProperty(
        PKSPROPERTY,
        ULONG,
        LPVOID,
        ULONG,
        ULONG* bytesReturned
    ) {
        if (!bytesReturned) {
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }
        *bytesReturned = 0;
        return HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
    }

    STDMETHODIMP_(NTSTATUS) KsMethod(
        PKSMETHOD,
        ULONG,
        LPVOID,
        ULONG,
        ULONG* bytesReturned
    ) {
        if (!bytesReturned) {
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }
        *bytesReturned = 0;
        return HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
    }

    STDMETHODIMP_(NTSTATUS) KsEvent(
        PKSEVENT,
        ULONG,
        LPVOID,
        ULONG,
        ULONG* bytesReturned
    ) {
        if (!bytesReturned) {
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }
        *bytesReturned = 0;
        return HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
    }

private:
    std::recursive_mutex lock_;
    wil::com_ptr_nothrow<MediaStream> stream_;
    wil::com_ptr_nothrow<IMFMediaEventQueue> queue_;
    wil::com_ptr_nothrow<IMFPresentationDescriptor> descriptor_;
    bool announced_ = false;
};

class Activator final : public ComObject, public IMFActivate, public AttributeSink {
public:
    Activator() {
        initializeAttributes();
    }

    STDMETHODIMP QueryInterface(REFIID riid, void** result) {
        if (!result) {
            return E_POINTER;
        }
        if (IsEqualIID(riid, __uuidof(IUnknown))) {
            *result = static_cast<IUnknown*>(static_cast<IMFActivate*>(this));
            AddRef();
            return S_OK;
        }
        if (IsEqualIID(riid, __uuidof(IMFActivate))) {
            *result = static_cast<IMFActivate*>(this);
            AddRef();
            return S_OK;
        }
        if (IsEqualIID(riid, __uuidof(IMFAttributes))) {
            *result = static_cast<IMFAttributes*>(static_cast<IMFActivate*>(this));
            AddRef();
            return S_OK;
        }
        *result = nullptr;
        return E_NOINTERFACE;
    }

    HRESULT initialize() {
        RETURN_IF_FAILED(SetUINT32(
            MF_VIRTUALCAMERA_PROVIDE_ASSOCIATED_CAMERA_SOURCES,
            1
        ));
        RETURN_IF_FAILED(SetGUID(MFT_TRANSFORM_CLSID_Attribute, kVirtualCameraClsid));
        source_.attach(new MediaSource());
        return source_->initialize(attributes_.get());
    }

    STDMETHODIMP ActivateObject(REFIID riid, void** result) {
        return source_ ? source_->QueryInterface(riid, result) : E_UNEXPECTED;
    }

    STDMETHODIMP ShutdownObject() {
        return S_OK;
    }

    STDMETHODIMP DetachObject() {
        source_.reset();
        return S_OK;
    }

private:
    wil::com_ptr_nothrow<MediaSource> source_;
};

class ClassFactory final : public ComObject, public IClassFactory {
public:
    STDMETHODIMP QueryInterface(REFIID riid, void** result) {
        if (!result) {
            return E_POINTER;
        }
        if (IsEqualIID(riid, __uuidof(IUnknown)) || IsEqualIID(riid, __uuidof(IClassFactory))) {
            *result = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        *result = nullptr;
        return E_NOINTERFACE;
    }

    STDMETHODIMP CreateInstance(IUnknown* outer, REFIID riid, void** result) {
        if (!result) {
            return E_POINTER;
        }
        *result = nullptr;
        if (outer) {
            return CLASS_E_NOAGGREGATION;
        }

        auto* activator = new Activator();
        const HRESULT hr = activator->initialize();
        if (FAILED(hr)) {
            activator->Release();
            return hr;
        }
        const HRESULT queryResult = activator->QueryInterface(riid, result);
        activator->Release();
        return queryResult;
    }

    STDMETHODIMP LockServer(BOOL lock) {
        if (lock) {
            AddRef();
        } else {
            Release();
        }
        return S_OK;
    }
};

HRESULT registerServer() {
    wchar_t modulePath[MAX_PATH]{};
    if (!GetModuleFileNameW(g_module, modulePath, MAX_PATH)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    wchar_t clsid[64]{};
    StringFromGUID2(kVirtualCameraClsid, clsid, static_cast<int>(std::size(clsid)));
    const std::wstring key = L"Software\\Classes\\CLSID\\" + std::wstring(clsid) +
        L"\\InprocServer32";

    HKEY handle = nullptr;
    LSTATUS status = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE,
        key.c_str(),
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        nullptr,
        &handle,
        nullptr
    );
    if (status != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(status);
    }

    status = RegSetValueExW(
        handle,
        nullptr,
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(modulePath),
        static_cast<DWORD>((std::wcslen(modulePath) + 1) * sizeof(wchar_t))
    );
    if (status == ERROR_SUCCESS) {
        const wchar_t threadingModel[] = L"Both";
        status = RegSetValueExW(
            handle,
            L"ThreadingModel",
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(threadingModel),
            sizeof(threadingModel)
        );
    }
    RegCloseKey(handle);
    return HRESULT_FROM_WIN32(status);
}

}

extern "C" BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = module;
        DisableThreadLibraryCalls(module);
    }
    return TRUE;
}

extern "C" HRESULT STDAPICALLTYPE DllGetClassObject(
    REFCLSID classId,
    REFIID interfaceId,
    LPVOID* result
) {
    if (!result) {
        return E_POINTER;
    }
    *result = nullptr;
    if (!IsEqualIID(classId, kVirtualCameraClsid)) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    auto* factory = new ClassFactory();
    const HRESULT hr = factory->QueryInterface(interfaceId, result);
    factory->Release();
    return hr;
}

extern "C" HRESULT STDAPICALLTYPE DllCanUnloadNow() {
    return g_moduleReferences.load() == 0 ? S_OK : S_FALSE;
}

extern "C" HRESULT STDAPICALLTYPE DllRegisterServer() {
    return registerServer();
}

extern "C" HRESULT STDAPICALLTYPE DllUnregisterServer() {
    wchar_t clsid[64]{};
    StringFromGUID2(kVirtualCameraClsid, clsid, static_cast<int>(std::size(clsid)));
    const std::wstring key = L"Software\\Classes\\CLSID\\" + std::wstring(clsid);
    return HRESULT_FROM_WIN32(RegDeleteTreeW(HKEY_LOCAL_MACHINE, key.c_str()));
}
