#include "TimePreference.h"
#include <algorithm>
#include <stdexcept>

// Конструктор по умолчанию, инициализирует пустые значения
TimePreference::TimePreference() : isStrict(false) {}

// Конструктор с разбором строки предпочтений времени
TimePreference::TimePreference(const std::string& prefStr) : isStrict(false) {
    if (prefStr.empty()) return;

    size_t dashPos = prefStr.find('-');
    if (dashPos != std::string::npos) {
        timeRange.first = prefStr.substr(0, dashPos);
        timeRange.second = prefStr.substr(dashPos + 1);
        isStrict = true;
    }
}

// Проверяет, валидны ли временные предпочтения
bool TimePreference::isValid() const {
    return !timeRange.first.empty() && !timeRange.second.empty();
}

// Проверяет, пусты ли временные предпочтения
bool TimePreference::empty() const {
    return timeRange.first.empty() && timeRange.second.empty();
}

// Получает начальное время в долях дня
double TimePreference::getStartTime() const {
    if (timeRange.first.empty()) return 0.0;
    try {
        std::string time = timeRange.first;
        std::replace(time.begin(), time.end(), '.', ':');
        size_t colonPos = time.find(':');
        if (colonPos == std::string::npos) return std::stod(time);
        double hours = std::stod(time.substr(0, colonPos));
        double minutes = std::stod(time.substr(colonPos + 1)) / 60.0;
        return hours + minutes;
    }
    catch (...) {
        return 0.0;
    }
}

// Получает конечное время в долях дня
double TimePreference::getEndTime() const {
    if (timeRange.second.empty()) return 24.0;
    try {
        std::string time = timeRange.second;
        std::replace(time.begin(), time.end(), '.', ':');
        size_t colonPos = time.find(':');
        if (colonPos == std::string::npos) return std::stod(time);
        double hours = std::stod(time.substr(0, colonPos));
        double minutes = std::stod(time.substr(colonPos + 1)) / 60.0;
        return hours + minutes;
    }
    catch (...) {
        return 24.0;
    }
}