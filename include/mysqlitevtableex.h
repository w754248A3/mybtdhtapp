#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <utility>
#include <vector>
#include <winnt.h>
#include "leikaifeng.h"
#ifndef _MYSQLITEVTABLEEX
#define _MYSQLITEVTABLEEX

/*
** 2018-04-19
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
**
** This file implements a template virtual-table.
** Developers can make a copy of this file as a baseline for writing
** new virtual tables and/or table-valued functions.
**
** Steps for writing a new virtual table implementation:
**
**     (1)  Make a copy of this file.  Perhaps call it "mynewvtab.c"
**
**     (2)  Replace this header comment with something appropriate for
**          the new virtual table
**
**     (3)  Change every occurrence of "templatevtab" to some other string
**          appropriate for the new virtual table.  Ideally, the new string
**          should be the basename of the source file: "mynewvtab".  Also
**          globally change "TEMPLATEVTAB" to "MYNEWVTAB".
**
**     (4)  Run a test compilation to make sure the unmodified virtual
**          table works.
**
**     (5)  Begin making incremental changes, testing as you go, to evolve
**          the new virtual table to do what you want it to do.
**
** This template is minimal, in the sense that it uses only the required
** methods on the sqlite3_module object.  As a result, templatevtab is
** a read-only and eponymous-only table.  Those limitation can be removed
** by adding new methods.
**
** This template implements an eponymous-only virtual table with a rowid and
** two columns named "a" and "b".  The table as 10 rows with fixed integer
** values. Usage example:
**
**     SELECT rowid, a, b FROM templatevtab;
*/

#include <cwchar>
#if !defined(SQLITEINT_H)
#include "sqlite3ext.h"
#endif
SQLITE_EXTENSION_INIT1
#include <assert.h>
#include <string.h>

namespace SqlMyEx {
/* templatevtab_vtab is a subclass of sqlite3_vtab which is
** underlying representation of the virtual table
*/
typedef struct templatevtab_vtab templatevtab_vtab;
struct templatevtab_vtab {
  sqlite3_vtab base; /* Base class - must be first */
  /* Add new fields here, as necessary */
};

/* templatevtab_cursor is a subclass of sqlite3_vtab_cursor which will
** serve as the underlying representation of a cursor that scans
** over rows of the result
*/
typedef struct templatevtab_cursor templatevtab_cursor;
struct templatevtab_cursor {
  sqlite3_vtab_cursor base; /* Base class - must be first */
  /* Insert new fields here.  For this templatevtab we only keep track
  ** of the rowid */
  sqlite3_int64 iRowid; /* The rowid */

  size_t index;

  std::vector<std::pair<INT64, std::string>>* vs;
};


static int SetErrorMeg(sqlite3_vtab* vtab, const char* errorStr){
  sqlite3_free(vtab->zErrMsg);
  auto p= sqlite3_mprintf(errorStr);
  if(p == NULL){
    return  SQLITE_NOMEM;
  }
  vtab->zErrMsg = p;
  return SQLITE_ERROR;
}

/*
** The templatevtabConnect() method is invoked to create a new
** template virtual table.
**
** Think of this routine as the constructor for templatevtab_vtab objects.
**
** All this routine needs to do is:
**
**    (1) Allocate the templatevtab_vtab object and initialize all fields.
**
**    (2) Tell SQLite (via the sqlite3_declare_vtab() interface) what the
**        result set of queries against the virtual table will look like.
*/
static int templatevtabConnect(sqlite3 *db, void *pAux, int argc,
                               const char *const *argv, sqlite3_vtab **ppVtab,
                               char **pzErr) {
  templatevtab_vtab *pNew;
  int rc;

  rc = sqlite3_declare_vtab(db, R""""(
            CREATE TABLE x(
            a,
            b,
            vs HIDDEN
          );
          )"""");
  /* For convenience, define symbolic names for the index to each column. */
#define TEMPLATEVTAB_A 0
#define TEMPLATEVTAB_B 1
  if (rc == SQLITE_OK) {
    pNew = (decltype(pNew))sqlite3_malloc(sizeof(*pNew));
    *ppVtab = (sqlite3_vtab *)pNew;
    if (pNew == 0)
      return SQLITE_NOMEM;
    memset(pNew, 0, sizeof(*pNew));
  }
  return rc;
}

/*
** This method is the destructor for templatevtab_vtab objects.
*/
static int templatevtabDisconnect(sqlite3_vtab *pVtab) {
  templatevtab_vtab *p = (templatevtab_vtab *)pVtab;
  sqlite3_free(p);
  return SQLITE_OK;
}

/*
** Constructor for a new templatevtab_cursor object.
*/
static int templatevtabOpen(sqlite3_vtab *p, sqlite3_vtab_cursor **ppCursor) {
  templatevtab_cursor *pCur;
  pCur = (decltype(pCur))sqlite3_malloc(sizeof(*pCur));
  if (pCur == 0)
    return SQLITE_NOMEM;
  memset(pCur, 0, sizeof(*pCur));
  *ppCursor = &pCur->base;
  return SQLITE_OK;
}

/*
** Destructor for a templatevtab_cursor.
*/
static int templatevtabClose(sqlite3_vtab_cursor *cur) {
  templatevtab_cursor *pCur = (templatevtab_cursor *)cur;
  sqlite3_free(pCur);
  return SQLITE_OK;
}

/*
** Advance a templatevtab_cursor to its next row of output.
*/
static int templatevtabNext(sqlite3_vtab_cursor *cur) {
  templatevtab_cursor *pCur = (templatevtab_cursor *)cur;
  pCur->iRowid++;
  pCur->index++;
  return SQLITE_OK;
}

/*
** Return values of columns for the row at which the templatevtab_cursor
** is currently pointing.
*/
static int templatevtabColumn(
    sqlite3_vtab_cursor *cur, /* The cursor */
    sqlite3_context *ctx,     /* First argument to sqlite3_result_...() */
    int i                     /* Which column to return */
) {
  Print("need clo", i);
  templatevtab_cursor *pCur = (templatevtab_cursor *)cur;

  auto& v = pCur->vs->at(pCur->index);
  if(i == 0){
    sqlite3_result_int64(ctx, v.first);
  }
  else if(i == 1){
    
    sqlite3_result_text(ctx, v.second.data(), (int)v.second.size(), SQLITE_TRANSIENT);
  }
  else{
    sqlite3_result_text(ctx, "Column get not clo", -1, SQLITE_STATIC);
    return  SQLITE_ERROR;
  }
  
  return SQLITE_OK;
}

/*
** Return the rowid for the current row.  In this implementation, the
** rowid is the same as the output value.
*/
static int templatevtabRowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid) {
  templatevtab_cursor *pCur = (templatevtab_cursor *)cur;
  *pRowid = pCur->iRowid;
  return SQLITE_OK;
}

/*
** Return TRUE if the cursor has been moved off of the last
** row of output.
*/
static int templatevtabEof(sqlite3_vtab_cursor *cur) {
  templatevtab_cursor *pCur = (templatevtab_cursor *)cur;
  return pCur->index >= pCur->vs->size();
}



/*
** This method is called to "rewind" the templatevtab_cursor object back
** to the first row of output.  This method is always called at least
** once prior to any call to templatevtabColumn() or templatevtabRowid() or
** templatevtabEof().
*/
static int templatevtabFilter(sqlite3_vtab_cursor *pVtabCursor, int idxNum,
                              const char *idxStr, int argc,
                              sqlite3_value **argv) {



                        
  if (argc < 1){
    return SetErrorMeg(pVtabCursor->pVtab, "Filter args length error");
  }

  auto v = sqlite3_value_pointer(argv[0],"std::vector");
  if(v == NULL){
    return SetErrorMeg(pVtabCursor->pVtab, "Filter args pointer is null");
    
  }
                           
  Print(v);
  templatevtab_cursor *pCur = (templatevtab_cursor *)pVtabCursor;
  pCur->iRowid = 1;
  pCur->index=0;
  pCur->vs=(decltype(pCur->vs))v;


  return SQLITE_OK;
}

/*
** SQLite will invoke this method one or more times while planning a query
** that uses the virtual table.  This routine needs to create
** a query plan for each invocation and compute an estimated cost for that
** plan.
*/
static int templatevtabBestIndex(sqlite3_vtab *tab,sqlite3_index_info *pIdxInfo) {

  //op 为SQLITE_INDEX_CONSTRAINT_LIMIT  SQLITE_INDEX_CONSTRAINT_OFFSET 时 iColumn无意义
  {

    std::array<sqlite3_index_info::sqlite3_index_constraint, 1> needArgs{};

    needArgs[0]={
      .iColumn=2,
      .op= SQLITE_INDEX_CONSTRAINT_EQ,
      .usable=true,
      .iTermOffset=0 //这个值不用
    };

    int needIndex =1;
    for (const auto&  argsItem: needArgs) {
        const auto begin = pIdxInfo->aConstraint;
        const auto end = begin+ pIdxInfo->nConstraint;
        Print("have find clo index", argsItem.iColumn);
        const auto res = std::find_if(begin, end,
        [&argsItem](decltype(*begin) v)->bool{
            Print("clo index", v.iColumn);
            return v.iColumn == argsItem.iColumn;
        });

        if(res == end){
          //缺少必须参数
          return SetErrorMeg(tab, "not find v table args");
        }
        else if(res->op != argsItem.op || res->usable != argsItem.usable){
          //参数不满足约束
          return SQLITE_CONSTRAINT;
        }
        else{
          //满足约束后告诉core需要这个参数
          auto i = res-begin;
          
          pIdxInfo->aConstraintUsage[i].argvIndex=needIndex;
          pIdxInfo->aConstraintUsage[i].omit=1; //抑制core再次检查约束,避免不使用隐藏列时core获取隐藏列的值
          needIndex++;
        }
    }

  }
  

  pIdxInfo->estimatedCost = (double)10;
  //pIdxInfo->estimatedRows = 10;
  return SQLITE_OK;
}

/*
** This following structure defines all the methods for the
** virtual table.
*/
static sqlite3_module templatevtabModule = {
    /* iVersion    */ 0,
    /* xCreate     */ 0,
    /* xConnect    */ templatevtabConnect,
    /* xBestIndex  */ templatevtabBestIndex,
    /* xDisconnect */ templatevtabDisconnect,
    /* xDestroy    */ 0,
    /* xOpen       */ templatevtabOpen,
    /* xClose      */ templatevtabClose,
    /* xFilter     */ templatevtabFilter,
    /* xNext       */ templatevtabNext,
    /* xEof        */ templatevtabEof,
    /* xColumn     */ templatevtabColumn,
    /* xRowid      */ templatevtabRowid,
    /* xUpdate     */ 0,
    /* xBegin      */ 0,
    /* xSync       */ 0,
    /* xCommit     */ 0,
    /* xRollback   */ 0,
    /* xFindMethod */ 0,
    /* xRename     */ 0,
    /* xSavepoint  */ 0,
    /* xRelease    */ 0,
    /* xRollbackTo */ 0,
    /* xShadowName */ 0,
    /* xIntegrity  */ 0};

#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_mysqlitevtableex_init(
  sqlite3 *db, 
  char **pzErrMsg, 
  const sqlite3_api_routines *pApi
){
  int rc = SQLITE_OK;
  SQLITE_EXTENSION_INIT2(pApi);
  rc = sqlite3_create_module(db, "templatevtab", &templatevtabModule, 0);
  return rc;
}
} // namespace SqlMyEx
#endif // !_MYSQLITEVTABLEEX