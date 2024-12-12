#pragma once
#ifndef _MYBTCLASS
#define _MYBTCLASS

#include <string>
#include <charconv>
#include <cstddef>
#include <algorithm>



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



    

}

#endif // !_MYBTCLASS