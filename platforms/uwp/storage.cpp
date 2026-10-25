// SPDX-License-Identifier: GPL-3.0-or-later
#include "storage.h"
#include <windows.h>
#include <winrt/Windows.Storage.AccessCache.h>
namespace melee::uwp {
using namespace winrt;
using namespace Windows::Storage;
Windows::Foundation::IAsyncOperation<Windows::Foundation::Collections::IVectorView<StorageFolder>> usb_drives(){
    co_return co_await KnownFolders::RemovableDevices().GetFoldersAsync();
}
Windows::Foundation::IAsyncOperation<StorageFolder> prepare_portable_root(StorageFolder drive){
    if(!drive)throw hresult_invalid_argument(L"Select a USB drive first.");
    // Accept only a root returned by UWP's removable-device enumeration.
    // A missing or unwritable USB never redirects game data to LocalState.
    auto roots=co_await usb_drives();bool removable=false;
    for(auto const& root:roots){
        auto a=root.Path(),b=drive.Path();
        if(!a.empty()&&!b.empty()&&CompareStringOrdinal(a.c_str(),int(a.size()),b.c_str(),int(b.size()),TRUE)==CSTR_EQUAL){removable=true;break;}
    }
    if(!removable)throw hresult_access_denied(L"Game data must remain on the selected USB drive.");
    auto data=co_await drive.CreateFolderAsync(L"Melee-UWP",CreationCollisionOption::OpenIfExists);
    for(auto const* name:{L"user",L"textures",L"cache",L"logs"})
        co_await data.CreateFolderAsync(name,CreationCollisionOption::OpenIfExists);
    Windows::Storage::AccessCache::StorageApplicationPermissions::FutureAccessList().AddOrReplace(L"melee-usb-data",data);
    co_return data;
}
void remember_disc(StorageFile const& disc){
    if(!disc)throw hresult_invalid_argument(L"Select a game image first.");
    Windows::Storage::AccessCache::StorageApplicationPermissions::FutureAccessList().AddOrReplace(L"melee-selected-disc",disc);
}
}
