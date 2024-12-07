#define _WIN32_WINNT _WIN32_WINNT_WIN8
#define NTDDI_VERSION NTDDI_WIN8

#include <libtorrent/session.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/alert_types.hpp>
#include <iostream>
#include <format>
#include "leikaifeng.h"


std::string getstring(const std::string& s){
        std::string res{};
        char buf[]="0123456789ABCDEFD";

        for (auto  i : s)
        {       
                std::string hexString = std::format("{:02X}", i);

                res.append(hexString);
        }
        

        return res;
}


boost::asio::ip::address createaddress(const char* str){

         boost::asio::ip::address_v4 ip_v4 = boost::asio::ip::address_v4::from_string(str);
        boost::asio::ip::address ip{ip_v4};

        return ip;
}

void f(std::string url){
        lt::settings_pack p;
        p.set_int(lt::settings_pack::alert_mask, lt::alert_category::status
        |lt::alert_category::dht
        |lt::alert_category::dht_operation
        |lt::alert_category::ip_block
                | lt::alert_category::error);


       p.set_str(lt::settings_pack::string_types::dht_bootstrap_nodes,
       "router.bittorrent.com:6881,router.utorrent.com:6881,router.bitcomet.com:6881,dht.transmissionbt.com:6881");


        lt::session ses{p};
        lt::ip_filter filter{};
        filter.add_rule(createaddress("0.0.0.0"),  createaddress("192.168.0.109"),  lt::ip_filter::blocked);
        filter.add_rule(createaddress("192.168.0.111"),  createaddress("255.255.255.255"),  lt::ip_filter::blocked);
       
        //ses.set_ip_filter(filter);




        lt::add_torrent_params atp = lt::parse_magnet_uri(url);
        
        boost::asio::ip::address ip = createaddress("192.168.0.110");



        atp.peers.push_back(libtorrent::tcp::endpoint{ip, 6881});

        atp.save_path = "D:/mybtdownload/"; // save in current dir
        lt::torrent_handle h = ses.add_torrent(atp);
        bool isover = false;
        for (;;) {
                std::vector<lt::alert*> alerts;
                ses.pop_alerts(&alerts);

                for (lt::alert const* a : alerts) {
                std::cout << UTF8::GetMultiByteFromUTF8(a->message()) << std::endl;
                // if we receive the finished alert or an error, we're done
                if(lt::alert_cast<lt::peer_blocked_alert>(a)){
                        std::cout << "blocked ip" << std::endl;
                }
                auto type = a->type();
                if(type == libtorrent::dht_announce_alert::alert_type){
                        auto v = (libtorrent::dht_announce_alert*)a;

                        Print("pansk url :", getstring(v->info_hash.to_string()));

                }

                if(type == libtorrent::dht_get_peers_alert::alert_type){
                        auto v = (libtorrent::dht_get_peers_alert*)a;

                        Print("get url :", getstring(v->info_hash.to_string()));

                }

                if(auto statealert = lt::alert_cast<lt::state_update_alert>(a)){
                       
                       for (auto&  statu: statealert->status)
                       {
                               Print("needCount:", statu.total, 
                               " download:", statu.total_payload_download);

                       }
                       
                }

                if (lt::alert_cast<lt::torrent_finished_alert>(a)) {
                        isover=true;
                }
                if (lt::alert_cast<lt::torrent_error_alert>(a)) {
                        isover=true;
                }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                ses.post_torrent_updates();
              
                if(isover){
                        std::cout << "done, shutting down" << std::endl;

                }
        }
        
        

}
int main(int argc, char const* argv[])
{
        if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <magnet-url>" << std::endl;
        return 1;
        }
        auto wpath = UTF8::GetWideChar(argv[1]);

        auto u8 = UTF8::GetUTF8(wpath);
       try{
                
                f(std::string{reinterpret_cast<char*>(u8.data()), u8.size()});

       }
       catch (std::exception& e)
        {
        std::cerr << "Error: " << e.what() << std::endl;
        }

}