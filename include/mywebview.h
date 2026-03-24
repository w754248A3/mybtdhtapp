#pragma once

#ifndef _MYWEBVIEW
#define _MYWEBVIEW

#include <memory>
#include <string>
#include "mysqliteclass.h"
namespace MyWebView {

void StartServer(std::shared_ptr<SqlMy::MyWebViewSelectClass> table, std::wstring path);

} // namespace MyWebView

#endif // !_MYWEBVIEW