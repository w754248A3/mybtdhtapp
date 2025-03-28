#pragma once

#include "mybtclass.h"
#include <algorithm>
#include <cstddef>
#include <format>
#include <functional>
#include <thread>
#include <type_traits>
#include <wininet.h>
#ifndef _MYSQLITECLASS
#define _MYSQLITECLASS


#include <sqlite3.h>
#include <stdio.h>
#include <iostream>
#include <cstdint>
#include "leikaifeng.h"
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <libtorrent/aux_/path.hpp>
#include <memory>

namespace SqlMy
{

  enum class SqlStepCode: int{
    OK = SQLITE_OK,
    STEP_ERROR=SQLITE_ERROR,
    ROW=SQLITE_ROW,
    CONSTRAINT_UNIQUE = SQLITE_CONSTRAINT_UNIQUE,
    CONSTRAINT = SQLITE_CONSTRAINT,
    DONE= SQLITE_DONE,
    BUSY= SQLITE_BUSY,

  };

  class MySqliteStmt
  {

    sqlite3_stmt *m_stmt;
    sqlite3 *m_db;
    int m_up_step_code;

  public:
    MySqliteStmt(sqlite3 *db, const std::string &sql, bool is_long_use = false) : m_db(db)
    {
      Print(sql);
      const char *notUse = NULL;
     
      auto res = sqlite3_prepare_v3(
          m_db,
          sql.data(),
          static_cast<int>(sql.size()),
          is_long_use ? SQLITE_PREPARE_PERSISTENT  :0,
          &m_stmt,
          &notUse);

      if (res != SQLITE_OK)
      {

        Print(sqlite3_errmsg(m_db));
        Exit("prepare stm error");
      }
    }

    template<typename T>
    SqlStepCode Step(const std::string& meg, T&& is_return_code_func) requires (std::is_invocable_r_v<bool, T, SqlStepCode>)
    {
      auto res = (SqlStepCode)sqlite3_step(m_stmt);
      m_up_step_code=(int)res;

      if(is_return_code_func(res)){
        return res;
      }

      if (res == SqlStepCode::ROW)
      {
        return SqlStepCode::ROW;
      }


      if (res != SqlStepCode::DONE && res != SqlStepCode::OK)
      {
        
        if(res ==SqlStepCode::CONSTRAINT_UNIQUE 
        || res == SqlStepCode::CONSTRAINT){
          return res;
        }
        auto excode = sqlite3_extended_errcode(m_db);
       
        Print("code", (int)res, "excode:", excode, "error meg", sqlite3_errmsg(m_db), "call meg", meg);
        Exit("step stm error");
      }

      return SqlStepCode::OK;
    }

    
    SqlStepCode Step(const std::string& meg){
     
      return Step(meg, [](SqlStepCode p) -> bool {return false;});
    }

     SqlStepCode Step(const std::string& meg, bool is_errorCode_exit){
      if(is_errorCode_exit){
        return Step(meg, [](SqlStepCode p) -> bool {return false;});
      }
      else{
        return Step(meg,[](SqlStepCode p) -> bool {return p == SqlStepCode::STEP_ERROR;});
      }
      
    }

    void ClaerBind()
    {
      auto res = sqlite3_clear_bindings(m_stmt);

      if (res != SQLITE_OK)
      {
        Print(sqlite3_errmsg(m_db));
        Exit("ClaerBind stm error");
      }
    }

    std::string GetBindExpandedSql(){
      return  GetBindExpandedSql(m_stmt);
    }

    static std::string GetBindExpandedSql(sqlite3_stmt* stmt){
      auto p = sqlite3_expanded_sql(stmt);

      if(p == NULL){
        
        Exit("GetBindExpandedSql stm error");
      }
     
      std::string sql{p};
      sqlite3_free(p);

      return p;
    }

   
    int64_t GetInt64(int index)
    {
      return sqlite3_column_int64(m_stmt, index);
    }

    std::string GetText(int index)
    {

      auto res = sqlite3_column_text(m_stmt, index);
      if (res == NULL)
      {
        Print(sqlite3_errmsg(m_db));
        Exit("get text error return null");
      }

      return std::string{(const char *)res};
    }

    
    void Reset()
    {
      auto uperrorcode =  m_up_step_code;

      auto res = sqlite3_reset(m_stmt);

      if (res != SQLITE_OK && res != uperrorcode)
      {
        Print(sqlite3_errmsg(m_db));
        Exit("Reset stm error");
      }
    }

    void BindText(int index, const std::string &s)
    {
      auto res = sqlite3_bind_text(m_stmt, index, s.data(), (int)s.size(), SQLITE_STATIC);

      if (res != SQLITE_OK)
      {
        Print(sqlite3_errmsg(m_db));
        Exit("bind stm Text error");
      }
    }

    void BindInt64(int index, int64_t v)
    {
      auto res = sqlite3_bind_int64(m_stmt, index, v);

      if (res != SQLITE_OK)
      {
        Print(sqlite3_errmsg(m_db));
        Exit("bind stm Int64 error");
      }
    }

    void BindPointer(int index, void *p, const char* pointerTypeStr)
    {
      auto res = sqlite3_bind_pointer(m_stmt, index, p, pointerTypeStr, NULL);

      if (res != SQLITE_OK)
      {
        Print(sqlite3_errmsg(m_db));
        Exit("bind stm Pointer error");
      }
    }

    static fts5_api *GetFts5ApiP(sqlite3 *db)
    {
      MySqliteStmt stmt{db, "SELECT fts5(?1)"};
      fts5_api *p;
      stmt.BindPointer(1, &p, "fts5_api_ptr");

      stmt.Step("GetFts5ApiP");

      if (p == NULL)
      {
        Exit("get fts5 api is null");
      }

      return p;
    }

    

    ~MySqliteStmt()
    {

      sqlite3_finalize(m_stmt);
    }
  };

  class MySqliteConnect
  {

    sqlite3 *m_db;

  public:
    MySqliteConnect(const std::string &path)
    {
      auto flags = SQLITE_OPEN_NOMUTEX |   SQLITE_OPEN_READWRITE| SQLITE_OPEN_CREATE| SQLITE_OPEN_URI;

      auto res = sqlite3_open_v2(path.c_str(),
                                 &m_db, flags, NULL);

      if (res != SQLITE_OK)
      {

        Print(sqlite3_errmsg(m_db));
        Exit("open sqlite db error");
      }
    }

    sqlite3 *Get()
    {
      return m_db;
    }
    void Attach(const std::string& path, const std::string& name){
      auto command = std::format("ATTACH DATABASE '{0}' AS {1};", path, name);
    
      MySqliteStmt stmt{this->Get(), command};


      if (stmt.Step("Attach error") != SqlStepCode::OK){
         Exit("Attach error");
      }




    }

    void SetBusyTimeout(int milliseconds){
      
        auto res = sqlite3_busy_timeout(m_db, milliseconds);

        if(res != SQLITE_OK){
          Print(res, sqlite3_errmsg(m_db));
          Exit("SetBusyTimeout error");
        }
    }

    void RegisterTrace(){
     

      auto res = sqlite3_trace_v2(m_db, SQLITE_TRACE_STMT, 
      [](unsigned int flags, void* p, void* p2, void* p3){

        Print("flags:", flags, "sql:", MySqliteStmt::GetBindExpandedSql((sqlite3_stmt*)p2));
        return 0;
      }, NULL);


      if (res != SQLITE_OK)
      {
        Print(sqlite3_errmsg(m_db));
        Exit("RegisterTrace error");
      }

    }

    void SetConnectConfig(int v){
      auto res = sqlite3_db_config(m_db, v);

      if(res != SQLITE_OK){
        Print(sqlite3_errmsg(m_db));
        Exit("SetConnectConfig error");
      }
    }

    ~MySqliteConnect()
    {

      auto res = sqlite3_close(m_db);

      if (res != SQLITE_OK)
      {
        Exit("can not close sqlite db");
      }
    }
  };

  namespace MySqliteTokenizers
  {

    bool findFirstUtf8Char(const char *str, size_t offset, size_t size,
                           size_t *findFirstCharSize, const char **errorMsg)
    {
      if (str == nullptr || offset >= size || findFirstCharSize == nullptr)
      {
        if (errorMsg)
        {
          *errorMsg = "Invalid input parameters.";
        }
        return false;
      }

      const unsigned char *utf8_str = reinterpret_cast<const unsigned char *>(str);

      size_t i = offset;
      if (i >= size)
      {
        if (errorMsg)
        {
          *errorMsg = "Offset is out of bounds.";
        }
        return false;
      }

      unsigned char first_byte = utf8_str[i];
      size_t char_size = 0;

      // Check the number of bytes for the UTF-8 character
      if ((first_byte & 0x80) == 0)
      { // 1-byte character (ASCII)
        char_size = 1;
      }
      else if ((first_byte & 0xE0) == 0xC0)
      { // 2-byte character
        char_size = 2;
      }
      else if ((first_byte & 0xF0) == 0xE0)
      { // 3-byte character
        char_size = 3;
      }
      else if ((first_byte & 0xF8) == 0xF0)
      { // 4-byte character
        char_size = 4;
      }
      else
      {
        if (errorMsg)
        {
          *errorMsg = "Invalid UTF-8 start byte.";
        }
        return false;
      }

      // Check if there are enough bytes in the string to form a valid UTF-8 character
      if (i + char_size > size)
      {
        if (errorMsg)
        {
          *errorMsg = "Not enough bytes to complete the UTF-8 character.";
        }
        return false;
      }

      // Check for continuation bytes
      for (size_t j = 1; j < char_size; ++j)
      {
        if ((utf8_str[i + j] & 0xC0) != 0x80)
        {
          if (errorMsg)
          {
            *errorMsg = "Invalid UTF-8 continuation byte.";
          }
          return false;
        }
      }

      // Successfully found a valid UTF-8 character
      *findFirstCharSize = char_size;
      return true;
    }

    int CreateTokenizer(void *, const char **azArg, int nArg, Fts5Tokenizer **ppOut)
    {
      *ppOut = (Fts5Tokenizer *)(new int64_t());

      return SQLITE_OK;
    }

    void DeleteTokenizer(Fts5Tokenizer *p)
    {
      delete ((int64_t *)p);
    }

    int TokenizeTokenizer(Fts5Tokenizer *,
                          void *pCtx,
                          int flags, /* FTS5_TOKENIZE_* 标志的掩码 */
                          const char *pText, int nText,
                          // const char *pLocale, int nLocale,
                          int (*xToken)(
                              void *pCtx,         /* xTokenize() 的第二个参数的副本 */
                              int tflags,         /* FTS5_TOKEN_* 标志的掩码 */
                              const char *pToken, /* 指向包含标记的缓冲区的指针 */
                              int nToken,         /* 标记的大小（以字节为单位） */
                              int iStart,         /* 输入文本中标记的字节偏移量 */
                              int iEnd            /* 输入文本中标记末尾的字节偏移量 */
                              ))
    {

      size_t offset = 0;
      while (offset < (size_t)nText)
      {
        size_t count;

        const char *error;
        auto res = findFirstUtf8Char(pText, offset, (size_t)nText, &count, &error);

        if (res)
        {
          auto pToken = pText + offset;
          auto nToken = count;
          auto iStart = offset;

          auto iEnd = offset + count;

          offset += count;



          if (xToken(pCtx, 0, pToken, (int)nToken, (int)iStart, (int)iEnd) != SQLITE_OK)
          {
            Exit("xToken call not ok error");
          }

          char c = pToken[0];
          if(nToken==1 && c >= 'a' && c<= 'z'){
            char c_offset = c-'a';
            c = 'A'+c_offset;

            if (xToken(pCtx, FTS5_TOKEN_COLOCATED, &c, (int)nToken, (int)iStart, (int)iEnd) != SQLITE_OK)
            {
              Exit("xToken COLOCATED call not ok error");
            }
          }
        }
        else
        {

          Print(error, nText, offset);
          Exit("Tokenize find utf8 char error");
        }
      }

      return SQLITE_OK;
    }

    void RegisterTokenizer(fts5_api *papi, const std::string &name)
    {
      fts5_tokenizer v{};

      v.xCreate = CreateTokenizer;

      v.xDelete = DeleteTokenizer;

      v.xTokenize = TokenizeTokenizer;

      auto res = papi->xCreateTokenizer(papi, name.data(), NULL, &v, NULL);

      if (res != SQLITE_OK)
      {
        Exit("RegigTokenizer error");
      }
    }

  }

  void CreateTable(std::shared_ptr<MySqliteConnect> db, const std::string& sql){
    MySqliteStmt stmt{db->Get(), sql};
   
    if(stmt.Step("CreateTable")!= SqlStepCode::OK){
      Exit("create table error");
    }

   
  }

  class MyTransactionStmt{

    std::unique_ptr<MySqliteStmt> m_commit;

    std::unique_ptr<MySqliteStmt> m_rollback;

    std::unique_ptr<MySqliteStmt> m_begin;

  public:

    MyTransactionStmt(std::shared_ptr<MySqliteConnect> db){

      m_begin = std::make_unique<MySqliteStmt>(db->Get(), "BEGIN IMMEDIATE;", true);

      m_commit= std::make_unique<MySqliteStmt>(db->Get(), "COMMIT;", true);

      m_rollback= std::make_unique<MySqliteStmt>(db->Get(), "ROLLBACK;", true);
    }
   
    bool Begin(){
      //貌似非bing参数的语句不需要Reset;

      //m_begin->Reset();

      //m_commit->Reset();

      //m_rollback->Reset();
      auto res = m_begin->Step("Begin", [](auto p)-> bool {return p == SqlStepCode::BUSY;});

      if(res == SqlStepCode::BUSY){
        return false;
      }
      else if(res == SqlStepCode::OK){
        return true;
      }
      else{
            Exit("start Transaction error");
            return false;
      }
    }

    void Commit(){
        if(m_commit->Step("Commit") != SqlStepCode::OK){
            Exit("commit Transaction error");
        }
    }


    void Rollback(){
      if(m_rollback->Step("Rollback") != SqlStepCode::OK){
          Exit("rollback Transaction error");
        }
    }

    bool BeginLoop(){
      
        for (int n =0; n<1000; n++) {
          if(this->Begin()){
            
            return true;
          }
          else{
            std::this_thread::yield();
          }
        }

        return false;
    }
    
    bool RunOnTransaction(std::function<bool()> func){
        if(!this->BeginLoop()){
          Exit("can not start Begin");
        }
      
        if(func()){
          this->Commit();
          return true;
        }
        else{
          this->Rollback();
          return false;
        }

    }

  };

  class MyHashCountTable{

    std::shared_ptr<MySqliteConnect> m_db;
    std::unique_ptr<MySqliteStmt> m_inset;
  
    public:
      MyHashCountTable(std::shared_ptr<MySqliteConnect> db): m_db(db){

         SqlMy::CreateTable(db,
          R""""(
            CREATE TABLE IF NOT EXISTS mymdb567.hash_count_table (
            id INTEGER PRIMARY KEY,
            hash_value TEXT NOT NULL,   
            count INTEGER NOT NULL DEFAULT 1,    
            UNIQUE(hash_value)                   
              );
          )"""");

      
         m_inset = std::make_unique<MySqliteStmt>(db->Get(), R""""(
            INSERT INTO mymdb567.hash_count_table (hash_value) VALUES (?1)
            ON CONFLICT(hash_value) DO UPDATE SET count=count+1
            RETURNING count;
          )"""", true);

        
      }

     bool Insert(const std::string& s, int64_t* pcount){
       
          m_inset->Reset();
          m_inset->BindText(1, s);
        
          auto res = m_inset->Step("hash count Insert 1");
       
          if(res == SqlStepCode::ROW){
            
            *pcount = m_inset->GetInt64(0);
          
            return m_inset->Step("hash count Insert 2") == SqlStepCode::OK;
          }
          else{
            return false;
          }


     }

  };


  class MyHashTable{

    std::shared_ptr<MySqliteConnect> m_db;
    std::unique_ptr<MySqliteStmt> m_inset;
    std::unique_ptr<MySqliteStmt> m_is_have_hash;
    public:
      MyHashTable(std::shared_ptr<MySqliteConnect> db): m_db(db){

         SqlMy::CreateTable(db,
          R""""(
            CREATE TABLE IF NOT EXISTS hash_table (
            id INTEGER PRIMARY KEY,
            hash_value TEXT NOT NULL,           
            UNIQUE(hash_value)                   
              );
          )"""");

      
         m_inset = std::make_unique<MySqliteStmt>(db->Get(), R""""(
            INSERT INTO hash_table (hash_value) VALUES (?1) RETURNING id;
          )"""", true);

        m_is_have_hash =  std::make_unique<MySqliteStmt>(db->Get(), R""""(
            SELECT id FROM hash_table
            WHERE hash_value = ?1
            LIMIT 1;
          )"""", true);

        
      }

      bool IsHaveHash(const std::string& s){

          m_is_have_hash->Reset();

          m_is_have_hash->BindText(1, s);

          auto res = m_is_have_hash->Step("IsHaveHash");

          if(res == SqlStepCode::ROW){
            return true;
          }
          else if (res == SqlStepCode::OK) {
            return false;
          
          }
          else{
            Exit("HaveHash value result other code");
            return false;
          }

          
      }

     bool Insert(const std::string& s, int64_t* pid){
          m_inset->Reset();
          m_inset->BindText(1, s);
        
          auto res = m_inset->Step("hash Insert 1");
       
          if(res == SqlStepCode::ROW){
            
            *pid = m_inset->GetInt64(0);
          
            return m_inset->Step("hash Insert 1") == SqlStepCode::OK;
          }
          else{
            return false;
          }


     }

  };



  class MyFullTextTable{
   
    std::shared_ptr<MySqliteConnect> m_db;
    std::unique_ptr<MySqliteStmt> m_inset;
    public:
      MyFullTextTable(std::shared_ptr<MySqliteConnect> db): m_db(db){

        auto papi = MySqliteStmt::GetFts5ApiP(db->Get());

  

        MySqliteTokenizers::RegisterTokenizer(papi, "mytokenizer");



         SqlMy::CreateTable(db,
          R""""(
            CREATE VIRTUAL TABLE IF NOT EXISTS
             fulltext_table USING fts5(text, tokenize="mytokenizer", content='', columnsize=0);               
          )"""");

      
         m_inset = std::make_unique<MySqliteStmt>(db->Get(), R""""(
            INSERT INTO fulltext_table (rowid, text) VALUES (?1, ?2);
          )"""", true);
        
      }


      bool Insert(int64_t id, const std::string& s){
          m_inset->Reset();

          m_inset->BindInt64(1, id);
          m_inset->BindText(2, s);
        
          auto res = m_inset->Step("full text Insert");
          bool isok= res == SqlStepCode::OK;
         
         

         return isok;
     }


     void Sou(const std::string& s, std::function<void(int64_t, std::string&)> func){
        MySqliteStmt stmt{m_db->Get(),R""""(
            SELECT rowid, text FROM fulltext_table
            WHERE fulltext_table MATCH ?1 
            ORDER BY rank
            LIMIT 30;
          )""""};
        stmt.BindText(1, s);
       
        while (stmt.Step("sou") == SqlStepCode::ROW) {
          auto id = stmt.GetInt64(0);
          auto text = stmt.GetText(1);

          func(id, text);
        }

        
     }

  };


  class MyTorrentFileTable{
   
      std::shared_ptr<MySqliteConnect> m_db;
      std::unique_ptr<MySqliteStmt> m_inset;
    public:
       MyTorrentFileTable(std::shared_ptr<MySqliteConnect> db): m_db(db){


         
         SqlMy::CreateTable(db,
          R""""(
           CREATE TABLE IF NOT EXISTS file_table (
            id INTEGER PRIMARY KEY,
            hash_id INTEGER NOT NULL,   
            name TEXT NOT NULL,   
            size INTEGER NOT NULL
            );
          )"""");

          {
            MySqliteStmt stmt{db->Get(), R""""(
                    CREATE INDEX IF NOT EXISTS count_index_name ON file_table (hash_id);
              )""""};

              if(stmt.Step("create index on file_table error") != SqlStepCode::OK){
                Exit("create index on file_table error");
              }
          }

      
         m_inset = std::make_unique<MySqliteStmt>(db->Get(), 
         R""""(
            INSERT INTO file_table (hash_id, name, size) VALUES (?1, ?2, ?3) RETURNING id;
          )"""", true);
        
        
      }

      void SelectNewLine(std::function<void(int64_t, int64_t, std::string&, int64_t)> func){
        MySqliteStmt stmt{m_db->Get(), 
        R""""(
            SELECT id, hash_id, name, size FROM file_table
            ORDER BY id DESC
            LIMIT 300;
          )""""};


          while (stmt.Step("SelectNewLine while")== SqlStepCode::ROW) {
            auto id = stmt.GetInt64(0);
            auto hash_id = stmt.GetInt64(1);
            auto name = stmt.GetText(2);
            auto size = stmt.GetInt64(3);

            func(id, hash_id, name, size);
          }
      }

      bool Insert(int64_t hash_id,  const std::string& name, int64_t size, int64_t* pid){
          m_inset->Reset();

          m_inset->BindInt64(1,hash_id);
          m_inset->BindText(2,name);
          m_inset->BindInt64(3,size);
        
          auto res = m_inset->Step("torrent insert 1");
       
          if(res == SqlStepCode::ROW){
            
            *pid = m_inset->GetInt64(0);
          
            return m_inset->Step("torrent insert 2") == SqlStepCode::OK;
          }
          else{
            return false;
          }

     }

  };

  class MyWebViewSelectClass{

    std::shared_ptr<MySqliteConnect> m_db;
    std::unique_ptr<MySqliteStmt> m_select_from_key;
    std::unique_ptr<MySqliteStmt> m_select_from_new;
    std::unique_ptr<MySqliteStmt> m_select_from_hot;
  
    public:
      MyWebViewSelectClass(std::shared_ptr<MySqliteConnect> db): m_db(db){


        auto papi = MySqliteStmt::GetFts5ApiP(db->Get());

  

        MySqliteTokenizers::RegisterTokenizer(papi, "mytokenizer");


          m_select_from_key = std::make_unique<MySqliteStmt>(m_db->Get(), R""""(
            WITH textquery AS (
              SELECT rowid, rank  FROM fulltext_table
              WHERE fulltext_table MATCH ?1  
            ),
            filegroupquery AS (
              SELECT ft.hash_id, min(tq.rank) AS rank FROM file_table AS ft
              JOIN textquery AS tq
              ON tq.rowid = ft.id 
              GROUP BY ft.hash_id      
            ),
            fullfilequery AS(
              SELECT ft.hash_id, ft.name, ft.size, fg.rank  FROM file_table AS ft
              JOIN filegroupquery AS fg
              ON fg.hash_id = ft.hash_id
            ),
            subquery AS (
              SELECT 
                json_object(
                  'hash_value', ht.hash_value,
                  'files',  json_group_array(
                        json_object(
                            'name', f.name,
                            'size', f.size
                        )
                    ) 
                ) AS json_value_1
              FROM hash_table AS ht
              JOIN fullfilequery AS f
                  ON ht.id = f.hash_id
              GROUP BY ht.hash_value
              ORDER BY f.rank
              LIMIT ?2 OFFSET ?3
          )
          SELECT json_group_array(json(json_value_1)) AS json_array
          FROM subquery;
          )"""", true); 


          m_select_from_new = std::make_unique<MySqliteStmt>(m_db->Get(), R""""(
            WITH 
            subquery AS (
              SELECT 
                json_object(
                  'hash_value', ht.hash_value,
                  'files',  json_group_array(
                        json_object(
                            'name', f.name,
                            'size', f.size
                        )
                    ) 
                ) AS json_value_1
              FROM hash_table AS ht
              JOIN file_table AS f
                  ON ht.id = f.hash_id
              GROUP BY ht.hash_value
              ORDER BY f.id DESC
              LIMIT ?1 OFFSET ?2
          )
          SELECT json_group_array(json(json_value_1)) AS json_array
          FROM subquery;
          )"""", true); 


          m_select_from_hot = std::make_unique<MySqliteStmt>(m_db->Get(), R""""(
            WITH 
            hot_subquery AS (
              SELECT ht.id, ht.hash_value, hct.count
              FROM mymdb567.hash_count_table AS hct
              JOIN hash_table AS ht
                ON ht.hash_value = hct.hash_value
            ),
            subquery AS (
              SELECT 
                json_object(
                  'hash_value', ht.hash_value,
                  'count', ht.count,
                  'files',  json_group_array(
                        json_object(
                            'name', f.name,
                            'size', f.size
                        )
                    ) 
                ) AS json_value_1
              FROM hot_subquery AS ht
              JOIN file_table AS f
                  ON ht.id = f.hash_id
              GROUP BY ht.hash_value
              ORDER BY ht.count DESC
              LIMIT ?1 OFFSET ?2
          )
          SELECT json_group_array(json(json_value_1)) AS json_array
          FROM subquery;
          )"""", true); 
      }

    bool SelectHotLine(int64_t count, int64_t offset, std::string* str){
          auto& stmt = *m_select_from_hot;

          stmt.Reset();

          stmt.BindInt64(1, count);
          stmt.BindInt64(2, offset);

          if(stmt.Step("web view SelectHotLine 1", false)== SqlStepCode::ROW){
            *str = std::move(stmt.GetText(0));

            return stmt.Step("web view SelectHotLine 2")== SqlStepCode::OK;
          }
          else{
            return false;
          }
    }

    bool SelectNewLine(int64_t count, int64_t offset, std::string* str){


          auto& stmt = *m_select_from_new;

          stmt.Reset();

          stmt.BindInt64(1, count);
          stmt.BindInt64(2, offset);

          if(stmt.Step("web view SelectNewLine 1", false)== SqlStepCode::ROW){
            *str = std::move(stmt.GetText(0));

            return stmt.Step("web view SelectNewLine 2")== SqlStepCode::OK;
          }
          else{
            return false;
          }
    }
      
    bool SelectFromKey(const std::string& key, int64_t count, int64_t offset, std::string* str){
         
          auto& stmt = *m_select_from_key;

          stmt.Reset();

          stmt.BindText(1, key);

          stmt.BindInt64(2, count);
          stmt.BindInt64(3, offset);

          if(stmt.Step("web view SelectFromKey 1", false)== SqlStepCode::ROW){
            *str = std::move(stmt.GetText(0));

            return stmt.Step("web view SelectFromKey 2")== SqlStepCode::OK;
          }
          else{
            return false;
          }
    }
  };

  class MyDataInsertClass{
    std::shared_ptr<MySqliteConnect> m_db;
    MyTorrentFileTable m_file_table;
    MyFullTextTable m_fulltext_table;
    MyHashTable m_hash_table;
    MyTransactionStmt m_transaction;
    public:
      MyDataInsertClass(std::shared_ptr<MySqliteConnect> db): m_db(db), m_file_table(db), m_fulltext_table(db), m_hash_table(db), m_transaction(db){
         
      }

    auto& GetFileTable(){
      return  m_file_table;;
    }



    bool IsHaveHash(const std::string& key){
      return m_hash_table.IsHaveHash(key);
    }

    bool Insert(const BtMy::Torrent_Data& data){


        return m_transaction.RunOnTransaction([&v = data, obj= this]()-> bool{
          
          return obj->_Insert(v);
        });

        
    }

    private:
    bool _Insert(const BtMy::Torrent_Data& data){

      int64_t hashid=0;
      {
        auto res = m_hash_table.Insert(data.hash, &hashid);

        if(res != true){
          return res;
        }
      }

      {

         
          int64_t fulltexttableid=0;
          auto res = m_file_table.Insert(hashid, data.name, data.size, &fulltexttableid);

          if(res != true){
            return res;
          }


          res = m_fulltext_table.Insert(fulltexttableid, data.name);

          if(res != true){
            return res;
          }

      }
      

      for (const auto& item : data.files) {

        int64_t fulltexttableid=0;
        auto res = m_file_table.Insert(hashid, item.first, item.second, &fulltexttableid);

        if(res != true){
          return res;
        }


        res = m_fulltext_table.Insert(fulltexttableid, item.first);

        if(res != true){
          return res;
        }
      
      }

      return true;

    }
  };
}

#endif // !_MYSQLITECLASS