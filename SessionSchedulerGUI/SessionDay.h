#pragma once
#include <string>

// Структура для представления дня сессии
struct SessionDay {
    std::string date;     // Дата в формате "ДД.ММ.ГГГГ"
    bool isHoliday;       // Флаг, указывающий, является ли день праздничным
    std::string weekday;  // День недели (например, "Пн", "Вт")

    // Конструктор для инициализации даты и статуса праздника
    // @param date Дата в формате строки
    // @param isHoliday Признак праздничного дня
    SessionDay(const std::string& date, bool isHoliday);

    // Получение даты
    // @return Дата в формате строки
    std::string getDate() const { return date; }
};