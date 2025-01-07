

#ifndef SQLITE_CORE
#define SQLITE_CORE
#include "include/mysqlitevtableex.h"
#include <vector>
#include <winnt.h>
#endif

#include "leikaifeng.h"
#include "mysqliteclass.h"
#include "mysqlitevtableex.h"
 

int main(){


  SqlMy::MySqliteConnect db{"file:v3Uv0oBM?mode=memory&cache=shared", true};

  char * p;
  auto res = SqlMyEx::sqlite3_mysqlitevtableex_init(db.Get(), &p, nullptr);
  if(res != SQLITE_OK){
        Print(sqlite3_errmsg(db.Get()));
        Exit("init error");
  }
  Print("ok");


  SqlMy::MySqliteStmt stmt{db.Get(),
      R""""(
          WITH 
            subquery AS (
              SELECT 
                json_object(
                  'type', a,
                  'files',  json_group_array(
                        b
                    ) 
                ) AS json_value_1
              FROM templatevtab(?1)
              GROUP BY a
              )

            SELECT json_group_array(json(json_value_1)) FROM subquery;
          )""""

  };
  std::vector<std::pair<INT64, std::string>> vs{
    {1,"123"},
    {2,"345"},
    {1,"678"}
  };
  stmt.BindPointer(1, &vs, "std::vector");
  while (true) {
      auto res = stmt.Step();

      if(res == SqlMy::SqlStepCode::OK){
        return 0;
      }

      if(res == SqlMy::SqlStepCode::ROW ){
        Print(stmt.GetText(0));
      }
      else{
        Print(sqlite3_errmsg(db.Get()));
        Exit("step error");
      }
  }
}