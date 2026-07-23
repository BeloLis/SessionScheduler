#pragma once
#include <string>
#include <vector>
#include <chrono>

// Утилиты для работы с датами
namespace DateUtils {
    // Возвращает день недели (0=воскресенье, 1=понедельник, ..., 6=суббота)
    int getWeekday(const std::string& date);

    // Проверяет, является ли дата воскресеньем
    bool isSunday(const std::string& date);

    // Проверяет, находится ли дата в январе
    bool isJanuary(const std::string& date);

    // Форматирует дату в строку формата DD.MM.YYYY
    std::string formatDate(const std::string& date);

    // Парсит дату в time_point
    std::chrono::system_clock::time_point parseDate(const std::string& date);

    // Возвращает название дня недели на русском
    std::string getDayOfWeek(const std::string& date);

    // Получает предыдущий день от указанной даты
    std::string getPreviousDay(const std::string& date, int daysBack = 1);

    // Проверяет, находится ли дата в заданном диапазоне
    bool isDateInRange(const std::string& date, const std::string& start, const std::string& end);

    // Вычисляет количество дней между двумя датами
    int daysBetween(const std::string& date1, const std::string& date2);
}