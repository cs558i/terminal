// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
//
// Module Name:
// - WindowsPackageManagerFactory.h
//
// Abstract:
// - These set of factories are designed to create production-level instances of WinGet objects
//   Two separate factories are needed here:
//   - WindowsPackageManagerDefaultFactory: This factory uses the standard WinRT activation mechanism
//   - WindowsPackageManagerManualActivationFactory: This factory uses manual activation to create the WinGet objects.
//                                                   This is necessary for elevated sessions.
//   WindowsPackageManagerFactory is the base class for both factories.
// Author:
// - Carlos Zamora (carlos-zamora) 23-Jul-2024

#pragma once

#include "combaseapi.h"
#include "wil/result_macros.h"
#include "wil/win32_helpers.h"
#include <type_traits>

#include <winrt/Microsoft.Management.Deployment.h>

namespace winrt::TerminalApp::implementation
{
    class WindowsPackageManagerFactory
    {
    public:
        WindowsPackageManagerFactory() = default;

        Microsoft::Management::Deployment::PackageManager CreatePackageManager()
        {
            return CreateInstance<Microsoft::Management::Deployment::PackageManager>();
        }

        Microsoft::Management::Deployment::FindPackagesOptions CreateFindPackagesOptions()
        {
            return CreateInstance<Microsoft::Management::Deployment::FindPackagesOptions>();
        }

        Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions CreateCreateCompositePackageCatalogOptions()
        {
            return CreateInstance<Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions>();
        }

        Microsoft::Management::Deployment::InstallOptions CreateInstallOptions()
        {
            return CreateInstance<Microsoft::Management::Deployment::InstallOptions>();
        }

        Microsoft::Management::Deployment::UninstallOptions CreateUninstallOptions()
        {
            return CreateInstance<Microsoft::Management::Deployment::UninstallOptions>();
        }

        Microsoft::Management::Deployment::PackageMatchFilter CreatePackageMatchFilter()
        {
            return CreateInstance<Microsoft::Management::Deployment::PackageMatchFilter>();
        }

    protected:
        // TODO CARLOS: the compiler isn't allowing a virtual template? Or am I doing something wrong here?
        virtual template<typename T>
        T CreateInstance(const winrt::guid& clsid, const winrt::guid iid);

    private:
        template<typename T>
        T CreateInstance()
        {
            winrt::guid clsid, iid;
            if (std::is_same<T, Microsoft::Management::Deployment::PackageManager>::value)
            {
                clsid = { "C53A4F16-787E-42A4-B304-29EFFB4BF597" };
                iid = { __uuidof(Microsoft::Management::Deployment::PackageManager) };
            }
            else if (std::is_same<T, Microsoft::Management::Deployment::FindPackagesOptions>::value)
            {
                clsid = { "572DED96-9C60-4526-8F92-EE7D91D38C1A" };
                iid = { __uuidof(Microsoft::Management::Deployment::FindPackagesOptions) };
            }
            else if (std::is_same<T, Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions>::value)
            {
                clsid = { "526534B8-7E46-47C8-8416-B1685C327D37" };
                iid = { __uuidof(Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions) };
            }
            else if (std::is_same<T, Microsoft::Management::Deployment::InstallOptions>::value)
            {
                clsid = { "1095F097-EB96-453B-B4E6-1613637F3B14" };
                iid = { __uuidof(Microsoft::Management::Deployment::InstallOptions) };
            }
            else if (std::is_same<T, Microsoft::Management::Deployment::UninstallOptions>::value)
            {
                clsid = { "E1D9A11E-9F85-4D87-9C17-2B93143ADB8D" };
                iid = { __uuidof(Microsoft::Management::Deployment::UninstallOptions) };
            }
            else if (std::is_same<T, Microsoft::Management::Deployment::PackageMatchFilter>::value)
            {
                clsid = { "D02C9DAF-99DC-429C-B503-4E504E4AB000" };
                iid = { __uuidof(Microsoft::Management::Deployment::PackageMatchFilter) };
            }
            else
            {
                static_assert(false, "Unsupported type");
            }

            return CreateInstance<T>(clsid, iid);
        }
    };

    class WindowsPackageManagerDefaultFactory : public WindowsPackageManagerFactory
    {
    public:
        WindowsPackageManagerDefaultFactory() = default;

    protected:
        template<typename T>
        T CreateInstance(const guid& clsid, const guid& iid) override
        {
            return winrt::create_instance(clsid, iid);

            // TODO CARLOS: this is the original code in DevHome (converted from C#).
            //              But (a similar line to) the line above worked and is more concise.
            //              Verify it works properly and delete the commented code below.

            //IUnknown* pUnknown;
            //try
            //{
            //    THROW_IF_FAILED(CoCreateInstance(clsid, nullptr, CLSCTX::CLSCTX_LOCAL_SERVER, iid, reinterpret_cast<void**>(&pUnknown)));
            //    return winrt::from_abi<T>(pUnknown);
            //}
            //finally
            //{
            //    // CoCreateInstance and from_abi both AddRef on the native object.
            //    // Release once to prevent memory leak.
            //    if (pUnknown)
            //    {
            //        pUnknown->Release();
            //    }
            //}
        }
    };

    class WindowsPackageManagerManualActivationFactory : public WindowsPackageManagerFactory
    {
    public:
        WindowsPackageManagerManualActivationFactory()
        {
            _winrtactModule.reset(LoadLibraryExW(L"winrtact.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));
        }

    protected:
        template<typename T>
        T CreateInstance(const guid& clsid, const guid& iid) override
        {
            IUnknown* pUnknown;
            try
            {
                const auto WinGetServerManualActivation_CreateInstance = GetProcAddresssByFunctionDeclaration(_winrtactModule.get(), WinGetServerManualActivation_CreateInstance);
                THROW_LAST_ERROR_IF(!WinGetServerManualActivation_CreateInstance);
                THROW_IF_FAILED(WinGetServerManualActivation_CreateInstance(clsid, iid, 0, reinterpret_cast<void**>(pUnknown)));
                return winrt::from_abi<T>(pUnknown);
            }
            finally
            {
                // CoCreateInstance and from_abi both AddRef on the native object.
                // Release once to prevent memory leak.
                if (pUnknown)
                {
                    pUnknown->Release();
                }
            }
        }

    private:
        wil::unique_hmodule _winrtactModule;
    };
}
