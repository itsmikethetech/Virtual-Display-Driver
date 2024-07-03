/*++

Copyright (c) Microsoft Corporation

Abstract:

    This module contains a sample implementation of an indirect display driver. See the included README.md file and the
    various TODO blocks throughout this file and all accompanying files for information on building a production driver.

    MSDN documentation on indirect displays can be found at https://msdn.microsoft.com/en-us/library/windows/hardware/mt761968(v=vs.85).aspx.

Environment:

    User Mode, UMDF

--*/

#include "Driver.h"
//#include "Driver.tmh"
#include<fstream>
#include<sstream>
#include<string>
#include<tuple>
#include<vector>
#include <AdapterOption.h>
#include <xmllite.h>
#include <shlwapi.h>
#include <atlbase.h>
#include <iostream>

#pragma comment(lib, "xmllite.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace std;
using namespace Microsoft::IndirectDisp;
using namespace Microsoft::WRL;

extern "C" DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD IddSampleDeviceAdd;
EVT_WDF_DEVICE_D0_ENTRY IddSampleDeviceD0Entry;

EVT_IDD_CX_ADAPTER_INIT_FINISHED IddSampleAdapterInitFinished;
EVT_IDD_CX_ADAPTER_COMMIT_MODES IddSampleAdapterCommitModes;

EVT_IDD_CX_PARSE_MONITOR_DESCRIPTION IddSampleParseMonitorDescription;
EVT_IDD_CX_MONITOR_GET_DEFAULT_DESCRIPTION_MODES IddSampleMonitorGetDefaultModes;
EVT_IDD_CX_MONITOR_QUERY_TARGET_MODES IddSampleMonitorQueryModes;

EVT_IDD_CX_MONITOR_ASSIGN_SWAPCHAIN IddSampleMonitorAssignSwapChain;
EVT_IDD_CX_MONITOR_UNASSIGN_SWAPCHAIN IddSampleMonitorUnassignSwapChain;

struct
{
    AdapterOption Adapter;
} Options;
vector<tuple<int, int, int>> monitorModes;
vector< DISPLAYCONFIG_VIDEO_SIGNAL_INFO> s_KnownMonitorModes2;
UINT numVirtualDisplays;
wstring gpuname;

struct IndirectDeviceContextWrapper
{
    IndirectDeviceContext* pContext;

    void Cleanup()
    {
        delete pContext;
        pContext = nullptr;
    }
};

// This macro creates the methods for accessing an IndirectDeviceContextWrapper as a context for a WDF object
WDF_DECLARE_CONTEXT_TYPE(IndirectDeviceContextWrapper);

extern "C" BOOL WINAPI DllMain(
    _In_ HINSTANCE hInstance,
    _In_ UINT dwReason,
    _In_opt_ LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(lpReserved);
    UNREFERENCED_PARAMETER(dwReason);

    return TRUE;
}

_Use_decl_annotations_
extern "C" NTSTATUS DriverEntry(
    PDRIVER_OBJECT  pDriverObject,
    PUNICODE_STRING pRegistryPath
)
{
    WDF_DRIVER_CONFIG Config;
    NTSTATUS Status;

    WDF_OBJECT_ATTRIBUTES Attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);

    WDF_DRIVER_CONFIG_INIT(&Config,
        IddSampleDeviceAdd
    );

    Status = WdfDriverCreate(pDriverObject, pRegistryPath, &Attributes, &Config, WDF_NO_HANDLE);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    return Status;
}
vector<string> split(string& input, char delimiter)
{
    istringstream stream(input);
    string field;
    vector<string> result;
    while (getline(stream, field, delimiter)) {
        result.push_back(field);
    }
    return result;
}

void loadSettings() {
    // Check if the file exists
    const std::wstring& filename = L"C:\\IddSampleDriver\\vdd_settings.xml";
    if (PathFileExistsW(filename.c_str())) {
        CComPtr<IStream> pStream;
        CComPtr<IXmlReader> pReader;
        HRESULT hr = SHCreateStreamOnFileW(filename.c_str(), STGM_READ, &pStream);
        // Indented just because it's over eager fault tollerance
        if (FAILED(hr)) {
            std::wcout << L"Failed to create file stream. HRESULT: " << hr << std::endl;
            return;
        }
        hr = CreateXmlReader(__uuidof(IXmlReader), (void**)&pReader, NULL);
        if (FAILED(hr)) {
            std::wcout << L"Failed to create XmlReader. HRESULT: " << hr << std::endl;
            return;
        }
        hr = pReader->SetInput(pStream);
        if (FAILED(hr)) {
            std::wcout << L"Failed to set input stream. HRESULT: " << hr << std::endl;
            return;
        }

        XmlNodeType nodeType;
        const WCHAR* pwszLocalName;
        const WCHAR* pwszValue;
        UINT cwchLocalName;
        UINT cwchValue;
        std::wstring currentElement;
        std::wstring width, height, refreshRate;
        std::vector<std::tuple<std::int_fast16_t, std::int_fast16_t, std::int_fast16_t>> res;
        std::wstring gpuFriendlyName;
        UINT monitorcount = 1;

        while (S_OK == (hr = pReader->Read(&nodeType))) {
            switch (nodeType) {
            case XmlNodeType_Element:
                hr = pReader->GetLocalName(&pwszLocalName, &cwchLocalName);
                if (FAILED(hr)) {
                    std::wcout << L"Failed to get local name. HRESULT: " << hr << std::endl;
                    return;
                }
                currentElement = std::wstring(pwszLocalName, cwchLocalName);
                break;
            case XmlNodeType_Text:
                hr = pReader->GetValue(&pwszValue, &cwchValue);
                if (FAILED(hr)) {
                    std::wcout << L"Failed to get value. HRESULT: " << hr << std::endl;
                    return;
                }
                if (currentElement == L"count") {
                    monitorcount = std::stoi(std::wstring(pwszValue, cwchValue));
                }
                else if (currentElement == L"friendlyname") {
                    gpuFriendlyName = std::wstring(pwszValue, cwchValue);
                }
                else if (currentElement == L"width") {
                    width = std::wstring(pwszValue, cwchValue);
                }
                else if (currentElement == L"height") {
                    height = std::wstring(pwszValue, cwchValue);
                }
                else if (currentElement == L"refresh_rate") {
                    refreshRate = std::wstring(pwszValue, cwchValue);
                    res.push_back(std::make_tuple(stoi(width), stoi(height), stoi(refreshRate)));
                }
                break;
            }
        }
        numVirtualDisplays = monitorcount;
        gpuname = gpuFriendlyName;
        monitorModes = res;
        return;
    }
    ifstream ifs("C:\\IddSampleDriver\\option.txt");
    if (ifs.is_open()) {
        string line;
        vector<tuple<int, int, int>> res;
        getline(ifs, line);//num of displays
        numVirtualDisplays = stoi(line);
        while (getline(ifs, line)) {
            vector<string> strvec = split(line, ',');
            if (strvec.size() == 3 && strvec[0].substr(0, 1) != "#") {
                res.push_back({ stoi(strvec[0]),stoi(strvec[1]),stoi(strvec[2]) });
            }
        }
        monitorModes = res; return;
    }
    else {
        numVirtualDisplays = 1;
        vector<tuple<int, int, int>> res = {
            make_tuple(3840, 2160, 30),
            make_tuple(3840, 2160, 60),
            make_tuple(3840, 2160, 90),
            make_tuple(3840, 2160, 120),
            make_tuple(3840, 2160, 144),
            make_tuple(2560, 1440, 30),
            make_tuple(2560, 1440, 60),
            make_tuple(2560, 1440, 90),
            make_tuple(2560, 1440, 120),
            make_tuple(2560, 1440, 144),
            make_tuple(1920, 1080, 30),
            make_tuple(1920, 1080, 60),
            make_tuple(1920, 1080, 90),
            make_tuple(1920, 1080, 120),
            make_tuple(1920, 1080, 144),
            make_tuple(1366, 768, 30),
            make_tuple(1366, 768, 60),
            make_tuple(1366, 768, 90),
            make_tuple(1366, 768, 120),
            make_tuple(1366, 768, 144),
            make_tuple(1280, 720, 30),
            make_tuple(1280, 720, 60),
            make_tuple(1280, 720, 90),
            make_tuple(1280, 720, 130),
            make_tuple(1280, 720, 144),
            make_tuple(800, 600, 30),
            make_tuple(800, 600, 60),
            make_tuple(800, 600, 90),
            make_tuple(800, 600, 120),
            make_tuple(800, 600, 144)
        };
        monitorModes = res; return;
    }
}

_Use_decl_annotations_
NTSTATUS IddSampleDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT pDeviceInit)
{
    NTSTATUS Status = STATUS_SUCCESS;
    WDF_PNPPOWER_EVENT_CALLBACKS PnpPowerCallbacks;

    UNREFERENCED_PARAMETER(Driver);

    // Register for power callbacks - in this sample only power-on is needed
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpPowerCallbacks);
    PnpPowerCallbacks.EvtDeviceD0Entry = IddSampleDeviceD0Entry;
    WdfDeviceInitSetPnpPowerEventCallbacks(pDeviceInit, &PnpPowerCallbacks);

    IDD_CX_CLIENT_CONFIG IddConfig;
    IDD_CX_CLIENT_CONFIG_INIT(&IddConfig);

    // If the driver wishes to handle custom IoDeviceControl requests, it's necessary to use this callback since IddCx
    // redirects IoDeviceControl requests to an internal queue. This sample does not need this.
    // IddConfig.EvtIddCxDeviceIoControl = IddSampleIoDeviceControl;

    loadSettings();
    if (gpuname.empty()) {
        Options.Adapter.load("C:\\IddSampleDriver\\adapter.txt");
    }
    else {
        Options.Adapter.xmlprovide(gpuname);
    }
    IddConfig.EvtIddCxAdapterInitFinished = IddSampleAdapterInitFinished;

    IddConfig.EvtIddCxParseMonitorDescription = IddSampleParseMonitorDescription;
    IddConfig.EvtIddCxMonitorGetDefaultDescriptionModes = IddSampleMonitorGetDefaultModes;
    IddConfig.EvtIddCxMonitorQueryTargetModes = IddSampleMonitorQueryModes;
    IddConfig.EvtIddCxAdapterCommitModes = IddSampleAdapterCommitModes;
    IddConfig.EvtIddCxMonitorAssignSwapChain = IddSampleMonitorAssignSwapChain;
    IddConfig.EvtIddCxMonitorUnassignSwapChain = IddSampleMonitorUnassignSwapChain;

    Status = IddCxDeviceInitConfig(pDeviceInit, &IddConfig);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    WDF_OBJECT_ATTRIBUTES Attr;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attr, IndirectDeviceContextWrapper);
    Attr.EvtCleanupCallback = [](WDFOBJECT Object)
    {
        // Automatically cleanup the context when the WDF object is about to be deleted
        auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(Object);
        if (pContext)
        {
            pContext->Cleanup();
        }
    };

    WDFDEVICE Device = nullptr;
    Status = WdfDeviceCreate(&pDeviceInit, &Attr, &Device);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = IddCxDeviceInitialize(Device);

    // Create a new device context object and attach it to the WDF device object
    auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(Device);
    pContext->pContext = new IndirectDeviceContext(Device);

    return Status;
}

_Use_decl_annotations_
NTSTATUS IddSampleDeviceD0Entry(WDFDEVICE Device, WDF_POWER_DEVICE_STATE PreviousState)
{
    UNREFERENCED_PARAMETER(PreviousState);

    // This function is called by WDF to start the device in the fully-on power state.

    auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(Device);
    pContext->pContext->InitAdapter();

    return STATUS_SUCCESS;
}

#pragma region Direct3DDevice

Direct3DDevice::Direct3DDevice(LUID AdapterLuid) : AdapterLuid(AdapterLuid)
{

}

Direct3DDevice::Direct3DDevice()
{

}

HRESULT Direct3DDevice::Init()
{
    // The DXGI factory could be cached, but if a new render adapter appears on the system, a new factory needs to be
    // created. If caching is desired, check DxgiFactory->IsCurrent() each time and recreate the factory if !IsCurrent.
    HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&DxgiFactory));
    if (FAILED(hr))
    {
        return hr;
    }

    // Find the specified render adapter
    hr = DxgiFactory->EnumAdapterByLuid(AdapterLuid, IID_PPV_ARGS(&Adapter));
    if (FAILED(hr))
    {
        return hr;
    }

#if 0 // Test code
    {
        FILE* file;
        fopen_s(&file, "C:\\IddSampleDriver\\desc.bin", "wb");

        DXGI_ADAPTER_DESC desc;
        Adapter->GetDesc(&desc);

        fwrite(&desc, 1, sizeof(desc), file);
        fclose(file);
    }
#endif

    // Create a D3D device using the render adapter. BGRA support is required by the WHQL test suite.
    hr = D3D11CreateDevice(Adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, &Device, nullptr, &DeviceContext);
    if (FAILED(hr))
    {
        // If creating the D3D device failed, it's possible the render GPU was lost (e.g. detachable GPU) or else the
        // system is in a transient state.
        return hr;
    }

    return S_OK;
}

#pragma endregion

#pragma region SwapChainProcessor

SwapChainProcessor::SwapChainProcessor(IDDCX_SWAPCHAIN hSwapChain, shared_ptr<Direct3DDevice> Device, HANDLE NewFrameEvent)
    : m_hSwapChain(hSwapChain), m_Device(Device), m_hAvailableBufferEvent(NewFrameEvent)
{
    m_hTerminateEvent.Attach(CreateEvent(nullptr, FALSE, FALSE, nullptr));

    // Immediately create and run the swap-chain processing thread, passing 'this' as the thread parameter
    m_hThread.Attach(CreateThread(nullptr, 0, RunThread, this, 0, nullptr));
}

SwapChainProcessor::~SwapChainProcessor()
{
    // Alert the swap-chain processing thread to terminate
    SetEvent(m_hTerminateEvent.Get());

    if (m_hThread.Get())
    {
        // Wait for the thread to terminate
        WaitForSingleObject(m_hThread.Get(), INFINITE);
    }
}

DWORD CALLBACK SwapChainProcessor::RunThread(LPVOID Argument)
{
    reinterpret_cast<SwapChainProcessor*>(Argument)->Run();
    return 0;
}

void SwapChainProcessor::Run()
{
    // For improved performance, make use of the Multimedia Class Scheduler Service, which will intelligently
    // prioritize this thread for improved throughput in high CPU-load scenarios.
    DWORD AvTask = 0;
    HANDLE AvTaskHandle = AvSetMmThreadCharacteristicsW(L"Distribution", &AvTask);

    RunCore();

    // Always delete the swap-chain object when swap-chain processing loop terminates in order to kick the system to
    // provide a new swap-chain if necessary.
    WdfObjectDelete((WDFOBJECT)m_hSwapChain);
    m_hSwapChain = nullptr;

    AvRevertMmThreadCharacteristics(AvTaskHandle);
}

void SwapChainProcessor::RunCore()
{
    // Get the DXGI device interface
    ComPtr<IDXGIDevice> DxgiDevice;
    HRESULT hr = m_Device->Device.As(&DxgiDevice);
    if (FAILED(hr))
    {
        return;
    }

    IDARG_IN_SWAPCHAINSETDEVICE SetDevice = {};
    SetDevice.pDevice = DxgiDevice.Get();

    hr = IddCxSwapChainSetDevice(m_hSwapChain, &SetDevice);
    if (FAILED(hr))
    {
        return;
    }

    // Acquire and release buffers in a loop
    for (;;)
    {
        ComPtr<IDXGIResource> AcquiredBuffer;

        // Ask for the next buffer from the producer
        IDARG_OUT_RELEASEANDACQUIREBUFFER Buffer = {};
        hr = IddCxSwapChainReleaseAndAcquireBuffer(m_hSwapChain, &Buffer);

        // AcquireBuffer immediately returns STATUS_PENDING if no buffer is yet available
        if (hr == E_PENDING)
        {
            // We must wait for a new buffer
            HANDLE WaitHandles[] =
            {
                m_hAvailableBufferEvent,
                m_hTerminateEvent.Get()
            };
            DWORD WaitResult = WaitForMultipleObjects(ARRAYSIZE(WaitHandles), WaitHandles, FALSE, 16);
            if (WaitResult == WAIT_OBJECT_0 || WaitResult == WAIT_TIMEOUT)
            {
                // We have a new buffer, so try the AcquireBuffer again
                continue;
            }
            else if (WaitResult == WAIT_OBJECT_0 + 1)
            {
                // We need to terminate
                break;
            }
            else
            {
                // The wait was cancelled or something unexpected happened
                hr = HRESULT_FROM_WIN32(WaitResult);
                break;
            }
        }
        else if (SUCCEEDED(hr))
        {
            AcquiredBuffer.Attach(Buffer.MetaData.pSurface);

            // ==============================
            // TODO: Process the frame here
            //
            // This is the most performance-critical section of code in an IddCx driver. It's important that whatever
            // is done with the acquired surface be finished as quickly as possible. This operation could be:
            //  * a GPU copy to another buffer surface for later processing (such as a staging surface for mapping to CPU memory)
            //  * a GPU encode operation
            //  * a GPU VPBlt to another surface
            //  * a GPU custom compute shader encode operation
            // ==============================

            AcquiredBuffer.Reset();
            hr = IddCxSwapChainFinishedProcessingFrame(m_hSwapChain);
            if (FAILED(hr))
            {
                break;
            }

            // ==============================
            // TODO: Report frame statistics once the asynchronous encode/send work is completed
            //
            // Drivers should report information about sub-frame timings, like encode time, send time, etc.
            // ==============================
            // IddCxSwapChainReportFrameStatistics(m_hSwapChain, ...);
        }
        else
        {
            // The swap-chain was likely abandoned (e.g. DXGI_ERROR_ACCESS_LOST), so exit the processing loop
            break;
        }
    }
}

#pragma endregion

#pragma region IndirectDeviceContext

const UINT64 MHZ = 1000000;
const UINT64 KHZ = 1000;

constexpr DISPLAYCONFIG_VIDEO_SIGNAL_INFO dispinfo(UINT32 h, UINT32 v, UINT32 r) {
    const UINT32 clock_rate = r * (v + 4) * (v + 4) + 1000;
    return {
      clock_rate,                                      // pixel clock rate [Hz]
    { clock_rate, v + 4 },                         // fractional horizontal refresh rate [Hz]
    { clock_rate, (v + 4) * (v + 4) },          // fractional vertical refresh rate [Hz]
    { h, v },                                    // (horizontal, vertical) active pixel resolution
    { h + 4, v + 4 },                         // (horizontal, vertical) total pixel resolution
    { { 255, 0 }},                                   // video standard and vsync divider
    DISPLAYCONFIG_SCANLINE_ORDERING_PROGRESSIVE
    };
}

// This is a sample monitor EDID - FOR SAMPLE PURPOSES ONLY
const BYTE IndirectDeviceContext::s_KnownMonitorEdid[] =
{
0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x36, 0x94, 0x37, 0x13, 0x31, 0x73, 0x37, 0x01,
0x19, 0x22, 0x01, 0x03, 0x80, 0x32, 0x1f, 0x78, 0x07, 0xee, 0x95, 0xa3, 0x54, 0x4c, 0x99, 0x26,
0x0f, 0x50, 0x54, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x08, 0xe8, 0x00, 0x30, 0xf2, 0x70, 0x5a, 0x80, 0xb0, 0x58,
0x8a, 0x00, 0x80, 0x88, 0x42, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x17, 0xf0, 0x0f,
0xff, 0x37, 0x00, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x10, 0x00, 0x56,
0x44, 0x44, 0x62, 0x79, 0x4d, 0x54, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x2e
};

IndirectDeviceContext::IndirectDeviceContext(_In_ WDFDEVICE WdfDevice) :
    m_WdfDevice(WdfDevice)
{
}

IndirectDeviceContext::~IndirectDeviceContext()
{
    m_ProcessingThread.reset();
}

#define NUM_VIRTUAL_DISPLAYS 1

void IndirectDeviceContext::InitAdapter()
{
    // ==============================
    // TODO: Update the below diagnostic information in accordance with the target hardware. The strings and version
    // numbers are used for telemetry and may be displayed to the user in some situations.
    //
    // This is also where static per-adapter capabilities are determined.
    // ==============================

    IDDCX_ADAPTER_CAPS AdapterCaps = {};
    AdapterCaps.Size = sizeof(AdapterCaps);

    // Declare basic feature support for the adapter (required)
    AdapterCaps.MaxMonitorsSupported = numVirtualDisplays;
    AdapterCaps.EndPointDiagnostics.Size = sizeof(AdapterCaps.EndPointDiagnostics);
    AdapterCaps.EndPointDiagnostics.GammaSupport = IDDCX_FEATURE_IMPLEMENTATION_NONE;
    AdapterCaps.EndPointDiagnostics.TransmissionType = IDDCX_TRANSMISSION_TYPE_WIRED_OTHER;

    // Declare your device strings for telemetry (required)
    AdapterCaps.EndPointDiagnostics.pEndPointFriendlyName = L"IddSample Device";
    AdapterCaps.EndPointDiagnostics.pEndPointManufacturerName = L"Microsoft";
    AdapterCaps.EndPointDiagnostics.pEndPointModelName = L"IddSample Model";

    // Declare your hardware and firmware versions (required)
    IDDCX_ENDPOINT_VERSION Version = {};
    Version.Size = sizeof(Version);
    Version.MajorVer = 1;
    AdapterCaps.EndPointDiagnostics.pFirmwareVersion = &Version;
    AdapterCaps.EndPointDiagnostics.pHardwareVersion = &Version;

    // Initialize a WDF context that can store a pointer to the device context object
    WDF_OBJECT_ATTRIBUTES Attr;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attr, IndirectDeviceContextWrapper);

    IDARG_IN_ADAPTER_INIT AdapterInit = {};
    AdapterInit.WdfDevice = m_WdfDevice;
    AdapterInit.pCaps = &AdapterCaps;
    AdapterInit.ObjectAttributes = &Attr;

    // Start the initialization of the adapter, which will trigger the AdapterFinishInit callback later
    IDARG_OUT_ADAPTER_INIT AdapterInitOut;
    NTSTATUS Status = IddCxAdapterInitAsync(&AdapterInit, &AdapterInitOut);

    if (NT_SUCCESS(Status))
    {
        // Store a reference to the WDF adapter handle
        m_Adapter = AdapterInitOut.AdapterObject;

        // Store the device context object into the WDF object context
        auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(AdapterInitOut.AdapterObject);
        pContext->pContext = this;
    }
}

void IndirectDeviceContext::FinishInit()
{
    Options.Adapter.apply(m_Adapter);
    for (unsigned int i = 0; i < numVirtualDisplays; i++) {
        CreateMonitor(i);
    }
}

void IndirectDeviceContext::CreateMonitor(unsigned int index) {
    // ==============================
    // TODO: In a real driver, the EDID should be retrieved dynamically from a connected physical monitor. The EDID
    // provided here is purely for demonstration, as it describes only 640x480 @ 60 Hz and 800x600 @ 60 Hz. Monitor
    // manufacturers are required to correctly fill in physical monitor attributes in order to allow the OS to optimize
    // settings like viewing distance and scale factor. Manufacturers should also use a unique serial number every
    // single device to ensure the OS can tell the monitors apart.
    // ==============================

    WDF_OBJECT_ATTRIBUTES Attr;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attr, IndirectDeviceContextWrapper);

    IDDCX_MONITOR_INFO MonitorInfo = {};
    MonitorInfo.Size = sizeof(MonitorInfo);
    MonitorInfo.MonitorType = DISPLAYCONFIG_OUTPUT_TECHNOLOGY_HDMI;
    MonitorInfo.ConnectorIndex = index;
    MonitorInfo.MonitorDescription.Size = sizeof(MonitorInfo.MonitorDescription);
    MonitorInfo.MonitorDescription.Type = IDDCX_MONITOR_DESCRIPTION_TYPE_EDID;
    MonitorInfo.MonitorDescription.DataSize = sizeof(s_KnownMonitorEdid);
    MonitorInfo.MonitorDescription.pData = const_cast<BYTE*>(s_KnownMonitorEdid);

    // ==============================
    // TODO: The monitor's container ID should be distinct from "this" device's container ID if the monitor is not
    // permanently attached to the display adapter device object. The container ID is typically made unique for each
    // monitor and can be used to associate the monitor with other devices, like audio or input devices. In this
    // sample we generate a random container ID GUID, but it's best practice to choose a stable container ID for a
    // unique monitor or to use "this" device's container ID for a permanent/integrated monitor.
    // ==============================

    // Create a container ID
    CoCreateGuid(&MonitorInfo.MonitorContainerId);

    IDARG_IN_MONITORCREATE MonitorCreate = {};
    MonitorCreate.ObjectAttributes = &Attr;
    MonitorCreate.pMonitorInfo = &MonitorInfo;

    // Create a monitor object with the specified monitor descriptor
    IDARG_OUT_MONITORCREATE MonitorCreateOut;
    NTSTATUS Status = IddCxMonitorCreate(m_Adapter, &MonitorCreate, &MonitorCreateOut);
    if (NT_SUCCESS(Status))
    {
        m_Monitor = MonitorCreateOut.MonitorObject;

        // Associate the monitor with this device context
        auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(MonitorCreateOut.MonitorObject);
        pContext->pContext = this;

        // Tell the OS that the monitor has been plugged in
        IDARG_OUT_MONITORARRIVAL ArrivalOut;
        Status = IddCxMonitorArrival(m_Monitor, &ArrivalOut);
    }
}

void IndirectDeviceContext::AssignSwapChain(IDDCX_SWAPCHAIN SwapChain, LUID RenderAdapter, HANDLE NewFrameEvent)
{
    m_ProcessingThread.reset();

    auto Device = make_shared<Direct3DDevice>(RenderAdapter);
    if (FAILED(Device->Init()))
    {
        // It's important to delete the swap-chain if D3D initialization fails, so that the OS knows to generate a new
        // swap-chain and try again.
        WdfObjectDelete(SwapChain);
    }
    else
    {
        // Create a new swap-chain processing thread
        m_ProcessingThread.reset(new SwapChainProcessor(SwapChain, Device, NewFrameEvent));
    }
}

void IndirectDeviceContext::UnassignSwapChain()
{
    // Stop processing the last swap-chain
    m_ProcessingThread.reset();
}

#pragma endregion

#pragma region DDI Callbacks

_Use_decl_annotations_
NTSTATUS IddSampleAdapterInitFinished(IDDCX_ADAPTER AdapterObject, const IDARG_IN_ADAPTER_INIT_FINISHED* pInArgs)
{
    // This is called when the OS has finished setting up the adapter for use by the IddCx driver. It's now possible
    // to report attached monitors.

    auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(AdapterObject);
    if (NT_SUCCESS(pInArgs->AdapterInitStatus))
    {
        pContext->pContext->FinishInit();
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS IddSampleAdapterCommitModes(IDDCX_ADAPTER AdapterObject, const IDARG_IN_COMMITMODES* pInArgs)
{
    UNREFERENCED_PARAMETER(AdapterObject);
    UNREFERENCED_PARAMETER(pInArgs);

    // For the sample, do nothing when modes are picked - the swap-chain is taken care of by IddCx

    // ==============================
    // TODO: In a real driver, this function would be used to reconfigure the device to commit the new modes. Loop
    // through pInArgs->pPaths and look for IDDCX_PATH_FLAGS_ACTIVE. Any path not active is inactive (e.g. the monitor
    // should be turned off).
    // ==============================

    return STATUS_SUCCESS;
}
_Use_decl_annotations_
NTSTATUS IddSampleParseMonitorDescription(const IDARG_IN_PARSEMONITORDESCRIPTION* pInArgs, IDARG_OUT_PARSEMONITORDESCRIPTION* pOutArgs)
{
    // ==============================
    // TODO: In a real driver, this function would be called to generate monitor modes for an EDID by parsing it. In
    // this sample driver, we hard-code the EDID, so this function can generate known modes.
    // ==============================

    for (int i = 0; i < monitorModes.size(); i++) {
        s_KnownMonitorModes2.push_back(dispinfo(std::get<0>(monitorModes[i]), std::get<1>(monitorModes[i]), std::get<2>(monitorModes[i])));
    }
    pOutArgs->MonitorModeBufferOutputCount = (UINT)monitorModes.size();

    if (pInArgs->MonitorModeBufferInputCount < monitorModes.size())
    {
        // Return success if there was no buffer, since the caller was only asking for a count of modes
        return (pInArgs->MonitorModeBufferInputCount > 0) ? STATUS_BUFFER_TOO_SMALL : STATUS_SUCCESS;
    }
    else
    {
        // Copy the known modes to the output buffer
        for (DWORD ModeIndex = 0; ModeIndex < monitorModes.size(); ModeIndex++)
        {
            pInArgs->pMonitorModes[ModeIndex].Size = sizeof(IDDCX_MONITOR_MODE);
            pInArgs->pMonitorModes[ModeIndex].Origin = IDDCX_MONITOR_MODE_ORIGIN_MONITORDESCRIPTOR;
            pInArgs->pMonitorModes[ModeIndex].MonitorVideoSignalInfo = s_KnownMonitorModes2[ModeIndex];
        }

        // Set the preferred mode as represented in the EDID
        pOutArgs->PreferredMonitorModeIdx = 0;

        return STATUS_SUCCESS;
    }
}

_Use_decl_annotations_
NTSTATUS IddSampleMonitorGetDefaultModes(IDDCX_MONITOR MonitorObject, const IDARG_IN_GETDEFAULTDESCRIPTIONMODES* pInArgs, IDARG_OUT_GETDEFAULTDESCRIPTIONMODES* pOutArgs)
{
    UNREFERENCED_PARAMETER(MonitorObject);
    UNREFERENCED_PARAMETER(pInArgs);
    UNREFERENCED_PARAMETER(pOutArgs);

    // Should never be called since we create a single monitor with a known EDID in this sample driver.

    // ==============================
    // TODO: In a real driver, this function would be called to generate monitor modes for a monitor with no EDID.
    // Drivers should report modes that are guaranteed to be supported by the transport protocol and by nearly all
    // monitors (such 640x480, 800x600, or 1024x768). If the driver has access to monitor modes from a descriptor other
    // than an EDID, those modes would also be reported here.
    // ==============================

    return STATUS_NOT_IMPLEMENTED;
}

/// <summary>
/// Creates a target mode from the fundamental mode attributes.
/// </summary>
void CreateTargetMode(DISPLAYCONFIG_VIDEO_SIGNAL_INFO& Mode, UINT Width, UINT Height, UINT VSync)
{
    Mode.totalSize.cx = Mode.activeSize.cx = Width;
    Mode.totalSize.cy = Mode.activeSize.cy = Height;
    Mode.AdditionalSignalInfo.vSyncFreqDivider = 1;
    Mode.AdditionalSignalInfo.videoStandard = 255;
    Mode.vSyncFreq.Numerator = VSync;
    Mode.vSyncFreq.Denominator = Mode.hSyncFreq.Denominator = 1;
    Mode.hSyncFreq.Numerator = VSync * Height;
    Mode.scanLineOrdering = DISPLAYCONFIG_SCANLINE_ORDERING_PROGRESSIVE;
    Mode.pixelRate = VSync * Width * Height;
}

void CreateTargetMode(IDDCX_TARGET_MODE& Mode, UINT Width, UINT Height, UINT VSync)
{
    Mode.Size = sizeof(Mode);
    CreateTargetMode(Mode.TargetVideoSignalInfo.targetVideoSignalInfo, Width, Height, VSync);
}

_Use_decl_annotations_
NTSTATUS IddSampleMonitorQueryModes(IDDCX_MONITOR MonitorObject, const IDARG_IN_QUERYTARGETMODES* pInArgs, IDARG_OUT_QUERYTARGETMODES* pOutArgs)////////////////////////////////////////////////////////////////////////////////
{
    UNREFERENCED_PARAMETER(MonitorObject);

    vector<IDDCX_TARGET_MODE> TargetModes(monitorModes.size());

    // Create a set of modes supported for frame processing and scan-out. These are typically not based on the
    // monitor's descriptor and instead are based on the static processing capability of the device. The OS will
    // report the available set of modes for a given output as the intersection of monitor modes with target modes.

    for (int i = 0; i < monitorModes.size(); i++) {
        CreateTargetMode(TargetModes[i], std::get<0>(monitorModes[i]), std::get<1>(monitorModes[i]), std::get<2>(monitorModes[i]));
    }

    pOutArgs->TargetModeBufferOutputCount = (UINT)TargetModes.size();

    if (pInArgs->TargetModeBufferInputCount >= TargetModes.size())
    {
        copy(TargetModes.begin(), TargetModes.end(), pInArgs->pTargetModes);
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS IddSampleMonitorAssignSwapChain(IDDCX_MONITOR MonitorObject, const IDARG_IN_SETSWAPCHAIN* pInArgs)
{
    auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(MonitorObject);
    pContext->pContext->AssignSwapChain(pInArgs->hSwapChain, pInArgs->RenderAdapterLuid, pInArgs->hNextSurfaceAvailable);
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS IddSampleMonitorUnassignSwapChain(IDDCX_MONITOR MonitorObject)
{
    auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(MonitorObject);
    pContext->pContext->UnassignSwapChain();
    return STATUS_SUCCESS;
}

#pragma endregion
