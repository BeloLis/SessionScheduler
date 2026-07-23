#pragma once
#include <string>

// Структура для представления записи в расписании
struct ScheduleEntry {
    std::string group;      // Название группы или потока
    std::string subject;    // Название дисциплины
    std::string classroom;  // Название аудитории (или "дист" для дистанционного формата)
    std::string date;       // Дата проведения в формате "ДД.ММ.ГГГГ"
    double startTime;       // Время начала в долях дня (например, 0.375 = 9:00)
    double endTime;         // Время окончания в долях дня
    std::string type;       // Тип аттестации ("З", "КР", "ЗаО", "КП", "Э", "Конс")
    std::string teacher;    // Имя преподавателя
};