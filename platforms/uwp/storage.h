// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.h>
namespace melee::uwp {
winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Storage::StorageFolder>> usb_drives();
winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::StorageFolder> prepare_portable_root(winrt::Windows::Storage::StorageFolder drive);
void remember_disc(winrt::Windows::Storage::StorageFile const& disc);
}
