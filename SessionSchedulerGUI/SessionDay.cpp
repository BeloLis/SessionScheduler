#include "SessionDay.h"
#include "DateUtils.h"

/// @brief Конструктор класса SessionDay.
/// @param date Дата в формате строки (например, "DD.MM.YYYY").
/// @param isHoliday Флаг, указывающий, является ли день праздничным.
SessionDay::SessionDay(const std::string& date, bool isHoliday)
    : date(DateUtils::formatDate(date)), isHoliday(isHoliday) {
    // Массив дней недели для преобразования числового индекса в строковое представление
    static const char* weekdays[] = { "Вс", "Пн", "Вт", "Ср", "Чт", "Пт", "Сб" };

    // Получаем индекс дня недели (0 - воскресенье, 1 - понедельник и т.д.)
    int wday = DateUtils::getWeekday(this->date);
    weekday = weekdays[wday];
}
