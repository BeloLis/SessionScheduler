#pragma once
#include <string>

// Утилиты для форматирования данных
namespace Utils {
    // Форматирует время из долей дня в строку HH:MM
    std::string formatTime(double time);
}