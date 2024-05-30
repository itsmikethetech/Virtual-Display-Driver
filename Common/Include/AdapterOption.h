#pragma once

// Anonymous namespace for usings
namespace
{
    using namespace std;
    using namespace Microsoft::IndirectDisp;
    using namespace Microsoft::WRL;

    struct AdapterOption
    {
        bool hasTargetAdapter{};
        LUID adapterLuid{};

        void load(const char* path)
        {
            ifstream ifs{ path };

            if (!ifs.is_open())
            {
                return;
            }

            std::string line;
            getline(ifs, line);

            std::wstring target_name{ line.begin(), line.end() };

            ComPtr<IDXGIFactory1> factory{};
            if (!SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
            {
                return;
            }

            for (UINT i = 0;; i++)
            {
                ComPtr<IDXGIAdapter> adapter{};
                if (!SUCCEEDED(factory->EnumAdapters(i, &adapter)))
                {
                    break;
                }

                DXGI_ADAPTER_DESC desc;
                if (!SUCCEEDED(adapter->GetDesc(&desc)))
                {
                    break;
                }

                // We found our target
                if (_wcsicmp(target_name.c_str(), desc.Description) == 0)
                {
                    adapterLuid = desc.AdapterLuid;
                    hasTargetAdapter = true;
                }
            }
        }

        void apply(const IDDCX_ADAPTER& adapter)
        {
            if (hasTargetAdapter)
            {
                if (IDD_IS_FUNCTION_AVAILABLE(IddCxAdapterSetRenderAdapter))
                {
                    IDARG_IN_ADAPTERSETRENDERADAPTER arg{};
                    arg.PreferredRenderAdapter = adapterLuid;

                    IddCxAdapterSetRenderAdapter(adapter, &arg);
                }
            }
        }
    };
}