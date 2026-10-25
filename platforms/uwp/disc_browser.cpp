// SPDX-License-Identifier: GPL-3.0-or-later
#include "disc_browser.h"
#include "storage.h"
#include <RmlUi/Core.h>
#include <winrt/Windows.Storage.AccessCache.h>
#include <algorithm>
#include <chrono>
#include <cwctype>
#include <future>
#include <memory>
#include <vector>
namespace melee::uwp {
namespace {
using namespace winrt::Windows::Storage;
struct Entry {std::string name;StorageFolder folder{nullptr};StorageFile file{nullptr};};
struct Listing {std::vector<Entry> entries;std::string path,error;};
Listing list(StorageFolder folder){
 winrt::init_apartment(winrt::apartment_type::multi_threaded);Listing out;
 try{
  if(!folder){for(auto const& drive:usb_drives().get())out.entries.push_back({"USB: "+winrt::to_string(drive.Name()),drive});out.path="Select a USB drive";}
  else{
   out.path=winrt::to_string(folder.Path());
   for(auto const& item:folder.GetFoldersAsync().get())out.entries.push_back({winrt::to_string(item.Name()),item});
   for(auto const& item:folder.GetFilesAsync().get()){
    auto ext=std::wstring(item.FileType());for(auto& ch:ext)ch=std::towlower(ch);
    if(ext==L".iso"||ext==L".gcm"||ext==L".rvz"||ext==L".ciso"||ext==L".gcz"||ext==L".wia")out.entries.push_back({winrt::to_string(item.Name()),nullptr,item});
   }
   std::stable_sort(out.entries.begin(),out.entries.end(),[](const Entry& a,const Entry& b){if(bool(a.folder)!=bool(b.folder))return bool(a.folder);return a.name<b.name;});
  }
 }catch(winrt::hresult_error const& e){out.error="Cannot access this folder: "+winrt::to_string(e.message());}
 winrt::uninit_apartment();return out;
}
std::string escaped(const std::string& s){std::string out;for(char c:s){if(c=='&')out+="&amp;";else if(c=='<')out+="&lt;";else if(c=='>')out+="&gt;";else if(c=='\"')out+="&quot;";else out+=c;}return out;}
class Browser final:public Rml::EventListener {
public:
 Rml::ElementDocument* document=nullptr;
 std::function<void(std::string,std::string)> done;
 StorageFolder folder{nullptr};std::vector<StorageFolder> parents;
 Listing listing;std::future<Listing> pending;std::vector<std::string> ids;
 bool completed=false;uint64_t last_axis=0;
 Browser(Rml::Context* context,std::function<void(std::string,std::string)> callback):done(std::move(callback)){
  const std::string markup=R"(<rml><head><title>Choose Disc - USB</title><link type="text/rcss" href="launcher.rcss"/>
<style>
body { decorator: none; background-color: #07071af0; padding: 7%; display: block; }
#usb-panel { background-color: #10152bf8; border: 3dp #c9a227; border-radius: 18dp; padding: 24dp; height: 88%; }
h1 { font-size: 30dp; margin: 0 0 12dp; color: #ffcb00; }
#usb-path { font-size: 16dp; margin-bottom: 14dp; }
#usb-list { height: 70%; overflow-y: auto; }
.usb-row { display: block; width: 100%; padding: 14dp; margin: 0 0 7dp; box-sizing: border-box; background-color: #222a48; color: #e8e8f0; border: 1dp #8a6a1a; border-radius: 7dp; font-size: 20dp; text-align: left; }
.usb-row:focus, .usb-row:hover { background-color: #ffcb00; color: #07071a; }
#usb-hint { margin-top: 12dp; font-size: 15dp; }
</style></head><body><div id="usb-panel"><h1>Choose Disc / USB</h1><div id="usb-path"></div><div id="usb-list"></div><div id="usb-hint">A / Enter: select - B / Escape: back - X: refresh. The disc stays on USB.</div></div></body></rml>)";
  auto source=std::string(SDL_GetBasePath())+"resources/usb-browser.rml";
  document=context->LoadDocumentFromMemory(markup,source);if(!document)throw std::runtime_error("Cannot create the USB disc browser.");
  document->AddEventListener(Rml::EventId::Click,this);document->AddEventListener(Rml::EventId::Keydown,this);
  document->Show(Rml::ModalFlag::Modal);scan();
 }
 ~Browser(){if(pending.valid())pending.wait();if(document){document->RemoveEventListener(Rml::EventId::Click,this);document->RemoveEventListener(Rml::EventId::Keydown,this);document->Close();}}
 void scan(){if(pending.valid())return;document->GetElementById("usb-path")->SetInnerRML("Reading USB...");auto current=folder;pending=std::async(std::launch::async,[current]{return list(current);});}
 void finish(std::string path={},std::string error={}){completed=true;document->Hide();done(std::move(path),std::move(error));}
 void tick(){
  if(!pending.valid()||pending.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return;
  try{listing=pending.get();}catch(winrt::hresult_error const& e){finish({},winrt::to_string(e.message()));return;}catch(std::exception const& e){finish({},e.what());return;}
  document->GetElementById("usb-path")->SetInnerRML(escaped(listing.error.empty()?listing.path:listing.error));
  ids={"usb-back","usb-refresh","usb-cancel"};
  std::string rows="<button class=\"usb-row\" id=\"usb-back\">Back</button><button class=\"usb-row\" id=\"usb-refresh\">Refresh</button><button class=\"usb-row\" id=\"usb-cancel\">Cancel</button>";
  for(size_t i=0;i<listing.entries.size();++i){auto id="usb-entry-"+std::to_string(i);ids.push_back(id);const auto& e=listing.entries[i];rows+="<button class=\"usb-row\" id=\""+id+"\">"+escaped((e.folder?"[Folder] ":"[Disc] ")+e.name)+"</button>";}
  if(listing.entries.empty())rows+="<p>No supported disc images here. Open the folder containing your ISO or RVZ.</p>";
  document->GetElementById("usb-list")->SetInnerRML(rows);document->UpdateDocument();
  document->GetElementById(ids[listing.entries.empty()?0:3])->Focus();
 }
 void back(){if(pending.valid())return;if(!folder){finish();return;}if(parents.empty())folder=nullptr;else{folder=parents.back();parents.pop_back();}scan();}
 void action(const std::string& id){
  if(pending.valid())return;
  if(id=="usb-back"){back();return;}if(id=="usb-refresh"){scan();return;}if(id=="usb-cancel"){finish();return;}
  if(id.rfind("usb-entry-",0)!=0)return;
  auto index=std::strtoul(id.c_str()+10,nullptr,10);if(index>=listing.entries.size())return;
  const auto e=listing.entries[index];
  if(e.folder){if(folder)parents.push_back(folder);folder=e.folder;scan();}
  else try{remember_disc(e.file);finish(winrt::to_string(e.file.Path()));}catch(winrt::hresult_error const& error){finish({},winrt::to_string(error.message()));}
 }
 void step(int amount){if(pending.valid()||ids.empty())return;auto focus=document->GetContext()->GetFocusElement();auto it=focus?std::find(ids.begin(),ids.end(),focus->GetId()):ids.end();int index=it==ids.end()?0:int(it-ids.begin());index=(index+amount+int(ids.size()))%int(ids.size());if(auto e=document->GetElementById(ids[size_t(index)])){e->Focus();e->ScrollIntoView();}}
 void activate(){auto e=document->GetContext()->GetFocusElement();if(e)action(e->GetId());}
 bool event(const SDL_Event& event){
  if(event.type==SDL_EVENT_GAMEPAD_BUTTON_DOWN){auto b=event.gbutton.button;if(b==SDL_GAMEPAD_BUTTON_DPAD_UP)step(-1);else if(b==SDL_GAMEPAD_BUTTON_DPAD_DOWN)step(1);else if(b==SDL_GAMEPAD_BUTTON_SOUTH||b==SDL_GAMEPAD_BUTTON_START)activate();else if(b==SDL_GAMEPAD_BUTTON_EAST)back();else if(b==SDL_GAMEPAD_BUTTON_WEST)scan();return true;}
  if(event.type==SDL_EVENT_GAMEPAD_AXIS_MOTION){if(event.gaxis.axis==SDL_GAMEPAD_AXIS_LEFTY&&std::abs(event.gaxis.value)>18000&&SDL_GetTicks()-last_axis>200){step(event.gaxis.value>0?1:-1);last_axis=SDL_GetTicks();}return true;}
  return event.type==SDL_EVENT_KEY_DOWN||event.type==SDL_EVENT_KEY_UP||event.type==SDL_EVENT_DROP_FILE;
 }
 void ProcessEvent(Rml::Event& event) override{
  if(event.GetId()==Rml::EventId::Click){auto e=event.GetTargetElement();while(e&&e!=document){if(e->GetTagName()=="button"){action(e->GetId());break;}e=e->GetParentNode();}}
  else if(event.GetId()==Rml::EventId::Keydown){auto k=event.GetParameter<int>("key_identifier",0);if(k==Rml::Input::KI_UP)step(-1);else if(k==Rml::Input::KI_DOWN)step(1);else if(k==Rml::Input::KI_RETURN)activate();else if(k==Rml::Input::KI_ESCAPE)back();else return;event.StopPropagation();}
 }
};
std::unique_ptr<Browser> browser;
}
void choose_disc(Rml::Context* context,std::function<void(std::string,std::string)> callback){if(browser)return;browser=std::make_unique<Browser>(context,std::move(callback));}
bool disc_browser_active(){return bool(browser)&&!browser->completed;}
bool disc_browser_event(const SDL_Event& e){return browser&&!browser->completed&&browser->event(e);}
void disc_browser_tick(){if(!browser)return;browser->tick();if(browser->completed)browser.reset();}
void close_disc_browser(){browser.reset();}
}
