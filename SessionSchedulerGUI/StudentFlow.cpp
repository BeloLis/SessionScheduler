#include "StudentFlow.h"

// Конструктор с названием потока
StudentFlow::StudentFlow(const std::string& name) : name(name) {}

// Добавляет группу в поток
void StudentFlow::addGroup(const StudentGroup& group) {
    groups.push_back(group);
}

// Возвращает список групп в потоке
const std::vector<StudentGroup>& StudentFlow::getGroups() const {
    return groups;
}