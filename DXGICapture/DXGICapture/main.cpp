#include <d3d11.h>
#include <dxgi1_2.h>
#include <stdio.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#pragma warning(disable:4996)


int main()
{
    HRESULT hr;
    // Driver types supported ֧�ֵ�������������
    D3D_DRIVER_TYPE DriverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };

    UINT NumDriverTypes = ARRAYSIZE(DriverTypes);
    // Feature levels supported ֧�ֵĹ��ܼ���
    D3D_FEATURE_LEVEL FeatureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_1
    };

    UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);

    D3D_FEATURE_LEVEL FeatureLevel;

    ID3D11Device *_pDX11Dev = nullptr;
    ID3D11DeviceContext *_pDX11DevCtx = nullptr;

    // Create D3D device ����D3D�豸
    for (UINT index = 0; index < NumDriverTypes; index++)
    {
        hr = D3D11CreateDevice(nullptr,
            DriverTypes[index],
            nullptr, 0,
            FeatureLevels,
            NumFeatureLevels,
            D3D11_SDK_VERSION,
            &_pDX11Dev,
            &FeatureLevel,
            &_pDX11DevCtx);

        if (SUCCEEDED(hr)) {
            break;
        }
    }
    
    IDXGIDevice *_pDXGIDev = nullptr;
    // Get DXGI device ��ȡ DXGI �豸
    hr = _pDX11Dev->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&_pDXGIDev));
    if (FAILED(hr)) {
        return false;
    }
        
    IDXGIAdapter *_pDXGIAdapter = nullptr;
    // Get DXGI adapter ��ȡ DXGI ������
    hr = _pDXGIDev->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&_pDXGIAdapter));
    if (FAILED(hr)) {
        return false;
    }

    UINT i = 0;
    IDXGIOutput *_pDXGIOutput = nullptr;
    // Get output ��ȡ���
    hr = _pDXGIAdapter->EnumOutputs(i, &_pDXGIOutput);
    if (FAILED(hr)) {
        return false;
    }

    DXGI_OUTPUT_DESC DesktopDesc;
    // Get output description struct ��ȡ��������ṹ
    _pDXGIOutput->GetDesc(&DesktopDesc);

    IDXGIOutput1 *_pDXGIOutput1 = nullptr;
    // QI for Output1 ����ӿڸ�Output1
    hr = _pDXGIOutput->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(&_pDXGIOutput1));
    if (FAILED(hr)) {
        return false;
    }

    IDXGIOutputDuplication *_pDXGIOutputDup = nullptr;
    // Create desktop duplication �������渱��
    hr = _pDXGIOutput1->DuplicateOutput(_pDX11Dev, &_pDXGIOutputDup);
    if (FAILED(hr)) {
        return false;
    }

    for (int i = 0; i < 10; i++) 
    {
        IDXGIResource *desktopResource = nullptr;
        DXGI_OUTDUPL_FRAME_INFO frameInfo;
        hr = _pDXGIOutputDup->AcquireNextFrame(20, &frameInfo, &desktopResource);
        if (FAILED(hr))
        {
            if (hr == DXGI_ERROR_WAIT_TIMEOUT)
            {
                if (desktopResource) {
                    desktopResource->Release();
                    desktopResource = nullptr;
                }
                hr = _pDXGIOutputDup->ReleaseFrame();
            }
            else
            {
                return false;
            }
        }

        ID3D11Texture2D *_pDX11Texture = nullptr;
        // query next frame staging buffer ��ѯ��һ֡�ݴ滺����
        hr = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&_pDX11Texture));
        desktopResource->Release();
        desktopResource = nullptr;
        if (FAILED(hr)) {
            return false;
        }

        ID3D11Texture2D *_pCopyBuffer = nullptr;

        D3D11_TEXTURE2D_DESC desc;
        // copy old description ���ƾ�����
        if (_pDX11Texture)
        {
            _pDX11Texture->GetDesc(&desc);
        }
        else if (_pCopyBuffer)
        {
            _pCopyBuffer->GetDesc(&desc);
        }
        else
        {
            return false;
        }

        // create a new staging buffer for fill frame image Ϊ���֡ͼ�񴴽�һ���µ��ݴ滺����
        if (_pCopyBuffer == nullptr) {
            D3D11_TEXTURE2D_DESC CopyBufferDesc;
            CopyBufferDesc.Width = desc.Width;
            CopyBufferDesc.Height = desc.Height;
            CopyBufferDesc.MipLevels = 1;
            CopyBufferDesc.ArraySize = 1;
            CopyBufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            CopyBufferDesc.SampleDesc.Count = 1;
            CopyBufferDesc.SampleDesc.Quality = 0;
            CopyBufferDesc.Usage = D3D11_USAGE_STAGING;
            CopyBufferDesc.BindFlags = 0;
            CopyBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            CopyBufferDesc.MiscFlags = 0;

            hr = _pDX11Dev->CreateTexture2D(&CopyBufferDesc, nullptr, &_pCopyBuffer);
            if (FAILED(hr)) {
                return false;
            }
        }

        if (_pDX11Texture)
        {
            // copy next staging buffer to new staging buffer ����һ���ݴ滺�������Ƶ��µ��ݴ滺����
            _pDX11DevCtx->CopyResource(_pCopyBuffer, _pDX11Texture);
        }

        IDXGISurface *CopySurface = nullptr;
        // create staging buffer for map bits Ϊӳ��λ�����ݴ滺����
        hr = _pCopyBuffer->QueryInterface(__uuidof(IDXGISurface), (void **)&CopySurface);
        if (FAILED(hr)) {
            return false;
        }

        DXGI_MAPPED_RECT MappedSurface;
        // copy bits to user space ��λ���Ƶ��û��ռ�
        hr = CopySurface->Map(&MappedSurface, DXGI_MAP_READ);

        char picName[128] = { 0 };
        snprintf(picName, sizeof(picName), "Screen%d.rgb", i);
        FILE *p = fopen(picName,"wb");

        if (SUCCEEDED(hr))
        {
            for (int i = 0; i < 1080; ++i)
            {
                fwrite(MappedSurface.pBits + i * MappedSurface.Pitch, 1, MappedSurface.Pitch, p);
            }
            CopySurface->Unmap();
        }

        fclose(p);

        CopySurface->Unmap();
        hr = CopySurface->Release();
        CopySurface = nullptr;

        if (_pDXGIOutputDup)
        {
            hr = _pDXGIOutputDup->ReleaseFrame();
        }
        Sleep(1000);
    }

    return 0;
}