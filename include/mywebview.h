#pragma once
#include <algorithm>
#include <memory>
#include <string>
#ifndef _MYWEBVIEW
#define _MYWEBVIEW

#include "myio.h"
#include "mysqliteclass.h"
namespace MyWebView {

std::string ToString(const std::u8string& s){
    return std::string{(const char*)s.data(), s.size()};
}

std::u8string ToU8String(const std::string& s){
    return std::u8string{(const char8_t*)s.data(), s.size()};
}

void Response(std::shared_ptr<TcpSocket> con, std::unique_ptr<HttpReqest> request, std::shared_ptr<SqlMy::MyTorrentDataTable> table) {

  auto key = request->GetQueryValue(u8"key");

  if(key == u8"")
  {
    HttpResponse404 res404{};
    res404.Send(con);

    return;
  }
    Print(UTF8::GetStdOut(key));
    std::string str{};
    if(table->SelectFromKey(ToString(key), &str)){
        HttpResponseStrContent connect{200, std::move(ToU8String(str))};

        connect.Send(con);
    }
    else{
        HttpResponse404 res404{};
    res404.Send(con);
    }
  

  

}

void RequestLoop(std::shared_ptr<TcpSocket> con, std::shared_ptr<SqlMy::MyTorrentDataTable> table) {

  try {
    int n = 0;

    while (true) {

      auto request = HttpReqest::Read(con);

      Response(con, std::move(request), table);
      n++;

      Print(n, "re use link");
    }
  } catch (Win32SysteamException &e) {
    Print(e.what());
  } catch (HttpReqest::FormatException &e) {
    Print("request format error:", e.what());
  } catch (::SystemException &e) {
    Print("SystemException :", e.what());
  }
}

void Func(std::shared_ptr<SqlMy::MyTorrentDataTable> table) {
  Info::Initialization();

  auto f = new Fiber{};

  f->Start(
      [](std::shared_ptr<SqlMy::MyTorrentDataTable> table) {
        TcpSocketListen lis{};
        lis.Bind(IPEndPoint{127, 0, 0, 1, 80});
        lis.Listen(1);

        while (true) {
          auto connect = lis.Accept();

          Fiber::GetThis().Create(RequestLoop, connect, table);
        }
      },
      table);
}
} // namespace MyWebView

#endif // !_MYWEBVIEW