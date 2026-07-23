#include "DateUtils.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <regex>
#include <stdexcept>
#include <iostream>

// Получает день недели для указанной даты (0=воскресенье, ..., 6=суббота)
int DateUtils::getWeekday(const std::string& dateStr) {
    int day, month, year;
    char delimiter;
    std::istringstream iss(dateStr);
    iss >> day >> delimiter >> month >> delimiter >> year;
    if (iss.fail() || delimiter != '.') {
        std::cerr << "Ошибка: Неверный формат даты '" << dateStr << "'\n";
        throw std::runtime_error("Неверный формат даты: " + dateStr);
    }
    struct tm tm = { 0 };
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    time_t time = mktime(&tm);
    if (time == -1) {
        std::cerr << "Ошибка: Неверная дата '" << dateStr << "'\n";
        throw std::runtime_error("Неверная дата: " + dateStr);
    }
    return tm.tm_wday;
}

// Проверяет, является ли дата воскресеньем
bool DateUtils::isSunday(const std::string& dateStr) {
    try {
        return getWeekday(dateStr) == 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка в isSunday для даты '" << dateStr << "': " << e.what() << "\n";
        return false;
    }
}

// Проверяет, находится ли дата в январе
bool DateUtils::isJanuary(const std::string& dateStr) {
    try {
        int day, month, year;
        char delimiter;
        std::istringstream iss(dateStr);
        iss >> day >> delimiter >> month >> delimiter >> year;
        if (iss.fail() || delimiter != '.') {
            std::cerr << "Ошибка: Неверный формат даты в isJanuary '" << dateStr << "'\n";
            return false;
        }
        return month == 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка в isJanuary для даты '" << dateStr << "': " << e.what() << "\n";
        return false;
    }
}

// Форматирует дату в строку формата DD.MM.YYYY
std::string DateUtils::formatDate(const std::string& date) {
    if (date.find('.') != std::string::npos && date.length() >= 8) {
        return date;
    }
    try {
        double excelDate = std::stod(date);
        time_t rawtime = (time_t)((excelDate - 25569) * 86400);
        struct tm timeinfo;
        gmtime_s(&timeinfo, &rawtime);
        char buffer[11];
        strftime(buffer, sizeof(buffer), "%d.%m.%Y", &timeinfo);
        return buffer;
    }
    catch (...) {
        std::cerr << "Ошибка форматирования даты '" << date << "', возвращаем как есть\n";
        return date;
    }
}

// Парсит дату в time_point
std::chrono::system_clock::time_point DateUtils::parseDate(const std::string& dateStr) {
    std::regex dateRegex(R"((\d{2})\.(\d{2})\.(\d{4}))");
    std::smatch match;
    if (std::regex_match(dateStr, match, dateRegex)) {
        tm time = {};
        time.tm_mday = std::stoi(match[1]);
        time.tm_mon = std::stoi(match[2]) - 1;
        time.tm_year = std::stoi(match[3]) - 1900;
        time_t t = mktime(&time);
        if (t == -1) {
            std::cerr << "Ошибка: Неверная дата в parseDate '" << dateStr << "'\n";
            throw std::runtime_error("Неверная дата: " + dateStr);
        }
        return std::chrono::system_clock::from_time_t(t);
    }
    std::cerr << "Ошибка: Неверный формат даты в parseDate '" << dateStr << "'\n";
    throw std::runtime_error("Неверный формат даты: " + dateStr);
}

// Возвращает название дня недели на русском
std::string DateUtils::getDayOfWeek(const std::string& date) {
    try {
        int wday = getWeekday(date);
        switch (wday) {
        case 0: return "воскресенье";
        case 1: return "понедельник";
        case 2: return "вторник";
        case 3: return "среда";
        case 4: return "четверг";
        case 5: return "пятница";
        case 6: return "суббота";
        default: return "";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка в getDayOfWeek для даты '" << date << "': " << e.what() << "\n";
        return "";
    }
}

// Получает предыдущий день от указанной даты
std::string DateUtils::getPreviousDay(const std::string& date, int daysBack) {
    try {
        struct tm tm = {};
        std::istringstream ss(date);
        int day, month, year;
        char dot;
        ss >> day >> dot >> month >> dot >> year;
        if (ss.fail() || dot != '.') {
            std::cerr << "Ошибка: Неверный формат даты в getPreviousDay '" << date << "'\n";
            return "";
        }
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day - daysBack;
        time_t time = mktime(&tm);
        if (time == -1) {
            std::cerr << "Ошибка: Неверная дата в getPreviousDay '" << date << "'\n";
            return "";
        }
        struct tm timeinfo = {};
        localtime_s(&timeinfo, &time);
        char buffer[11];
        strftime(buffer, sizeof(buffer), "%d.%m.%Y", &timeinfo);
        return buffer;
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка в getPreviousDay для даты '" << date << "': " << e.what() << "\n";
        return "";
    }
}

// Проверяет, находится ли дата в заданном диапазоне
bool DateUtils::isDateInRange(const std::string& date, const std::string& start, const std::string& end) {
    try {
        auto tDate = parseDate(date);
        auto tStart = parseDate(start);
        auto tEnd = parseDate(end);
        return tDate >= tStart && tDate <= tEnd;
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка в isDateInRange для даты '" << date << "': " << e.what() << "\n";
        return false;
    }
}

// Вычисляет количество дней между двумя датами
int DateUtils::daysBetween(const std::string& date1, const std::string& date2) {
    try {
        auto t1 = parseDate(date1);
        auto t2 = parseDate(date2);
        auto diff = std::chrono::duration_cast<std::chrono::hours>(t2 - t1).count() / 24;
        return static_cast<int>(diff);
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка в daysBetween для дат '" << date1 << "' и '" << date2 << "': " << e.what() << "\n";
        return 0;
    }
}