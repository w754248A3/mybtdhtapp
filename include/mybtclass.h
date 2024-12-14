#pragma once
#include <cstdint>
#include <utility>
#include <vector>
#ifndef _MYBTCLASS
#define _MYBTCLASS

#include <string>
#include <charconv>
#include <cstddef>
#include <algorithm>
#include <libtorrent/torrent_info.hpp>

namespace BtMy
{

    bool GetHash16String(const std::string &s, std::string *out)
    {
        auto index = out->size();

        for (auto i : s)
        {
            char addressStr[2] = {0};
            auto res = std::to_chars(std::begin(addressStr), std::end(addressStr), (unsigned char)i, 16);

            auto length = (size_t)(res.ptr - addressStr);

            if (length == 1)
            {

                addressStr[1] = addressStr[0];

                addressStr[0] = '0';

                length = 2;
            }

            out->append(addressStr, length);
        }

        auto p = (out->begin()) + (long long)index;

        std::transform(p, out->end(), p, ::toupper);

        return true;
    }

    void GetHashFromInfo(const lt::torrent_info& info, std::string* out){
        auto& hash = info.info_hashes();

        if(hash.has_v1()){
            GetHash16String(hash.v1.to_string(), out);
        }
        else{
            GetHash16String(hash.v2.to_string(), out);
        }
    }

    struct Torrent_Data{
        std::string hash;
        std::string name;
        int64_t size;
        std::vector<std::pair<std::string, int64_t>> files;
    };

    Torrent_Data GetTorrentData(const lt::torrent_info& info){
        Torrent_Data data{};

        GetHashFromInfo(info, &data.hash);

        auto& name = info.name();

        auto fileCount = info.num_files();
        auto total_size =info.total_size();

        auto& filestorm = info.orig_files();
        
        data.name=name;
        data.size= total_size;

        for (auto const &n  : filestorm.file_range())
        {
                auto filepath = filestorm.file_path(n);
                auto fileSize = filestorm.file_size(n);

                data.files.emplace_back(filepath, fileSize);
        }

        return data;
    }   
    

}

#endif // !_MYBTCLASS