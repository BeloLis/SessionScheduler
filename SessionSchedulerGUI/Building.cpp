#include "Building.h"

// Конструктор
Building::Building(const std::string& building_code)
    : building_code(building_code), building_number(building_code), room_type(normalizeType(building_code)) {
    size_t dashPos = building_code.find('-');
    if (dashPos != std::string::npos) {
        building_number = building_code.substr(0, dashPos);
        room_type = normalizeType(building_code.substr(dashPos + 1));
    }
}

// Нормализует тип аудитории
std::string Building::normalizeType(const std::string& type) const {
    std::string lowerType = type;
    std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);
    if (lowerType == "ка" || lowerType == "ka") return "COMP";
    if (lowerType == "оа" || lowerType == "oa") return "REG";
    return "UNKNOWN";
}

// Добавляет аудиторию в здание
void Building::addClassroom(const std::string& classroom_name, ClassroomType type, const std::string& group_name) {
    classrooms.emplace_back(classroom_name, type, group_name);
}

// Возвращает список аудиторий
const std::vector<Classroom>& Building::getClassrooms() const {
    return classrooms;
}