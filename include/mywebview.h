#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
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

struct RequestData{

  std::shared_ptr<TcpSocket> connect;

  std::unique_ptr<HttpReqest> request;

  std::shared_ptr<SqlMy::MyWebViewSelectClass> table;


  std::wstring path;
};

bool ResponseFile(std::shared_ptr<RequestData> data){
  auto path = UTF8::GetWideChar(data->request->GetPath());

    
    if(path.starts_with(L"/app")){
        path = data->path +path.substr(4);

        if(File::IsFileOrFolder(path).IsFile()){
            
        HttpResponseFileContent response{ path };
            
            response.SetRangeFromRequest(*data->request);

            response.Send(data->connect);

        }
        else{

            HttpResponse404 res404{};
            res404.Send(data->connect);

        }



        return true;
    }
    else{
      return false;
    }
}


void Response(std::shared_ptr<RequestData> data) {

  if(ResponseFile(data)){
    return;
  }

  auto key = data->request->GetQueryValue(u8"key");

  if(key == u8"")
  {
    HttpResponse404 res404{};
    res404.Send(data->connect);

    return;
  }

    auto pagestr = data->request->GetQueryValue(u8"page");
    size_t page=0;
    Number::Parse(pagestr, page);

    int64_t count = 30;

    auto offset = count*((int64_t)page);


    Print(UTF8::GetStdOut(key));
    std::string str{};
    if(data->table->SelectFromKey(ToString(key),count, offset, &str)){
        HttpResponseStrContent connect{200, std::move(ToU8String(str)), HttpResponseStrContent::JSON_TYPE};

        connect.Send(data->connect);
    }
    else{
        HttpResponse404 res404{};
    res404.Send(data->connect);
    }
  

  

}

void RequestLoop(std::shared_ptr<RequestData> data) {

  try {
    int n = 0;

    while (true) {

      data->request = HttpReqest::Read(data->connect);

      Response(data);
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

void Func(std::shared_ptr<SqlMy::MyWebViewSelectClass> table, std::wstring path) {
  Info::Initialization();

  auto f = new Fiber{};

  f->Start(
      [](std::shared_ptr<SqlMy::MyWebViewSelectClass> table, std::wstring path) {
        TcpSocketListen lis{};
        lis.Bind(IPEndPoint{127, 0, 0, 1, 80});
        lis.Listen(1);

        while (true) {
          auto data = std::make_shared<RequestData>();

          data->connect = lis.Accept();

          data->table=table;
          data->path=path;

          Fiber::GetThis().Create(RequestLoop, data);
        }
      },
      table, path);
}
} // namespace MyWebView

#endif // !_MYWEBVIEW