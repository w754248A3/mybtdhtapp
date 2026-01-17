#pragma once
#include "leikaifeng.h"
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
  auto path = UTF8::GetWideCharFromUTF8(data->request->GetPath());

    
    if(path.starts_with(L"/app")){
        path = data->path +path.substr(4);

        if(File::IsFileOrFolder(path).IsFile()){
          
          ResponseFunc::SendFile(path, data->connect,*(data->request.get()), false);


        }
        else{


          ResponseFunc::Send404(data->connect);

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

  auto pagestr = data->request->GetQueryValue(MYTEXT("page"));
  size_t page=0;
  Number::Parse(pagestr, page);

  int64_t count = 30;

  auto offset = count*((int64_t)page);



  auto key = data->request->GetQueryValue("key");
  auto selnew = data->request->GetQueryValue("new");
  auto selhot = data->request->GetQueryValue("hot");
  if(key != "")
  {
      Print(UTF8::GetMultiByteFromUTF8(key));
      std::string str{};
      if(data->table->SelectFromKey(key,count, offset, &str)){

        ResponseFunc::SendJsonContent(str, data->connect);
         
      }
      else{
          ResponseFunc::Send404(data->connect);
      }
  }
  else if(selnew != ""){
      std::string str{};
      if(data->table->SelectNewLine(count, offset, &str)){
          ResponseFunc::SendJsonContent(str, data->connect);
         
      }
      else{
          ResponseFunc::Send404(data->connect);
      }
  }
  else if(selhot != ""){
    std::string str{};
      if(data->table->SelectHotLine(count, offset, &str)){
          ResponseFunc::SendJsonContent(str, data->connect);
      }
      else{
          ResponseFunc::Send404(data->connect);
      }
  }
  else{
    ResponseFunc::Send404(data->connect);

    return;
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
        lis.Bind(IPEndPoint{"0.0.0.0", 8066});
        lis.Listen(1);
        Print(("ip:port===0.0.0.0:8066"));
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