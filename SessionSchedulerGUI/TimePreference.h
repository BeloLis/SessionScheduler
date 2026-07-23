#pragma once
#include <string>
#include <utility>

// Класс для представления предпочтений по времени для экзаменов
class TimePreference {
public:
    // Конструктор по умолчанию
    TimePreference();

    // Конструктор с инициализацией строки предпочтений времени
    explicit TimePreference(const std::string& prefStr);

    // Проверяет, валидны ли временные предпочтения
    bool isValid() const;

    // Проверяет, пусты ли временные предпочтения
    bool empty() const;

    // Возвращает начальное время в долях дня
    double getStartTime() const;

    // Возвращает конечное время в долях дня
    double getEndTime() const;

    // Пара строк, представляющая диапазон времени (начало и конец)
    std::pair<std::string, std::string> timeRange;

    // Флаг строгого соответствия временным предпочтениям
    bool isStrict;
};