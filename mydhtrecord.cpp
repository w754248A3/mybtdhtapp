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

std::string getstring(const std::string &s)
{
        std::string res{};
        char buf[] = "0123456789ABCDEFD";

        for (auto i : s)
        {
                std::string hexString = std::format("{:02X}", i);

                res.append(hexString);
        }

        return res;
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

        boost::asio::ip::address ip = createaddress("192.168.0.110");

        p.peers.push_back(libtorrent::tcp::endpoint{ip, 6881});

        p.save_path = "D:/mybtdownload/"; // save in current dir
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
    outputFile.write(buff.data(), buff.size());

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

        auto filename =std::string{"./torrent/"} + getstring(info.info_hashes().get_best().to_string())+".torrent";

        
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

void f()
{
        lt::settings_pack p;
        p.set_int(lt::settings_pack::alert_mask, lt::alert_category::status | lt::alert_category::dht | lt::alert_category::dht_operation | lt::alert_category::ip_block | lt::alert_category::error);

        p.set_str(lt::settings_pack::string_types::dht_bootstrap_nodes,
                  "router.bittorrent.com:6881,router.utorrent.com:6881,router.bitcomet.com:6881,dht.transmissionbt.com:6881");

        p.set_str(lt::settings_pack::string_types::dht_bootstrap_nodes,
                  "192.168.0.110:6881");

        lt::session ses{p};
     
        std::unordered_map<std::string, int> map{};

        
        //magnet:?xt=urn:btih:ffcd1c04449c405dd5f4d3e427c2c708ad67304a
        for (;;)
        {
                std::vector<lt::alert *> alerts;
                ses.pop_alerts(&alerts);

                for (lt::alert *a : alerts)
                {
                        // std::cout << UTF8::GetMultiByteFromUTF8(a->message()) << std::endl;
                        //  if we receive the finished alert or an error, we're done
                       
                      
                        if (auto v = lt::alert_cast<libtorrent::dht_announce_alert>(a))
                        {
                               
                                addtorrent(ses, v->info_hash, map);
                               
                        }
                      
                        if (auto v = lt::alert_cast<libtorrent::add_torrent_alert>(a))
                        {

                             
                                Print("add tr:", getstring(v->params.info_hashes.get(lt::protocol_version::V1).to_string()));
                        }
                       
                        if (auto v = lt::alert_cast<libtorrent::dht_get_peers_alert>(a))
                        {
                              
                                addtorrent(ses, v->info_hash, map);
                               
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
                                SaveFileInfo(*info);
                                SaveTorrentFile(*info);

                                ses.remove_torrent(handle);
                        }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                ses.post_torrent_updates();

              

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