#include "mysqliteclass.h"



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


int main2(){

    MySqliteConnect db{":strmasgg56hfgfg:"};

    std::string sql {R""""(
    CREATE TABLE IF NOT EXISTS hash_table (
    id INTEGER PRIMARY KEY,
    hash TEXT NOT NULL,
    length INTEGER NOT NULL,
    name TEXT NOT NULL
      );)""""};
    
    MySqliteStmt stmt{db.Get(), sql};

    stmt.Step();




    std::string sqlinset{R""""(
          INSERT INTO hash_table (hash, length, name) VALUES (?1, ?2, ?3) RETURNING id;
        )""""};

  MySqliteStmt stmt2{db.Get(), sqlinset};


  std::vector<std::string> vs{"456", "dfgg", "678"};


  std::string en{"22222"};


  for ( auto& n : vs)
  {
 /*    
    stmt2.BindText(1, n);

    stmt2.BindInt64(2, 11011);
    stmt2.BindText(3, en);

 */   
     std::string en{"22222"};
    stmt2.Bind<1>(n, (int64_t)10010, en);
   stmt2.Inset([](MySqliteStmt& v){

      auto n = v.GetInt64(0);

      Print("key,", n);

    });

  }


  MySqliteStmt sel{db.Get(), "SELECT * FROM hash_table;"};


  while (sel.Step())
  {
    /* 
     sel.Get([&en = en](int64_t id, std::string& hash, int64_t length, std::string& name){

          Print(id, hash, length, name, en);
     });
 */
    int64_t id;
    std::string hash;
    int64_t length;
    std::string name;


    sel.Get2<0>(&id, &hash, &length, &name);

    Print(id, hash, length, name, en);

  }
  
  


   
  Print("ok");

  return 0;
}



int main(){

  MySqliteConnect db{":strmasgg56hfgfg:"};
  
  auto papi = MySqliteStmt::GetFts5ApiP(db.Get());

  

  MySqliteTokenizers::RegisterTokenizer(papi, "mytokenizer");


    std::string sql {R""""(
    CREATE VIRTUAL TABLE ft111 USING fts5(text, tokenize="mytokenizer");
    )""""};


    
    MySqliteStmt stmt{db.Get(), sql};

    stmt.Step();

    std::string sqlinset{R""""(
          INSERT INTO ft111 (text) VALUES (?1);
        )""""};

    MySqliteStmt stmt2{db.Get(), sqlinset};



    auto vs = readAllLineFromFile();

    for ( auto& n : vs)
    {
  
      stmt2.Bind<1>(n);
      

      stmt2.Inset();

    }



  

  while(true){
    MySqliteStmt sel{db.Get(), "SELECT text FROM ft111 WHERE ft111 MATCH ?1;"};
    std::string input;

    std::getline(std::cin, input);
    Print("input", input);
      auto u8 = UTF8::GetUTF8(UTF8::GetWideChar(input));

      std::string que{(const char*)u8.data(), u8.size()};
      sel.Bind<1>(que);
  while (sel.Step())
  {
    /* 
     sel.Get([&en = en](int64_t id, std::string& hash, int64_t length, std::string& name){

          Print(id, hash, length, name, en);
     });
 */

    std::string name;


    sel.Get2<0>(&name);

    Print(UTF8::GetMultiByteFromUTF8(name));

  }
  
  }

  
  


}