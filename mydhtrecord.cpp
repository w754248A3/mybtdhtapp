#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <thread>
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
#include <iostream>
#include <fstream>
#include <format>
#include "leikaifeng.h"
#include "mysqliteclass.h"
#include "mybtclass.h"
#include "mywebview.h"
boost::asio::ip::address createaddress(const char *str)
{

        boost::asio::ip::address_v4 ip_v4 = boost::asio::ip::address_v4::from_string(str);
        boost::asio::ip::address ip{ip_v4};

        return ip;
}

void settorrent(lt::add_torrent_params &p)
{
        p.flags = lt::torrent_flags::upload_mode;

        boost::asio::ip::address ip = createaddress("192.168.0.110");

        p.peers.push_back(libtorrent::tcp::endpoint{ip, 6881});

        p.save_path = "D:/mybtdownload/"; // save in current dir
}

void isaddtorreent(lt::session &ses, lt::sha1_hash &hash, SqlMy::MyHashCountTable& counttable){
        std::string key{};

        BtMy::GetHash16String(hash.to_string(), &key);
        int64_t n=0;
        if(!counttable.Insert(key, &n)){
                Print("inset count table false");
        }


        Print("count ",n);


        if(n ==1){
                lt::add_torrent_params p{};

                p.info_hashes = lt::info_hash_t{hash};

                settorrent(p);

                ses.async_add_torrent(p);
        }
        else{
                Print("count > 1 ",n, key);
        }
}


void addtorrent(lt::session &ses, lt::sha1_hash &hash, std::unordered_map<std::string, int> &map)
{

        auto key = hash.to_string();

        if (map.find(key) == map.end())
        {
                // Print("bu cun zai");

                map.emplace(key, 0);

                lt::add_torrent_params p{};

                p.info_hashes = lt::info_hash_t{hash};

                settorrent(p);

                ses.async_add_torrent(p);
        }
        else
        {
                // Print("yi  cun zai");
        }
}



void WriteBuffToFile(std::vector<char> &buff, const std::string &path)
{
    std::ofstream outputFile(UTF8::GetMultiByteFromUTF8(path).data(), std::ios::binary);

    if (!outputFile)
    {
        Exit("open file error");
    }

    // 将字节缓冲区写入文件
    outputFile.write(buff.data(), (long long)buff.size());

    if (!outputFile)
    {
        Exit("weite file error");
    }

    
    outputFile.close();

}


void SaveTorrentFile(const lt::torrent_info& info){
        lt::create_torrent ct(info);
        auto te = ct.generate();
        std::vector<char> buffer;
        bencode(std::back_inserter(buffer), te);

        std::string name;
        BtMy::GetHash16String(info.info_hashes().get_best().to_string(), &name);

        auto filename =std::string{"./torrent/"} + name+".torrent";

        
        Print("ok");
       WriteBuffToFile(buffer, filename);
}

void SaveFileInfo(const lt::torrent_info& info){


        auto& name = info.name();

        auto fileCount = info.num_files();
        auto total_size =info.total_size();

        auto& filestorm = info.orig_files();
        
        Print("name:",  UTF8::GetMultiByteFromUTF8(name), "fileCount", fileCount, "total_size", total_size);
        for (auto const &n  : filestorm.file_range())
        {
                auto filepath = filestorm.file_path(n);
                auto fileSize = filestorm.file_size(n);

                Print("          item:", "fileSize", fileSize, UTF8::GetMultiByteFromUTF8(filepath));
        }
}



void runwebview(){
        auto db = std::make_shared<SqlMy::MySqliteConnect>("./data.db");
        MyWebView::Func(std::make_shared<SqlMy::MyWebViewSelectClass>(db), LR"(C:\Users\PC\cpp\myvue\fileView\dist\torrent)");
}

void f()
{
        auto countdb = std::make_shared<SqlMy::MySqliteConnect>("file:v3Uv0oBM?mode=memory&cache=shared", true);

        auto db = std::make_shared<SqlMy::MySqliteConnect>("./data.db");
        SqlMy::MyDataInsertClass table{db};

        SqlMy::MyHashCountTable counttable{countdb};

        std::thread t{runwebview};
        
        lt::settings_pack p;
        p.set_int(lt::settings_pack::alert_mask, lt::alert_category::status | lt::alert_category::dht | lt::alert_category::dht_operation | lt::alert_category::ip_block | lt::alert_category::error);

        p.set_str(lt::settings_pack::string_types::dht_bootstrap_nodes,
                  "router.bittorrent.com:6881,router.utorrent.com:6881,router.bitcomet.com:6881,dht.transmissionbt.com:6881");

        p.set_str(lt::settings_pack::string_types::dht_bootstrap_nodes,
                  "192.168.0.110:6881");

        lt::session ses{p};
     
        //std::unordered_map<std::string, int> map{};
      
        libtorrent::time_duration waitmy{std::chrono::seconds{5}};
        //magnet:?xt=urn:btih:ffcd1c04449c405dd5f4d3e427c2c708ad67304a
        for (;;)
        {
                while (ses.wait_for_alert(waitmy) == nullptr) {
                        Print("WAIT NULL");
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
                               
                        }
                      
                        if (auto v = lt::alert_cast<libtorrent::add_torrent_alert>(a))
                        {

                             
                                Print("add tr:");
                        }
                       
                        if (auto v = lt::alert_cast<libtorrent::dht_get_peers_alert>(a))
                        {
                              
                                isaddtorreent(ses, v->info_hash, counttable);
                               
                        }

                        if (auto statealert = lt::alert_cast<lt::state_update_alert>(a))
                        {

                                for (auto &statu : statealert->status)
                                {
                                        Print("tr name:", UTF8::GetMultiByteFromUTF8(statu.name));
                                }
                        }

                        if (auto immutable_item = lt::alert_cast<lt::dht_immutable_item_alert>(a))
                        {
                              
                        }

                        if (auto torrent_finished = lt::alert_cast<lt::torrent_finished_alert>(a))
                        {
                                Print("torrent_finished run");
                        }
                        if (lt::alert_cast<lt::torrent_error_alert>(a))
                        {
                              
                        }

                        if (auto session_error = lt::alert_cast<lt::session_error_alert>(a))
                        {
                                Print("session_error run");
                        }


                        if (auto metadata_received =lt::alert_cast<lt::metadata_received_alert>(a))
                        {
                              Print("metadata_received run");

                              auto handle = metadata_received->handle;
                                auto info = handle.torrent_file();
                               auto data = BtMy::GetTorrentData(*info);
                               table.Insert(data);
                                ses.remove_torrent(handle);
                        }
                }
                //std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                //ses.post_torrent_updates();

              

        }
}
int main(int argc, char const *argv[])
{
        
        try
        {

                f();
        }
        catch (std::exception &e)
        {
                std::cerr << "Error: " << e.what() << std::endl;
        }
}