// SPDX-License-Identifier: GPL-3.0-or-later
#include <windows.h>
#include "../uwp/storage.h"
#include <atomic>
#include <string>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.Popups.h>

int melee_uwp_run_abi_checks();
using namespace winrt;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;
namespace {
std::atomic<int> result{-1};
std::atomic<bool> auto_exit{false};
IAsyncAction check_foundation() {
    co_await resume_background();
    try {
        auto folder=Windows::Storage::ApplicationData::Current().LocalFolder();
        auto_exit.store(bool(co_await folder.TryGetItemAsync(L"probe-auto-exit.txt")));
        const int code=melee_uwp_run_abi_checks();
        std::string report="Melee UWP foundation probe 0.0.1.1\n";
        report+="Upstream v0.4.2; x64 GCC objects linked with Store CRT.\n";
        report+="No game image loaded. No graphics or netplay acceptance test.\n";
        if(code==0)report+="PASS disc endian scalars/floats/arrays/bitfields/stores\nPASS x64 register and stack ABI\nPASS original Melee RNG: 30000 steps\nPASS GCC stack probe and Store varargs adapters\n";
        else report+="FAIL check code="+std::to_string(code)+"\n";
        auto drives=co_await melee::uwp::usb_drives();
        report+="USB roots visible="+std::to_string(drives.Size())+"\n";
        bool internal_rejected=false;
        try{co_await melee::uwp::prepare_portable_root(folder);}
        catch(hresult_access_denied const&){internal_rejected=true;}
        report+=internal_rejected?"PASS portable storage rejects LocalState as a game-data target\n":"FAIL internal game-data target accepted\n";
        auto file=co_await folder.CreateFileAsync(L"melee-foundation.log",Windows::Storage::CreationCollisionOption::ReplaceExisting);
        co_await Windows::Storage::FileIO::WriteTextAsync(file,to_hstring(report));
        result.store(code==0&&!internal_rejected?101:code);
    } catch(hresult_error const& error) {
        OutputDebugStringW(error.message().c_str());result.store(100);
    }
}
struct View:implements<View,IFrameworkView> {
    CoreWindow window{nullptr};bool closed=false,notified=false;
    IAsyncAction operation{nullptr};
    IAsyncOperation<Windows::UI::Popups::IUICommand> dialog{nullptr};
    void Initialize(CoreApplicationView const& app){app.Activated([this](auto&&,auto&&){window.Activate();});}
    void SetWindow(CoreWindow const& value){window=value;window.Closed([this](auto&&,auto&&){closed=true;});}
    void Load(hstring const&){}
    void Run(){
        operation=check_foundation();
        while(!closed){
            window.Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
            const int code=result.load();
            if(code>=0&&!notified){
                notified=true;
                if(auto_exit.load()){closed=true;continue;}
                Windows::UI::Popups::MessageDialog message(code==0?
                    L"Foundation checks passed. Original game code, endian reads and x64 calls work with the UWP runtime. This diagnostic does not launch Melee. Results: LocalState/melee-foundation.log":
                    L"Foundation check failed. Review LocalState/melee-foundation.log.",L"Melee UWP - Foundation test");
                dialog=message.ShowAsync();
            }
            Sleep(10);
        }
    }
    void Uninitialize(){}
};
struct Source:implements<Source,IFrameworkViewSource>{IFrameworkView CreateView(){return make<View>();}};
}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int){
    init_apartment(apartment_type::multi_threaded);CoreApplication::Run(make<Source>());return 0;
}
