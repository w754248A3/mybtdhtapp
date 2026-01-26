#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <corecrt_startup.h>
#include <cstdio>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/torrent_status.hpp>
#include <string>
#include <thread>
#include <unordered_map>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN8
#endif

#ifndef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN8
#endif

#include <libtorrent/session.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/bdecode.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/session_stats.hpp>
#include <iostream>
#include <fstream>
#include <format>
#include <fcntl.h>  // _O_U16TEXT
#include "leikaifeng.h"
#include "mysqliteclass.h"
#include "mybtclass.h"
#include "mywebview.h"



namespace fs = std::filesystem;

// 获取当前可执行文件的完整路径
fs::path GetExecutablePath() {
    wchar_t buffer[MAX_PATH];
    // GetModuleFileNameW 是 Windows API，用于获取当前进程 exe 的路径
    // 使用宽字符版本 (W后缀)
    DWORD length = GetModuleFileNameW(NULL, buffer, MAX_PATH);
    if (length == 0 || length == MAX_PATH) {
        throw std::runtime_error("无法获取可执行文件路径");
    }
    return fs::path(buffer);
}


auto GetAppendExecutablePath(const std::wstring& p){

        auto path =  GetExecutablePath();
        auto addpath = path.parent_path();

        addpath.append(p);
        MyWin32Out::Print(L"GetExecutablePath:", path, L"add:", addpath);
        return  addpath.wstring();
}


boost::asio::ip::address createaddress(const char *str)
{

        boost::asio::ip::address_v4 ip_v4 = boost::asio::ip::address_v4::from_string(str);
        boost::asio::ip::address ip{ip_v4};

        return ip;
}

void settorrent(lt::add_torrent_params &p)
{
        p.flags = lt::torrent_flags::upload_mode;

        //boost::asio::ip::address ip = createaddress("192.168.0.110");

        //p.peers.push_back(libtorrent::tcp::endpoint{ip, 6881});
        
        p.save_path = "./mybtdownload/"; // save in current dir
}

void isaddtorreent(lt::session &ses, lt::sha1_hash &hash, SqlMy::MyHashCountTable& counttable){
        std::string key{};

        BtMy::GetHash16String(hash.to_string(), &key);
        int64_t n=0;
        if(!counttable.Insert(key, &n)){
                MyWin32Out::Print(L"inset count table false");
        }


        // MyWin32Out::Print("count ",n);


        if(n ==1){
                lt::add_torrent_params p{};

                p.info_hashes = lt::info_hash_t{hash};

                settorrent(p);

                ses.async_add_torrent(p);
        }
        else{
               // MyWin32Out::Print("count > 1 ",n, key);
        }
}


void addtorrent(lt::session &ses, lt::sha1_hash &hash, std::unordered_map<std::string, int> &map)
{

        auto key = hash.to_string();

        if (map.find(key) == map.end())
        {
                // MyWin32Out::Print("bu cun zai");

                map.emplace(key, 0);

                lt::add_torrent_params p{};

                p.info_hashes = lt::info_hash_t{hash};

                settorrent(p);

                ses.async_add_torrent(p);
        }
        else
        {
                // MyWin32Out::Print("yi  cun zai");
        }
}



void SaveTorrentFile(const lt::torrent_info& info){
        lt::create_torrent ct(info);
        auto te = ct.generate();
        std::vector<char> buffer;
        bencode(std::back_inserter(buffer), te);

        std::string name;
        BtMy::GetHash16String(info.info_hashes().get_best().to_string(), &name);

        auto filename =std::string{"./torrent/"} + name+".torrent";

}

void SaveFileInfo(const lt::torrent_info& info){


        auto& name = info.name();

        auto fileCount = info.num_files();
        auto total_size =info.total_size();

        auto& filestorm = info.orig_files();
        
        MyWin32Out::Print(L"name:",  UTF8::GetWideCharFromUTF8(name), L"fileCount", fileCount, L"total_size", total_size);
        for (auto const &n  : filestorm.file_range())
        {
                auto filepath = filestorm.file_path(n);
                auto fileSize = filestorm.file_size(n);

                MyWin32Out::Print(L"          item:", L"fileSize", fileSize, UTF8::GetWideCharFromUTF8(filepath));
        }
}


auto createconnect(){
        auto db = std::make_shared<SqlMy::MySqliteConnect>("./data.db");
        db->SetBusyTimeout(5000);
        
        {
                const std::string sql{"file:v3Uv0oBM?mode=memory&cache=shared"};
                //SqlMy::MySqliteConnect mdb{sql};

                db->Attach(sql,"mymdb567");
        }
        return db;
}


void runwebview(){
        auto db = createconnect();
        auto path = GetAppendExecutablePath(L"webpage");
        MyWebView::Func(std::make_shared<SqlMy::MyWebViewSelectClass>(db), path);
}

std::unordered_map<std::string, int> findNeed(){


        std::unordered_map<std::string, int> map{
        };

        auto list = {
                "ses.num_checking_torrents",
                "ses.num_stopped_torrents",
                "ses.num_upload_only_torrents",
                "ses.num_downloading_torrents",
                "ses.num_seeding_torrents",
                "ses.num_queued_seeding_torrents",
                "ses.num_queued_download_torrents",
                "ses.num_error_torrents"
                };
        
        for (const auto& item : list) {
               auto index = lt::find_metric_idx(item);
               if(index == -1){
                        MyWin32Out::Print(UTF8::GetWideCharFromUTF8( item), L"not find");
                        MyWin32Out::Exit(L"findNeed error");
               }

                map.emplace(std::string{item}, index);
        }

        return map;
}



void f(const std::string& u8peer)
{
        const auto statucmap = findNeed();

        auto db = createconnect();
        
        SqlMy::MyDataInsertClass table{db};

        SqlMy::MyHashCountTable counttable{db};

        std::thread t{runwebview};
        
        lt::settings_pack p;
        p.set_int(lt::settings_pack::alert_mask, lt::alert_category::status | lt::alert_category::dht | lt::alert_category::dht_operation | lt::alert_category::ip_block | lt::alert_category::error);

        p.set_str(lt::settings_pack::string_types::dht_bootstrap_nodes,
                  "router.bittorrent.com:6881,router.utorrent.com:6881,router.bitcomet.com:6881,dht.transmissionbt.com:6881");

        p.set_str(lt::settings_pack::string_types::dht_bootstrap_nodes,
                  u8peer);


        int allcount = 1000000;

        int active_downloads = allcount/10*8;

        int active_seeds = allcount/10*2;

        MyWin32Out::Print(L"active_downloads", active_downloads, L"active_seeds", active_seeds);
        
        p.set_int(lt::settings_pack::int_types::active_downloads, active_downloads);
        p.set_int(lt::settings_pack::int_types::active_seeds, active_seeds); // Reserve slots for seeding
        p.set_int(lt::settings_pack::int_types::active_limit, allcount);
        p.set_str(lt::settings_pack::listen_interfaces, "0.0.0.0:0,[::]:0");
        lt::session ses{p};
     
        //std::unordered_map<std::string, int> map{};
      
        libtorrent::time_duration waitmy{std::chrono::seconds{5}};
        //magnet:?xt=urn:btih:ffcd1c04449c405dd5f4d3e427c2c708ad67304a
        for (;;)
        {
                while (ses.wait_for_alert(waitmy) == nullptr) {
                        MyWin32Out::Print(L"WAIT NULL");
                }

                std::vector<lt::alert *> alerts;
                ses.pop_alerts(&alerts);
                
                for (lt::alert *a : alerts)
                {
                        // std::cout << UTF8::GetMultiByteFromUTF8(a->message()) << std::endl;
                        //  if we receive the finished alert or an error, we're done
                       
                      
                        if (auto v = lt::alert_cast<libtorrent::dht_announce_alert>(a))
                        {
                               
                                isaddtorreent(ses, v->info_hash, counttable);
                                //ses.post_session_stats();
                               
                        }
                      
                        if (auto v = lt::alert_cast<libtorrent::add_torrent_alert>(a))
                        {

                             
                                //MyWin32Out::Print(L"add tr:");
                        }
                       
                        if (auto v = lt::alert_cast<libtorrent::dht_get_peers_alert>(a))
                        {
                              
                                isaddtorreent(ses, v->info_hash, counttable);
                                //ses.post_session_stats();
                               
                        }

                        if (auto statealert = lt::alert_cast<lt::state_update_alert>(a))
                        {

                                for (auto &statu : statealert->status)
                                {
                                        MyWin32Out::Print(L"state_update_ name:", UTF8::GetWideCharFromUTF8(statu.name), 
                                        L"queue_position",   statu.queue_position,
                                        L"added_time", statu.added_time,
                                        L"completed_time", statu.completed_time,
                                        
                                        L"is seeding", statu.state & decltype(statu.state)::seeding
                                        );
                                }
                        }


                        if (auto immutable_item = lt::alert_cast<lt::dht_immutable_item_alert>(a))
                        {
                              
                        }

                        if (auto torrent_finished = lt::alert_cast<lt::torrent_finished_alert>(a))
                        {
                                MyWin32Out::Print(L"torrent_finished run");
                        }
                        if (lt::alert_cast<lt::torrent_error_alert>(a))
                        {
                              
                        }

                        if (auto session_error = lt::alert_cast<lt::session_error_alert>(a))
                        {
                                MyWin32Out::Print(L"session_error run");
                        }


                        if (auto metadata_received =lt::alert_cast<lt::metadata_received_alert>(a))
                        {
                              MyWin32Out::Print(L"metadata_received run");

                            
                              auto handle = metadata_received->handle;

                              handle.post_status();
                                auto info = handle.torrent_file();
                               auto data = BtMy::GetTorrentData(*info);
                               table.Insert(data);
                                //ses.remove_torrent(handle);
                        }

                        if (auto session_stats =lt::alert_cast<lt::session_stats_alert>(a))
                        {       auto sp = session_stats->counters();
                              for (const auto& item : statucmap) {
                              
                                MyWin32Out::Print(UTF8::GetWideCharFromUTF8(item.first) , sp[item.second]);
                              }
                        }

                        
                }
                //std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                //ses.post_torrent_updates();

              

        }
}



int wmain(int argc, wchar_t* argv[])
{
        
        _setmode(_fileno(stdout), _O_U16TEXT);
        _setmode(_fileno(stderr), _O_U16TEXT);
        _setmode(_fileno(stdin), _O_U16TEXT);

        try
        {

                if(argc != 2){
                        MyWin32Out::Print(L"need args peer");
                        return 0;
                }
                MyWin32Out::Print(argv[1]);
                auto u8peer = UTF8::GetUTF8FromWideChar(argv[1]);
                   
                f(u8peer);
        }
        catch (std::exception &e)
        {
                std::cerr << "Error: " << e.what() << std::endl;
        }

        return 0;
}