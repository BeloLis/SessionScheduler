#include "Utils.h"
#include <sstream>
#include <iomanip>
#include <cmath> 

// Утилиты для форматирования данных
namespace Utils {
    // Форматирует время из долей дня в строку HH:MM
    std::string formatTime(double time) {
        // Округляем до ближайшей минуты, чтобы избежать ошибок плавающей точки
        int totalMinutes = static_cast<int>(std::round(time * 24.0 * 60.0));

        // Нормализация на случай выхода за пределы суток
        if (totalMinutes < 0) totalMinutes = 0;
        if (totalMinutes >= 24 * 60) totalMinutes = 24 * 60 - 1;

        int hours = totalMinutes / 60;
        int minutes = totalMinutes % 60;

        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << hours << ":"
            << std::setw(2) << std::setfill('0') << minutes;
        return oss.str();
    }
}