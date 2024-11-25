#include <libtorrent/session.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <iostream>
int main(int argc, char const* argv[])
{
        
        lt::session ses{};
        auto str = u8"magnet:?xt=urn:btih:9bbcc2df740aa2b0512dded24498a5aa618f9739&dn=MIMK-138-FHD";
        std::string strstr{reinterpret_cast<const char*>(str)};

        lt::add_torrent_params atp = lt::parse_magnet_uri(strstr);
        boost::asio::ip::address_v4 ip_v4 = boost::asio::ip::address_v4::from_string(reinterpret_cast<const char*>(u8"192.168.0.110"));
        boost::asio::ip::address ip{ip_v4};



        atp.peers.push_back(libtorrent::tcp::endpoint{ip, 6881});

        atp.save_path = "."; // save in current dir
        lt::torrent_handle h = ses.add_torrent(atp);
       
        // ...

        std::cout << "ok" << std::endl;

        int n;

        std::cin >> n;
        
}