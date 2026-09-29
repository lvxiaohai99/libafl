/**
 * @file   Date.h
 * @brief  日期类
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "afl/base/Common.h"

#include <string>

namespace afl
{
namespace time
{
// 闰年判定宏（已弃用，请用 isLeapYear）

class Date
{
public:
    Date();
    Date(int year, int month, int day);

public:
    static Date today();
    static Date utcToday();
    static bool isLeapYear(int year);
    static bool isValid(int year, int month, int day);
    static int daysInMonth(int year, int month);
    static int daysInPreviousMonth(int year, int month);
    static int daysInNextMonth(int year, int month);
    static int compare(const Date& lhs, const Date& rhs);
    static int daysDiff(const Date& from, const Date& to);

public:
    bool set(int year, int month, int day);
    bool year(int year);
    bool month(int month);
    bool day(int day);

    int year() { return m_year; }
    int month() { return m_month; }
    int day() { return m_day; }
    bool isLeapYear() { return isLeapYear(m_year); }

    Date& addYears(int years);
    Date& addMonths(int months);
    Date& addDays(int days);

    Date nextDay() const;
    Date nextMonth() const;
    Date nextYear() const;

    bool isEqual(const Date& rhs) const;

    int daysDiff(const Date& to) const; // 距离日期to有多少天
    int daysToNextYear() const;         // 距离下一年的今天有多少天
    int daysToNextMonth() const;        // 距离下个月的今天有多少天
    int daysToPreviousYear() const;     // 距离上一年的今天有多少天
    int daysToPreviousMonth() const;    // 距离上个月的今天有多少天

    std::string toString() const;

public:
    Date& operator+=(int days);
    Date& operator-=(int days);
    Date& operator++();
    Date& operator--();
    Date operator++(int);
    Date operator--(int);
    Date operator+(int days);
    Date operator-(int days);

    bool operator<(const Date& rhs) const { return Date::compare(*this, rhs) < 0; }

    bool operator>(const Date& rhs) const { return Date::compare(*this, rhs) > 0; }

    bool operator==(const Date& rhs) const { return Date::compare(*this, rhs) == 0; }

    bool operator<=(const Date& rhs) const { return Date::compare(*this, rhs) <= 0; }

    bool operator>=(const Date& rhs) const { return Date::compare(*this, rhs) >= 0; }

    bool operator!=(const Date& rhs) const { return Date::compare(*this, rhs) != 0; }
private:
    void adjustMonth();

private:
    int m_year;  // [1900..]
    int m_month; // [1..12]
    int m_day;   // [1..31]
};

} // namespace time
} // namespace afl
