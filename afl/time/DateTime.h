/**
 * @file   DateTime.h
 * @brief  日期、时间工具
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
class DateTime
{
public:
    /** @brief 判断指定年份是否为闰年 */
    static bool isLeapYear(int year);

    /** @brief 获取当前日期和时间 */
    static void currentDateTime(struct tm* ptm);

    /**
     * @brief 获取当前日期和时间字符串 YYYY-MM-DD HH:MM:SS
     * @param buf 输出缓冲区
     * @param size buf 大小，需大于 sizeof("YYYY-MM-DD HH:MM:SS")
     */
    static void currentDateTime(char* buf, size_t size);

    /** @return 当前日期和时间 YYYY-MM-DD HH:MM:SS */
    static std::string currentDateTime();

    /**
     * @brief 获取当前日期 YYYY-MM-DD
     * @param buf 输出缓冲区
     * @param size buf 大小，需大于 sizeof("YYYY-MM-DD")
     */
    static void currentDate(char* buf, size_t size);

    /** @return 当前日期 YYYY-MM-DD */
    static std::string currentDate();

    /**
     * @brief 获取当前时间 HH:MM:SS
     * @param buf 输出缓冲区
     * @param size buf 大小，需大于 sizeof("HH:MM:SS")
     */
    static void currentTime(char* buf, size_t size);

    /** @return 当前时间 HH:MM:SS */
    static std::string currentTime();

    /**
    * @brief           将一个字符串转换成日期时间格式，要求原字符串格式: YYYY-MM-DD HH:MM:SS
    * @param strTime   包含时间格式的字符串
    * @param datetime  存储字符串转换结果的时间结构指针
    * @return          转换成功返回true，否则返回false
    */
    static bool stringToDataTime(const char* strTime, struct tm* datetime);
    /**
    * @brief           将一个字符串转换成日期时间格式，要求原字符串格式: YYYY-MM-DD HH:MM:SS
    * @param strTime   包含时间格式的字符串
    * @param datetime  存储字符串转换结果的时间结构指针
    * @return          转换成功返回true，否则返回false
    */
    static bool stringToDataTime(const char* strTime, time_t* datetime);

    /**
    * @brief           将一个字符串转换成日期时间格式，要求原字符串格式: YYYY-MM-DD HH:MM:SS
    * @param datetime  指向日期和时间的结构体指针
    * @param buf       存储格式化结果字符串的缓冲区
    * @param size      缓冲区大小
    */
    static void dateTimeToString(struct tm* datetime, char* buf, size_t size);
    /**
    * @brief           将一个日期和时间转换为字符串: YYYY-MM-DD HH:MM:SS
    * @param datetime  指向日期和时间的结构体指针
    * @return          返回时间格式化后的字符串
    */
    static std::string dateTimeToString(struct tm* datetime);

    /**
    * @brief           将一个日期转换为字符: YYYY-MM-DD
    * @param datetime  指向日期和时间的结构体指针
    * @param buf       存储格式化结果字符串的缓冲区
    * @param size      缓冲区大小
    */
    static void dateToString(struct tm* datetime, char* buf, size_t size);
    /**
    * @brief           将一个日期转换为字符: YYYY-MM-DD
    * @param datetime  指向日期和时间的结构体指针
    * @return          返回时间格式化后的字符串
    */
    static std::string dateToString(struct tm* datetime);

    /**
    * @brief           将一个时间转换为字符: HH:MM:SS
    * @param datetime  指向日期和时间的结构体指针
    * @param buf       存储当前日期和时间的缓冲区
    * @param size      缓冲区大小
    */
    static void timeToString(struct tm* datetime, char* buf, size_t size);
    /**
    * @brief           将一个时间转换为字符: HH:MM:SS
    * @param datetime  指向日期和时间的结构体指针
    * @return          返回时间格式化后的字符串
    */
    static std::string timeToString(struct tm* datetime);
};

} // namespace time
} // namespace afl
