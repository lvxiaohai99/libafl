#pragma once

/**
 * @file Asn1Json.h
 * @brief 按 asn1c 类型描述符把任意生成结构转成 JSON 文本（仅用于显示 / 日志）
 *
 * asn1c 0.9.29 没有 JER，这里遍历 asn_TYPE_descriptor_t 自行输出：
 *   SEQUENCE / SET      → 对象，缺省的 OPTIONAL 不输出
 *   CHOICE              → {"分支名": 值}，未选择时为 null
 *   SEQUENCE OF / SET OF→ 数组
 *   NativeInteger       → 数字；NativeEnumerated → 枚举名字符串（未知值输出数字）
 *   BOOLEAN / NULL      → true|false / null
 *   IA5String 等字符串  → 字符串
 *   OCTET STRING        → 大写 hex 字符串（无分隔）
 *   BIT STRING          → "0101…"，长度为有效位数
 *   其它（INTEGER 大数、REAL 等）→ asn1c print_struct 文本，能解析成数字则输出数字
 *
 * 不支持从 JSON 反向解码。
 * 依赖：先包含 asn1c 生成的头，再包含本文件（通常经 Asn1Cpp.h 间接包含）。
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#ifndef ASN_APPLICATION_H
#error "Include asn1c generated headers (e.g. MessageFrame.h) before afl/asn1/Asn1Json.h"
#endif

#include "BIT_STRING.h"
#include "BOOLEAN.h"
#include "INTEGER.h"
#include "OCTET_STRING.h"
#include "constr_CHOICE.h"
#include "constr_SEQUENCE.h"
#include "constr_SET_OF.h"

/* 弱引用：工程未链接某运行库类型时地址为 0，只是不会匹配到该分支 */
extern "C" {
extern asn_TYPE_operation_t asn_OP_SEQUENCE __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_SET __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_CHOICE __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_SEQUENCE_OF __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_SET_OF __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_NativeInteger __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_NativeEnumerated __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_BOOLEAN __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_NULL __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_OCTET_STRING __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_BIT_STRING __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_IA5String __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_UTF8String __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_VisibleString __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_PrintableString __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_NumericString __attribute__((weak));
extern asn_TYPE_operation_t asn_OP_ISO646String __attribute__((weak));
}

namespace afl {
namespace asn1 {
namespace json_detail {

inline bool opIs(const asn_TYPE_operation_t* op, const asn_TYPE_operation_t* ref)
{
    return ref != 0 && op == ref;
}

inline void newline(std::string& out, int indent, int level)
{
    if (indent > 0) {
        out += '\n';
        out.append(static_cast<size_t>(indent * level), ' ');
    }
}

inline void appendEscaped(std::string& out, const char* s, size_t n)
{
    static const char kHex[] = "0123456789abcdef";
    out += '"';
    for (size_t i = 0; i < n; ++i) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (c < 0x20) {
                out += "\\u00";
                out += kHex[c >> 4];
                out += kHex[c & 0x0F];
            } else {
                out += static_cast<char>(c);
            }
        }
    }
    out += '"';
}

inline int appendToString(const void* buf, size_t size, void* key)
{
    static_cast<std::string*>(key)->append(static_cast<const char*>(buf), size);
    return 0;
}

/** 用 asn1c 自带 print_struct 输出；能完整解析成有限数字则原样输出，否则作为字符串 */
inline void appendFallback(std::string& out, const asn_TYPE_descriptor_t* td, const void* ptr)
{
    std::string text;
    if (td->op->print_struct(td, ptr, 0, &appendToString, &text) < 0) {
        out += "null";
        return;
    }
    if (!text.empty()) {
        char* end = 0;
        const double v = strtod(text.c_str(), &end);
        if (end && *end == '\0' && v == v && v - v == 0) {
            out += text;
            return;
        }
    }
    appendEscaped(out, text.data(), text.size());
}

inline const void* memberPtr(const void* base, const asn_TYPE_member_t* elm)
{
    const char* p = static_cast<const char*>(base) + elm->memb_offset;
    if (elm->flags & ATF_POINTER) {
        return *reinterpret_cast<const void* const*>(p);
    }
    return p;
}

inline unsigned choicePresent(const void* base, const asn_CHOICE_specifics_t* sp)
{
    const char* p = static_cast<const char*>(base) + sp->pres_offset;
    switch (sp->pres_size) {
    case sizeof(uint8_t): {
        uint8_t v;
        memcpy(&v, p, sizeof(v));
        return v;
    }
    case sizeof(uint16_t): {
        uint16_t v;
        memcpy(&v, p, sizeof(v));
        return v;
    }
    case sizeof(uint32_t): {
        uint32_t v;
        memcpy(&v, p, sizeof(v));
        return v;
    }
    default:
        return 0;
    }
}

inline void appendValue(std::string& out, const asn_TYPE_descriptor_t* td, const void* ptr,
                        int indent, int level);

inline void appendKey(std::string& out, const char* name, int indent)
{
    appendEscaped(out, name, strlen(name));
    out += indent > 0 ? ": " : ":";
}

inline void appendSequence(std::string& out, const asn_TYPE_descriptor_t* td, const void* ptr,
                           int indent, int level)
{
    out += '{';
    bool first = true;
    for (unsigned i = 0; i < td->elements_count; ++i) {
        const asn_TYPE_member_t* elm = &td->elements[i];
        const void* m = memberPtr(ptr, elm);
        if (m == 0) {
            continue;
        }
        if (!first) {
            out += ',';
        }
        first = false;
        newline(out, indent, level + 1);
        appendKey(out, elm->name, indent);
        appendValue(out, elm->type, m, indent, level + 1);
    }
    if (!first) {
        newline(out, indent, level);
    }
    out += '}';
}

inline void appendChoice(std::string& out, const asn_TYPE_descriptor_t* td, const void* ptr,
                         int indent, int level)
{
    const asn_CHOICE_specifics_t* sp = static_cast<const asn_CHOICE_specifics_t*>(td->specifics);
    const unsigned present = sp ? choicePresent(ptr, sp) : 0;
    if (present == 0 || present > td->elements_count) {
        out += "null";
        return;
    }
    const asn_TYPE_member_t* elm = &td->elements[present - 1];
    out += '{';
    newline(out, indent, level + 1);
    appendKey(out, elm->name, indent);
    appendValue(out, elm->type, memberPtr(ptr, elm), indent, level + 1);
    newline(out, indent, level);
    out += '}';
}

inline void appendList(std::string& out, const asn_TYPE_descriptor_t* td, const void* ptr,
                       int indent, int level)
{
    const asn_anonymous_set_* list = _A_CSET_FROM_VOID(ptr);
    const asn_TYPE_descriptor_t* et = td->elements->type;
    out += '[';
    for (int i = 0; i < list->count; ++i) {
        if (i > 0) {
            out += ',';
        }
        newline(out, indent, level + 1);
        appendValue(out, et, list->array[i], indent, level + 1);
    }
    if (list->count > 0) {
        newline(out, indent, level);
    }
    out += ']';
}

inline void appendHex(std::string& out, const uint8_t* buf, size_t n)
{
    static const char kHex[] = "0123456789ABCDEF";
    out += '"';
    for (size_t i = 0; i < n; ++i) {
        out += kHex[buf[i] >> 4];
        out += kHex[buf[i] & 0x0F];
    }
    out += '"';
}

inline void appendBits(std::string& out, const BIT_STRING_t* bs)
{
    out += '"';
    const size_t bits = bs->size * 8 - static_cast<size_t>(bs->bits_unused);
    for (size_t i = 0; bs->buf && i < bits; ++i) {
        out += (bs->buf[i / 8] & (0x80 >> (i % 8))) ? '1' : '0';
    }
    out += '"';
}

inline void appendValue(std::string& out, const asn_TYPE_descriptor_t* td, const void* ptr,
                        int indent, int level)
{
    if (ptr == 0) {
        out += "null";
        return;
    }
    const asn_TYPE_operation_t* op = td->op;

    if (opIs(op, &asn_OP_SEQUENCE) || opIs(op, &asn_OP_SET)) {
        appendSequence(out, td, ptr, indent, level);
    } else if (opIs(op, &asn_OP_CHOICE)) {
        appendChoice(out, td, ptr, indent, level);
    } else if (opIs(op, &asn_OP_SEQUENCE_OF) || opIs(op, &asn_OP_SET_OF)) {
        appendList(out, td, ptr, indent, level);
    } else if (opIs(op, &asn_OP_NativeInteger)) {
        const asn_INTEGER_specifics_t* sp =
            static_cast<const asn_INTEGER_specifics_t*>(td->specifics);
        const long v = *static_cast<const long*>(ptr);
        char buf[32];
        if (sp && sp->field_unsigned) {
            snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(v));
        } else {
            snprintf(buf, sizeof(buf), "%ld", v);
        }
        out += buf;
    } else if (opIs(op, &asn_OP_NativeEnumerated)) {
        const asn_INTEGER_specifics_t* sp =
            static_cast<const asn_INTEGER_specifics_t*>(td->specifics);
        const long v = *static_cast<const long*>(ptr);
        for (int i = 0; sp && i < sp->map_count; ++i) {
            if (sp->value2enum[i].nat_value == v) {
                appendEscaped(out, sp->value2enum[i].enum_name, sp->value2enum[i].enum_len);
                return;
            }
        }
        char buf[32];
        snprintf(buf, sizeof(buf), "%ld", v);
        out += buf;
    } else if (opIs(op, &asn_OP_BOOLEAN)) {
        out += *static_cast<const BOOLEAN_t*>(ptr) ? "true" : "false";
    } else if (opIs(op, &asn_OP_NULL)) {
        out += "null";
    } else if (opIs(op, &asn_OP_BIT_STRING)) {
        appendBits(out, static_cast<const BIT_STRING_t*>(ptr));
    } else if (opIs(op, &asn_OP_OCTET_STRING)) {
        const OCTET_STRING_t* os = static_cast<const OCTET_STRING_t*>(ptr);
        appendHex(out, os->buf, os->size);
    } else if (opIs(op, &asn_OP_IA5String) || opIs(op, &asn_OP_UTF8String)
               || opIs(op, &asn_OP_VisibleString) || opIs(op, &asn_OP_PrintableString)
               || opIs(op, &asn_OP_NumericString) || opIs(op, &asn_OP_ISO646String)) {
        const OCTET_STRING_t* os = static_cast<const OCTET_STRING_t*>(ptr);
        appendEscaped(out, reinterpret_cast<const char*>(os->buf), os->size);
    } else {
        appendFallback(out, td, ptr);
    }
}

} // namespace json_detail

/**
 * 把 asn1c 结构转成 JSON 文本。
 * @param indent 每级缩进空格数；0 输出单行紧凑 JSON
 * @return ptr 为空时返回 "null"
 */
inline std::string toJson(const asn_TYPE_descriptor_t& td, const void* ptr, int indent = 2)
{
    std::string out;
    json_detail::appendValue(out, &td, ptr, indent, 0);
    return out;
}

} // namespace asn1
} // namespace afl
