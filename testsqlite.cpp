#include "include/mysqliteclass.h"
#include "leikaifeng.h"
#include "mysqliteclass.h"
#include "mybtclass.h"
#include <cstdint>
#include <functional>
#include <libtorrent/torrent_info.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>



std::vector<BtMy::Torrent_Data> GetInfoFileNames(){
  std::vector<std::string> vs{};
  for (const auto & entry : std::filesystem::directory_iterator(L"./torrent")){

    if(entry.is_directory()){
      continue;
    }
   
    auto p = UTF8::GetUTF8ToString(entry.path());
    vs.push_back(p);

  }

  std::vector<BtMy::Torrent_Data> names{};
 
  for (const auto & entry : vs){

    

    auto info = lt::torrent_info(entry);


    auto data = BtMy::GetTorrentData(info);

    names.push_back(data);
  }

  return names;
}


#ifdef FDSA
std::vector<std::string> readAllLineFromFile(){
  std::string filePath = "text.txt"; // 你的文件路径
    std::vector<std::string> lines;

    // 检查文件是否存在
    if (!std::filesystem::exists(filePath)) {
        Exit("file not find");
    }

    // 打开文件
    std::ifstream file(filePath);
    if (!file.is_open()) {
        Exit("file can not open");
    }

    // 读取文件的每一行
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    // 关闭文件
    file.close();

    return lines;
}




void TestHashTable(std::vector<std::string>& vs){
  auto db = std::make_shared<SqlMy::MySqliteConnect>(":strmasgg56hfgfg:");
 
  

  SqlMy::MyHashTable table{db};
  //std::vector<std::string> vs{"123", "234", "123", "567", "890", "123", "123", "567"};

  std::unordered_map<std::string, int> map{};


  Print("start loop");
  for (const auto& n : vs) {
    int64_t id;
   
    bool isadd;
    if(map.find(n) == map.end()){

      map.emplace(n, 0);
      isadd=true;
    }
    else{
      isadd=false;

    }

    auto isiser = table.Insert(n, &id);



    if(isiser != isadd){
      Print(n);
      Exit("not eq error");
    }

    if(isiser){
      Print("add ok", id);
    }
    else{
      Print("not add ");
    }

  }
}


void TestHashTableMain(){
  std::vector<std::string> vs{};
  for (const auto & entry : std::filesystem::directory_iterator(L"./torrent")){

    if(entry.is_directory()){
      continue;
    }
   
    auto p = UTF8::GetUTF8ToString(entry.path());
    vs.push_back(p);

  }

  std::vector<std::string> hashvs{};
 
  for (const auto & entry : vs){

    

    auto info = lt::torrent_info(entry);

    auto hash = info.info_hashes().get_best().to_string();
    std::string hashstr;
    BtMy::GetHash16String(hash, &hashstr);
    
    hashvs.push_back(hashstr);

  }


  TestHashTable(hashvs);

  
}



void TestFullTextTable(){
  auto datas = GetInfoFileNames();



  auto db = std::make_shared<SqlMy::MySqliteConnect>(":strmasgg56hfgfg:");
 
  SqlMy::MyFullTextTable table{db};

  {
    SqlMy::MyTransaction tr{db};

    int64_t index=0;
    for (const auto& item : datas) {
      
      table.Insert(index++, item.name);

      for (const auto& fileitem : item.files) {
          table.Insert(index++, fileitem.first);
      }
    }

    tr.Commit();
  }

  

  Print("starty sou");
  db->RegisterTrace();
  while (true) {
    std::string input;

    std::getline(std::cin, input);
    Print("input", input);

    auto u8 = UTF8::GetUTF8ToString(UTF8::GetWideChar(input));
    table.Sou(u8, [](int64_t id, std::string& text){
      Print(UTF8::GetMultiByteFromUTF8(text));
    });

  }

  
}
#endif


void TestFileTable(){
  auto datas = GetInfoFileNames();



  auto db = std::make_shared<SqlMy::MySqliteConnect>(":strmasgg56hfgfg:");
 
  SqlMy::MyTorrentDataTable table{db};


  {
    
   
    for (const auto& item : datas) {
      
      if(table.IsHaveHash(item.hash)==false){
        table.Insert(item);
      }

      

    }
  }
  Print("ok");
  while (true) {
    std::string input;

    std::getline(std::cin, input);
    Print("input", input);

    auto u8 = UTF8::GetUTF8ToString(UTF8::GetWideChar(input));
    table.SelectFromKey(u8, [](auto& json){
    Print(UTF8::GetMultiByteFromUTF8(json));
    });

  }

}


int main(){
 TestFileTable();

}



