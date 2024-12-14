#pragma once

#include <functional>
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

  enum class SqlStepCode{
    OK,
    ROW,
    CONSTRAINT_UNIQUE,
    CONSTRAINT
  };

  class MySqliteStmt
  {

    sqlite3_stmt *m_stmt;
    sqlite3 *m_db;

  public:
    MySqliteStmt(sqlite3 *db, const std::string &sql) : m_db(db)
    {

      const char *notUse = NULL;

      auto res = sqlite3_prepare_v2(
          m_db,
          sql.data(),
          static_cast<int>(sql.size()),
          &m_stmt,
          &notUse);

      if (res != SQLITE_OK)
      {

        Print(sqlite3_errmsg(m_db));
        Exit("prepare stm error");
      }
    }

    SqlStepCode Step()
    {
      auto res = sqlite3_step(m_stmt);

      if (res == SQLITE_ROW)
      {
        return SqlStepCode::ROW;
      }


      if (res != SQLITE_DONE && res != SQLITE_OK)
      {
        auto excode = sqlite3_extended_errcode(m_db);
       
        if(excode==SQLITE_CONSTRAINT_UNIQUE){
          return SqlStepCode::CONSTRAINT_UNIQUE;
        }

        if(excode==SQLITE_CONSTRAINT){
          return SqlStepCode::CONSTRAINT;
        }

        Print("code", res, "excode:", excode, "error meg", sqlite3_errmsg(m_db));
        Exit("step stm error");
      }

      return SqlStepCode::OK;
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
      auto uperrorcode =  sqlite3_errcode(m_db);

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

    void BindPointer(int index, void *p, const std::string &name)
    {
      auto res = sqlite3_bind_pointer(m_stmt, index, p, name.data(), NULL);

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

      stmt.Step();

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

      auto res = sqlite3_open_v2(path.c_str(),
                                 &m_db, SQLITE_OPEN_MEMORY | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

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

    if(stmt.Step()!= SqlStepCode::OK){
      Exit("create table error");
    }

  }


  class MyHashTable{

    std::shared_ptr<MySqliteConnect> m_db;
    std::unique_ptr<MySqliteStmt> m_inset;
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
          )"""");
        
      }

     bool Insert(const std::string& s, int64_t* pid){

          m_inset->BindText(1, s);
        
          auto res = m_inset->Step();
          bool isok;
          if(res == SqlStepCode::ROW){
            
            *pid = m_inset->GetInt64(0);
            isok = true;
          }
          else{
            isok =false;
          }

         m_inset->Reset();

         return isok;
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
             fulltext_table USING fts5(text, tokenize="mytokenizer");               
          )"""");

      
         m_inset = std::make_unique<MySqliteStmt>(db->Get(), R""""(
            INSERT INTO fulltext_table (rowid, text) VALUES (?1, ?2);
          )"""");
        
      }


      bool Insert(int64_t id, const std::string& s){

          m_inset->BindInt64(1, id);
          m_inset->BindText(2, s);
        
          auto res = m_inset->Step();
          bool isok= res == SqlStepCode::OK;
         
         m_inset->Reset();

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
       
        while (stmt.Step() == SqlStepCode::ROW) {
          auto id = stmt.GetInt64(0);
          auto text = stmt.GetText(1);

          func(id, text);
        }

        
     }

  };
}

#endif // !_MYSQLITECLASS