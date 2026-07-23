#pragma once
#include <vector>
#include <string>
#include <map>
#include <queue>
#include "SessionDay.h"
#include "Building.h"
#include "StudentFlow.h"
#include "Exam.h"
#include "GradedExam.h"
#include "FinalExam.h"
#include "Teacher.h"
#include "ScheduleEntry.h"

/// @brief Класс для планирования экзаменов и консультаций.
/// Отвечает за создание расписания с учетом доступности преподавателей, групп и аудиторий.
class Scheduler {
public:
    /// @brief Типы задач для планирования.
    struct ExamTask {
        enum class Type { EXAM, GRADED_EXAM, FINAL_EXAM, CONSULTATION };
        Type type;        ///< Тип задачи (экзамен, зачет, консультация и т.д.).
        size_t index;     ///< Индекс задачи в соответствующем массиве.
        int priority;     ///< Приоритет задачи для сортировки в очереди.

        /// @brief Оператор сравнения для приоритетной очереди.
        bool operator<(const ExamTask& other) const { return priority < other.priority; }
    };

    /// @brief Конструктор класса Scheduler.
    /// @param days Список дней сессии.
    /// @param buildings Список зданий с аудиториями.
    /// @param flows Список потоков студентов.
    /// @param exams Список зачетов и курсовых работ.
    /// @param gradedExams Список зачетов с оценкой и курсовых проектов.
    /// @param finalExams Список итоговых экзаменов.
    /// @param existingSchedule Существующее расписание для учета фиксированных задач.
    Scheduler(const std::vector<SessionDay>& days, const std::vector<Building>& buildings,
        const std::vector<StudentFlow>& flows, const std::vector<Exam>& exams,
        const std::vector<GradedExam>& gradedExams, const std::vector<FinalExam>& finalExams,
        const std::vector<ScheduleEntry>& existingSchedule);

    /// @brief Планирует экзамены и консультации.
    /// @return true, если расписание успешно создано, false в противном случае.
    bool scheduleExams();

    /// @brief Планирует консультацию для указанной группы, предмета и преподавателя.
    /// @param group Название группы или потока.
    /// @param subject Название дисциплины.
    /// @param teacher Имя преподавателя.
    /// @param date Дата консультации.
    /// @param preferredRooms Предпочитаемые аудитории.
    /// @return true, если консультация запланирована, false в противном случае.
    bool scheduleConsultation(const std::string& group, const std::string& subject, const std::string& teacher,
        const std::string& date, const std::vector<std::string>& preferredRooms);

    /// @brief Возвращает текущее расписание.
    const std::vector<ScheduleEntry>& getSchedule() const;

    /// @brief Возвращает список незапланированных задач с причинами.
    const std::vector<std::pair<ScheduleEntry, std::string>>& getUnscheduledTasks() const;

    /// @brief Возвращает прогресс планирования (доля запланированных задач).
    double getProgress() const;

    /// @brief Возвращает процент успешного планирования.
    double getSuccessRate() const;

private:
    std::vector<SessionDay> sessionDays_;     ///< Дни сессии.
    std::vector<Building> buildings_;         ///< Здания с аудиториями.
    std::vector<StudentFlow> flows_;          ///< Потоки студентов.
    std::vector<Exam> exams_;                 ///< Зачеты и курсовые работы.
    std::vector<GradedExam> gradedExams_;     ///< Зачеты с оценкой и курсовые проекты.
    std::vector<FinalExam> finalExams_;       ///< Итоговые экзамены.
    std::vector<ScheduleEntry> existingSchedule_; ///< Существующее расписание.
    std::map<std::string, Teacher> teachers_;     ///< Преподаватели.
    std::vector<ScheduleEntry> schedule_;         ///< Созданное расписание.
    std::vector<std::pair<ScheduleEntry, std::string>> unscheduledTasks_; ///< Незапланированные задачи.
    size_t totalTasks_;                           ///< Общее количество задач.
    size_t scheduledTasks_;                       ///< Количество запланированных задач.

    // Внутренние методы для проверки условий и планирования
    // Объединённый список "занятых" слотов: то, что уже запланировано в этом прогоне
    // (schedule_), плюс фиксированные записи из листа "Итог" (existingSchedule_).
    // Фиксированные записи сами никогда не двигаются (см. isFixed в markFixedTasks),
    // но обязаны учитываться как занятые при подборе слотов для остальных задач.
    std::vector<ScheduleEntry> allBusyEntries() const;
    int calculateTeacherPriority(const std::string& teacher) const;
    bool isTaskInExistingSchedule(const std::string& group, const std::string& subject, const std::string& teacher, const std::string& type) const;
    bool isTeacherAvailable(const std::string& teacher, const std::string& date, double start, double end,
        const std::string& classroom, const std::string& type) const;
    bool isTeacherRestricted(const std::string& teacher, const std::string& date, const std::string& weekday) const;
    bool isClassroomAvailable(const std::string& classroom, const std::string& date, double start, double end) const;
    bool isGroupAvailable(const std::string& groupOrFlow, const std::string& date, double start, double end) const;
    bool hasNoAttestationsBeforeExam(const std::string& group, const std::string& examDate, const std::string& subject, int minDays) const;
    bool isDateBeforeExam(const std::string& consultDate, const std::string& group, const std::string& subject,
        const std::string& teacher, int minDays) const;
    std::vector<std::string> getFlowGroups(const std::string& flowName) const;
    bool satisfiesTimePreference(const TimePreference& pref, double start, double end) const;
    bool isDateAllowed(const std::vector<std::string>& excludedDates, const std::string& date, const std::string& examType, const std::string& group) const;
    std::vector<std::string> getSuitableClassrooms(const std::vector<std::string>& preferredRooms,
        bool isComputer, const std::string& examType) const;
    bool hasOneDayGapAfterAttestation(const std::string& group, const std::string& targetDate,
        const std::string& subject) const;
    std::vector<std::string> getValidConsultationDates(const std::string& examDate, int prepDays) const;
};