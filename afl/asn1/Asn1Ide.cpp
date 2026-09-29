/**
 * @file Asn1Ide.cpp
 * @brief 仅供 clangd / IDE 索引：让 afl/asn1/*.h 与同目录 TU 关联。
 *
 * Asn1Cpp.h / Asn1Json.h 要求先包含 asn1c 生成头（ASN_APPLICATION_H）。
 * 单独打开这些头时，若没有同目录编译单元，clangd 会选错 flags，触发 #error，
 * 导致连同文件内的符号也无法跳转。本文件编进 compile_commands，不安装、不链接进业务库。
 */
#include "MessageFrame.h"
#include "afl/asn1/Asn1Cpp.h"
