#include "mywebview.h"
#include "myserverapi.h"



void StaticFileRouting(RequestResponseAPI& p, const std::wstring& folderPath){
    
    auto& path = p.GetPath();
    auto req_wpath = UTF8::GetWideCharFromUTF8(path);

    auto all_wpath = folderPath  +req_wpath.substr(4);
  
    auto is_folder_file = p.IsFileOrFolder(all_wpath);

    if(is_folder_file==1){
        p.SendFile(all_wpath,false);
    }
    else{
        p.Send404();
    }
}



bool ParseNumber(const mt::mystring& s, size_t& n){
    auto res = std::from_chars(s.data(), s.data() + s.size(), n);

    return  res.ec == std::errc{};
		
}


void APIRouting(RequestResponseAPI& p, std::shared_ptr<SqlMy::MyWebViewSelectClass> table) {

  auto pagestr = p.GetQueryValue(MYTEXT("page"));
  size_t page=0;
  ParseNumber(pagestr, page);

  int64_t count = 30;

  auto offset = count*((int64_t)page);



  auto key = p.GetQueryValue("key");
  auto selnew = p.GetQueryValue("new");
  auto selhot = p.GetQueryValue("hot");
  if(key != "")
  {
      MyWin32Out::Print(UTF8::GetWideCharFromUTF8(key));
      std::string str{};
      if(table->SelectFromKey(key,count, offset, &str)){

        p.SendJsonContent(str);

      }
      else{
          p.Send404();
      }
  }
  else if(selnew != ""){
      std::string str{};
      if(table->SelectNewLine(count, offset, &str)){
        
        p.SendJsonContent(str);

      }
      else{
          p.Send404();
      }
  }
  else if(selhot != ""){
    std::string str{};
      if(table->SelectHotLine(count, offset, &str)){
          
        p.SendJsonContent(str);

      }
      else{
          p.Send404();
      }
  }
  else{
    p.Send404();
  }
}

namespace MyWebView {

void StartServer(std::shared_ptr<SqlMy::MyWebViewSelectClass> table, std::wstring path){
    
	RunServer rs{};

    
    //静态文件路由
	rs.Routing([](RequestResponseAPI& p){
		return p.GetMethod() == mt::Method::GET && p.GetPath().starts_with(MYTEXT("/app"));
	},
	[&folderPath=path](RequestResponseAPI& p){
        StaticFileRouting(p, folderPath);
	});


	rs.Routing([]([[maybe_unused]] RequestResponseAPI& p){
		return true;
	},
	[table](RequestResponseAPI& p){

		APIRouting(p, table);
	});

	rs.Run(8066);

}

}