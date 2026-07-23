#include "Scheduler.h"
#include "DateUtils.h"
#include "Utils.h"
#include <string>
#include <algorithm>
#include <queue>
#include <sstream>
#include <cmath>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>

// --- Логирование ---

// Форматирование временной метки для логов
std::string getCurrentTimestamp() {
    auto now = std::time(nullptr);
    std::tm tm;
    localtime_s(&tm, &now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// Запись лога в файл scheduler.log
void logMessage(const std::string& level, const std::string& message) {
    std::ofstream log("scheduler.log", std::ios::app);
    log << "[" << getCurrentTimestamp() << "] [" << level << "] " << message << "\n";
    log.close();
}

// --- Вспомогательные функции ---

// Нормализация названия дня недели
// @param day Название дня недели (например, "понедельник", "пн", "monday")
// @return Нормализованное сокращение (например, "Пн")
std::string normalizeWeekday(const std::string& day) {
    std::string lowerDay = day;
    std::transform(lowerDay.begin(), lowerDay.end(), lowerDay.begin(), ::tolower);

    if (lowerDay == "понедельник" || lowerDay == "пн" || lowerDay == "monday") return "Пн";
    if (lowerDay == "вторник" || lowerDay == "вт" || lowerDay == "tuesday") return "Вт";
    if (lowerDay == "среда" || lowerDay == "ср" || lowerDay == "wednesday") return "Ср";
    if (lowerDay == "четверг" || lowerDay == "чт" || lowerDay == "thursday") return "Чт";
    if (lowerDay == "пятница" || lowerDay == "пт" || lowerDay == "friday") return "Пт";
    if (lowerDay == "суббота" || lowerDay == "сб" || lowerDay == "saturday") return "Сб";
    if (lowerDay == "воскресенье" || lowerDay == "вс" || lowerDay == "sunday") return "Вс";
    return day; // Возвращаем исходное значение, если не распознано
}

// --- Класс Scheduler ---

// Конструктор класса Scheduler
// Инициализирует данные для планирования и заполняет словарь преподавателей
Scheduler::Scheduler(const std::vector<SessionDay>& days,
    const std::vector<Building>& buildings,
    const std::vector<StudentFlow>& flows,
    const std::vector<Exam>& exams,
    const std::vector<GradedExam>& gradedExams,
    const std::vector<FinalExam>& finalExams,
    const std::vector<ScheduleEntry>& existingSchedule)
    : sessionDays_(days),
    buildings_(buildings),
    flows_(flows),
    exams_(exams),
    gradedExams_(gradedExams),
    finalExams_(finalExams),
    existingSchedule_(existingSchedule),
    totalTasks_(exams.size() + gradedExams.size() + finalExams.size()),
    scheduledTasks_(0) {
    // Логирование инициализации
    std::ostringstream oss;
    oss << "Инициализация Scheduler: дней сессии=" << days.size()
        << ", зданий=" << buildings.size()
        << ", потоков=" << flows.size()
        << ", зачётов/курсовых=" << exams.size()
        << ", зачётов/проектов=" << gradedExams.size()
        << ", экзаменов=" << finalExams.size()
        << ", существующих записей=" << existingSchedule.size();
    logMessage("INFO", oss.str());

    // Логирование списка дней сессии
    oss.str("");
    oss << "Список дней сессии:\n";
    for (const auto& day : sessionDays_) {
        oss << "  - Дата: " << day.date
            << ", Праздник: " << (day.isHoliday ? "да" : "нет")
            << ", День недели: " << day.weekday << "\n";
    }
    logMessage("INFO", oss.str());

    // Заполнение словаря преподавателей
    for (const auto& exam : exams_) {
        if (!exam.teacher.name.empty()) {
            teachers_[exam.teacher.name] = exam.teacher;
        }
    }
    for (const auto& exam : gradedExams_) {
        if (!exam.teacher.name.empty()) {
            teachers_[exam.teacher.name] = exam.teacher;
        }
    }
    for (const auto& exam : finalExams_) {
        if (!exam.teacher.name.empty()) {
            teachers_[exam.teacher.name] = exam.teacher;
        }
    }
}

// Планирование всех экзаменов, зачётов и курсовых
// @return true, если удалось запланировать хотя бы одну задачу
bool Scheduler::scheduleExams() {
    // Очереди задач для декабря и января
    std::priority_queue<ExamTask, std::vector<ExamTask>, std::less<ExamTask>> decemberTasks, januaryTasks,
        decemberFallbackTasks, januaryFallbackTasks;

    // --- Заполнение очередей задач ---

    // Обработка зачётов и курсовых работ
    for (size_t i = 0; i < exams_.size(); ++i) {
        const auto& exam = exams_[i];
        // Пропуск строк-заголовков
        if (exam.group == "Группа" || exam.teacher.name == "Преподаватель" || exam.subject == "Дисциплина") {
            logMessage("INFO", "Пропущена строка (заголовок): Группа='" + exam.group +
                "', Предмет='" + exam.subject +
                "', Преподаватель='" + exam.teacher.name +
                "', Тип='З/КР'");
            continue;
        }
        // Пропуск фиксированных задач
        if (exam.isFixed) {
            logMessage("INFO", "Пропущена задача (фиксированная): Группа='" + exam.group +
                "', Предмет='" + exam.subject +
                "', Преподаватель='" + exam.teacher.name +
                "', Тип='З/КР'");
            continue;
        }
        int priority = calculateTeacherPriority(exam.teacher.name);
        decemberTasks.push({ ExamTask::Type::EXAM, i, priority });
    }

    // Обработка зачётов и курсовых проектов
    for (size_t i = 0; i < gradedExams_.size(); ++i) {
        const auto& exam = gradedExams_[i];
        if (exam.groupOrFlow == "Группа/поток" || exam.teacher.name == "Преподаватель" || exam.subject == "Дисциплина") {
            logMessage("INFO", "Пропущена строка (заголовок): Группа='" + exam.groupOrFlow +
                "', Предмет='" + exam.subject +
                "', Преподаватель='" + exam.teacher.name +
                "', Тип='ЗаО/КП'");
            continue;
        }
        if (exam.isFixed) {
            logMessage("INFO", "Пропущена задача (фиксированная): Группа='" + exam.groupOrFlow +
                "', Предмет='" + exam.subject +
                "', Преподаватель='" + exam.teacher.name +
                "', Тип='ЗаО/КП'");
            continue;
        }
        int priority = calculateTeacherPriority(exam.teacher.name);
        decemberTasks.push({ ExamTask::Type::GRADED_EXAM, i, priority });
    }

    // Обработка экзаменов
    for (size_t i = 0; i < finalExams_.size(); ++i) {
        const auto& exam = finalExams_[i];
        if (exam.groupOrFlow == "Группа/поток" || exam.teacher.name == "Преподаватель" || exam.subject == "Дисциплина") {
            logMessage("INFO", "Пропущена строка (заголовок): Группа='" + exam.groupOrFlow +
                "', Предмет='" + exam.subject +
                "', Преподаватель='" + exam.teacher.name +
                "', Тип='Э'");
            continue;
        }
        if (exam.isFixed) {
            logMessage("INFO", "Пропущена задача (фиксированная): Группа='" + exam.groupOrFlow +
                "', Предмет='" + exam.subject +
                "', Преподаватель='" + exam.teacher.name +
                "', Тип='Э'");
            continue;
        }
        int priority = calculateTeacherPriority(exam.teacher.name);
        januaryTasks.push({ ExamTask::Type::FINAL_EXAM, i, priority });
    }

    // --- Функция планирования задач ---

    auto scheduleTasks = [&](std::priority_queue<ExamTask, std::vector<ExamTask>, std::less<ExamTask>>& tasks,
        bool preferDecember, const std::string& queueName) {
            std::vector<ExamTask> unscheduled;
            logMessage("INFO", "Начало обработки очереди '" + queueName +
                "', preferDecember=" + (preferDecember ? "да" : "нет") +
                ", задач: " + std::to_string(tasks.size()));

            while (!tasks.empty()) {
                auto task = tasks.top();
                tasks.pop();

                // --- Извлечение данных задачи ---
                std::string group, subject, teacher, type;
                std::vector<std::string> preferredRooms, excludedDates;
                TimePreference timePref;
                double duration = 0.0;
                bool isComputer = false;
                bool isDistance = false;
                bool isFixed = false;
                bool consultationFixed = false;
                int prepDays = 0;

                if (task.type == ExamTask::Type::EXAM) {
                    const auto& exam = exams_[task.index];
                    group = exam.group;
                    subject = exam.subject;
                    teacher = exam.teacher.name;
                    preferredRooms = exam.preferredRooms;
                    excludedDates = exam.excludedDates;
                    timePref = exam.timePref;
                    type = exam.type == ExamType::CREDIT ? "З" : "КР";
                    duration = 2.667;
                    isComputer = (type == "КР" || std::any_of(preferredRooms.begin(), preferredRooms.end(),
                        [](const std::string& room) { return room == "3-КА" || room == "11-КА"; }));
                    isDistance = std::find(preferredRooms.begin(), preferredRooms.end(), "дист") != preferredRooms.end();
                    isFixed = exam.isFixed;
                }
                else if (task.type == ExamTask::Type::GRADED_EXAM) {
                    const auto& exam = gradedExams_[task.index];
                    group = exam.groupOrFlow;
                    subject = exam.subject;
                    teacher = exam.teacher.name;
                    preferredRooms = exam.preferredRooms;
                    excludedDates = exam.excludedDates;
                    timePref = exam.timePref;
                    type = exam.type == GradedExamType::GRADED_CREDIT ? "ЗаО" : "КП";
                    duration = 5.0;
                    isComputer = (type == "КП" || type == "ЗаО" || std::any_of(preferredRooms.begin(), preferredRooms.end(),
                        [](const std::string& room) { return room == "3-КА" || room == "11-КА"; }));
                    isDistance = std::find(preferredRooms.begin(), preferredRooms.end(), "дист") != preferredRooms.end();
                    isFixed = exam.isFixed;
                }
                else if (task.type == ExamTask::Type::FINAL_EXAM) {
                    const auto& exam = finalExams_[task.index];
                    group = exam.groupOrFlow;
                    subject = exam.subject;
                    teacher = exam.teacher.name;
                    preferredRooms = exam.preferredRooms;
                    excludedDates = exam.excludedDates;
                    timePref = exam.timePref;
                    type = "Э";
                    isFixed = exam.isFixed;
                    consultationFixed = exam.consultationFixed;
                    bool isPartTime = group.find("в") == 0;
                    duration = isPartTime ? 6.667 : 7.0;
                    if (!exam.examDuration.empty()) {
                        try {
                            size_t plusPos = exam.examDuration.find('+');
                            double hours = std::stod(exam.examDuration.substr(0, plusPos));
                            double prepHours = plusPos != std::string::npos ? std::stod(exam.examDuration.substr(plusPos + 1)) : 0;
                            duration = hours - 2.0;
                            if (prepHours == 16) prepDays = 2;
                            else if (prepHours == 25) prepDays = 3;
                            else if (prepHours == 34) prepDays = 4;
                            else if (prepHours == 43) prepDays = 5;
                            else prepDays = 2;
                        }
                        catch (...) {
                            prepDays = 2;
                        }
                    }
                    else {
                        prepDays = 2;
                    }
                    isComputer = std::any_of(preferredRooms.begin(), preferredRooms.end(),
                        [](const std::string& room) { return room == "3-КА" || room == "11-КА"; });
                    isDistance = std::find(preferredRooms.begin(), preferredRooms.end(), "дист") != preferredRooms.end();
                }

                // Пропуск консультаций
                if (type == "Консультация") {
                    unscheduledTasks_.emplace_back(ScheduleEntry{ group, subject, "", "", 0.0, 0.0, type, teacher },
                        "Консультации пока не обрабатываются");
                    logMessage("INFO", "Пропущена консультация: Группа='" + group +
                        "', Предмет='" + subject +
                        "', Преподаватель='" + teacher +
                        "', Причина='Консультации будут обработаны позже'");
                    continue;
                }

                bool scheduled = false;
                std::vector<std::string> classrooms = getSuitableClassrooms(preferredRooms, isComputer, type);
                if (isDistance || group.find("в") == 0) {
                    classrooms = { "дист" };
                    logMessage("INFO", "Принудительно выбрано 'дист' для группы: Группа='" + group + "'");
                }

                // --- Определение возможных временных интервалов ---

                std::vector<std::pair<double, double>> possibleTimes;
                bool isPartTime = group.find("в") == 0 || isDistance;
                if (isPartTime) {
                    if (type == "З" || type == "КР") {
                        possibleTimes = { {18.5 / 24.0, 21.5 / 24.0}, {14.0 / 24.0, 17.0 / 24.0} };
                    }
                    else if (type == "Э") {
                        possibleTimes = { {15.0 / 24.0, 21.667 / 24.0} };
                    }
                    else if (type == "ЗаО" || type == "КП") {
                        possibleTimes = { {16.0 / 24.0, 21.0 / 24.0} };
                    }
                }
                else {
                    if (type == "З" || type == "КР") {
                        possibleTimes = { {9.0 / 24.0, 11.667 / 24.0}, {12.0 / 24.0, 14.667 / 24.0}, {15.0 / 24.0, 17.667 / 24.0} };
                    }
                    else if (type == "ЗаО" || type == "КП") {
                        possibleTimes = { {9.0 / 24.0, 14.0 / 24.0}, {10.0 / 24.0, 15.0 / 24.0}, {13.0 / 24.0, 18.0 / 24.0} };
                    }
                    else if (type == "Э") {
                        possibleTimes = { {9.0 / 24.0, 16.0 / 24.0}, {10.0 / 24.0, 17.0 / 24.0} };
                    }
                }

                // Логирование начала обработки дат
                std::ostringstream oss;
                oss << "Обработка задачи: Группа='" << group
                    << "', Предмет='" << subject
                    << "', Тип='" << type
                    << "', Преподаватель='" << teacher
                    << "', preferDecember=" << (preferDecember ? "да" : "нет")
                    << ", Очередь='" << queueName << "'";
                logMessage("INFO", oss.str());

                // --- Планирование задачи ---

                for (const auto& day : sessionDays_) {
                    // Проверка допустимости даты
                    if (!isDateAllowed(excludedDates, day.date, type, group)) {
                        logMessage("INFO", "Пропущена дата '" + day.date +
                            "' для группы '" + group + "': в исключённых датах");
                        continue;
                    }

                    std::string weekday = DateUtils::getDayOfWeek(day.date);
                    if (isTeacherRestricted(teacher, day.date, weekday)) {
                        logMessage("INFO", "Пропущена дата '" + day.date +
                            "' для группы '" + group + "': преподаватель '" + teacher + "' ограничен");
                        continue;
                    }

                    bool isDecember = !DateUtils::isJanuary(day.date);
                    if (preferDecember && !isDecember) {
                        logMessage("INFO", "Пропущена дата '" + day.date +
                            "' для группы '" + group + "': не декабрь (preferDecember=true)");
                        continue;
                    }
                    if (!preferDecember && isDecember) {
                        logMessage("INFO", "Пропущена дата '" + day.date +
                            "' для группы '" + group + "': декабрь (preferDecember=false)");
                        continue;
                    }

                    // Проверка 1-дневного интервала для З, КР, ЗаО, КП
                    if ((type == "З" || type == "КР" || type == "ЗаО" || type == "КП") &&
                        !hasOneDayGapAfterAttestation(group, day.date, subject)) {
                        logMessage("INFO", "Пропущена дата '" + day.date +
                            "' для группы '" + group + "': нарушение 1-дневного интервала");
                        continue;
                    }

                    // Проверка временных интервалов
                    for (const auto& time : possibleTimes) {
                        double start = time.first;
                        double end = time.second;
                        if (!satisfiesTimePreference(timePref, start, end)) {
                            logMessage("INFO", "Пропущено время " + Utils::formatTime(start) + "-" +
                                Utils::formatTime(end) + " для группы '" + group +
                                "': не соответствует предпочтениям времени");
                            continue;
                        }
                        if (!isTeacherAvailable(teacher, day.date, start, end, "", type)) {
                            logMessage("INFO", "Пропущено время " + Utils::formatTime(start) + "-" +
                                Utils::formatTime(end) + " для группы '" + group +
                                "': преподаватель '" + teacher + "' занят");
                            continue;
                        }
                        if (!isGroupAvailable(group, day.date, start, end)) {
                            logMessage("INFO", "Пропущено время " + Utils::formatTime(start) + "-" +
                                Utils::formatTime(end) + " для группы '" + group +
                                "': группа занята");
                            continue;
                        }
                        if (type == "Э" && !hasNoAttestationsBeforeExam(group, day.date, subject, prepDays)) {
                            logMessage("INFO", "Пропущена дата '" + day.date +
                                "' для группы '" + group + "': нарушение требования prepDays=" +
                                std::to_string(prepDays));
                            continue;
                        }

                        // Планирование консультации для экзамена
                        bool canScheduleConsultation = true;
                        std::string consultDate;
                        if (type == "Э") {
                            if (consultationFixed) {
                                // Консультация для этой группы/предмета/преподавателя уже
                                // зафиксирована строкой "Конс" в листе "Итог" (см.
                                // markFixedTasks) - повторно её не планируем. Сама она уже
                                // учтена как занятый слот через allBusyEntries().
                                canScheduleConsultation = true;
                                logMessage("INFO", "Консультация уже зафиксирована в листе Итог, повторное планирование пропущено: Группа='" +
                                    group + "', Предмет='" + subject + "', Преподаватель='" + teacher + "'");
                            }
                            else {
                                auto validConsultDates = getValidConsultationDates(day.date, prepDays);
                                canScheduleConsultation = false;
                                for (const auto& date : validConsultDates) {
                                    if (scheduleConsultation(group, subject, teacher, date, preferredRooms)) {
                                        canScheduleConsultation = true;
                                        consultDate = date;
                                        break;
                                    }
                                }
                                if (!canScheduleConsultation) {
                                    logMessage("INFO", "Пропущена дата '" + day.date +
                                        "' для группы '" + group +
                                        "': не удалось запланировать консультацию, prepDays=" +
                                        std::to_string(prepDays));
                                    continue;
                                }
                            }
                        }

                        // Поиск подходящей аудитории
                        bool classroomFound = false;
                        for (const auto& classroom : classrooms) {
                            bool isOverlapAllowed = false;
                            for (const auto& entry : allBusyEntries()) {
                                if (entry.classroom == classroom && entry.date == day.date &&
                                    entry.startTime < end && entry.endTime > start &&
                                    entry.teacher == teacher && entry.subject == subject) {
                                    isOverlapAllowed = true;
                                    break;
                                }
                            }
                            if (!isOverlapAllowed && !isClassroomAvailable(classroom, day.date, start, end)) {
                                logMessage("INFO", "Пропущена аудитория '" + classroom +
                                    "' на дату '" + day.date + "' для группы '" + group +
                                    "' в " + Utils::formatTime(start) + "-" + Utils::formatTime(end) +
                                    ": занята");
                                continue;
                            }
                            // Добавление записи в расписание
                            schedule_.push_back({
                                group, subject, classroom, day.date, start, end, type, teacher
                                });
                            scheduled = true;
                            scheduledTasks_++;
                            classroomFound = true;
                            logMessage("INFO", "Запланирована задача: Группа='" + group +
                                "', Предмет='" + subject +
                                "', Тип='" + type +
                                "', Дата='" + day.date +
                                "', Время=" + Utils::formatTime(start) + "-" + Utils::formatTime(end) +
                                ", Аудитория='" + classroom +
                                "', Преподаватель='" + teacher +
                                "', Очно-заочная=" + (isPartTime ? "да" : "нет") +
                                ", Нахлёст=" + (isOverlapAllowed ? "да" : "нет"));
                            break;
                        }

                        if (classroomFound) break;
                    }
                    if (scheduled) break;
                }

                // Обработка незапланированных задач
                if (!scheduled) {
                    std::string reason;
                    if (queueName == "decemberTasks") {
                        reason = "Не удалось запланировать в декабре, перенос в январь";
                        januaryFallbackTasks.push({ task.type, task.index, task.priority - 50 });
                    }
                    else if (queueName == "januaryTasks" && task.type == ExamTask::Type::FINAL_EXAM) {
                        reason = "Не удалось запланировать в январе, перенос в декабрь";
                        decemberFallbackTasks.push({ task.type, task.index, task.priority - 100 });
                    }
                    else {
                        reason = "Не удалось найти подходящее время или аудиторию";
                        unscheduledTasks_.emplace_back(ScheduleEntry{ group, subject, "", "", 0.0, 0.0, type, teacher }, reason);
                    }
                    logMessage("WARN", "Не запланирована задача: Группа='" + group +
                        "', Предмет='" + subject +
                        "', Тип='" + type +
                        "', Преподаватель='" + teacher +
                        "', Причина='" + reason +
                        "', Очередь='" + queueName + "'");
                }
            }
            return unscheduled;
        };

    // --- Планирование задач по очередям ---

    auto unscheduledDecember = scheduleTasks(decemberTasks, true, "decemberTasks");
    auto unscheduledJanuary = scheduleTasks(januaryTasks, false, "januaryTasks");
    auto unscheduledJanuaryFallback = scheduleTasks(januaryFallbackTasks, false, "januaryFallbackTasks");
    auto unscheduledDecemberFallback = scheduleTasks(decemberFallbackTasks, true, "decemberFallbackTasks");

    // --- Финальная попытка планирования ---

    std::priority_queue<ExamTask, std::vector<ExamTask>, std::less<ExamTask>> finalAttemptTasks;
    for (const auto& task : unscheduledDecember) {
        finalAttemptTasks.push(task);
    }
    for (const auto& task : unscheduledJanuary) {
        finalAttemptTasks.push(task);
    }
    for (const auto& task : unscheduledJanuaryFallback) {
        finalAttemptTasks.push(task);
    }
    for (const auto& task : unscheduledDecemberFallback) {
        finalAttemptTasks.push(task);
    }
    auto finalUnscheduled = scheduleTasks(finalAttemptTasks, false, "finalAttemptTasks");

    // --- Добавление незапланированных задач в список ---

    auto addUnscheduledTasks = [&](const std::vector<ExamTask>& tasks, const std::string& reason) {
        for (const auto& task : tasks) {
            std::string group, subject, teacher, type;
            if (task.type == ExamTask::Type::EXAM) {
                const auto& exam = exams_[task.index];
                group = exam.group;
                subject = exam.subject;
                teacher = exam.teacher.name;
                type = exam.type == ExamType::CREDIT ? "З" : "КР";
            }
            else if (task.type == ExamTask::Type::GRADED_EXAM) {
                const auto& exam = gradedExams_[task.index];
                group = exam.groupOrFlow;
                subject = exam.subject;
                teacher = exam.teacher.name;
                type = exam.type == GradedExamType::GRADED_CREDIT ? "ЗаО" : "КП";
            }
            else if (task.type == ExamTask::Type::FINAL_EXAM) {
                const auto& exam = finalExams_[task.index];
                group = exam.groupOrFlow;
                subject = exam.subject;
                teacher = exam.teacher.name;
                type = "Э";
            }
            unscheduledTasks_.emplace_back(ScheduleEntry{ group, subject, "", "", 0.0, 0.0, type, teacher }, reason);
            logMessage("WARN", "Добавлена незапланированная задача: Группа='" + group +
                "', Предмет='" + subject +
                "', Тип='" + type +
                "', Преподаватель='" + teacher +
                "', Причина='" + reason + "'");
        }
        };

    addUnscheduledTasks(unscheduledDecember, "Не удалось запланировать в декабре");
    addUnscheduledTasks(unscheduledJanuary, "Не удалось запланировать в январе");
    addUnscheduledTasks(unscheduledJanuaryFallback, "Не удалось запланировать в январе (перенос)");
    addUnscheduledTasks(unscheduledDecemberFallback, "Не удалось запланировать в декабре (резерв)");
    addUnscheduledTasks(finalUnscheduled, "Не удалось запланировать в финальной попытке");

    logMessage("INFO", "Планирование завершено: Запланировано задач=" + std::to_string(scheduledTasks_) +
        ", Не запланировано задач=" + std::to_string(unscheduledTasks_.size()));
    return !schedule_.empty();
}

// Получение подходящих аудиторий для задачи
// @param preferredRooms Предпочитаемые аудитории
// @param isComputer Требуется ли компьютерная аудитория
// @param examType Тип экзамена (З, КР, ЗаО, КП, Э, Конс)
// @return Список подходящих аудиторий
std::vector<std::string> Scheduler::getSuitableClassrooms(const std::vector<std::string>& preferredRooms,
    bool isComputer,
    const std::string& examType) const {
    std::vector<std::string> classrooms;
    bool preferComputer = isComputer || examType == "КР" || examType == "КП" || examType == "ЗаО";
    bool isDistance = std::find(preferredRooms.begin(), preferredRooms.end(), "дист") != preferredRooms.end();

    std::vector<std::pair<std::string, int>> prioritizedClassrooms;

    // Логирование начала выбора аудиторий
    std::ostringstream oss;
    oss << "Выбор аудиторий: Тип='" << examType
        << "', Предпочтения={";
    for (const auto& room : preferredRooms) oss << room << ", ";
    oss << "}, Компьютерные=" << (preferComputer ? "да" : "нет")
        << ", Дистанционно=" << (isDistance ? "да" : "нет");
    logMessage("INFO", oss.str());

    // --- Поиск по предпочтительным аудиториям ---

    for (const auto& pref : preferredRooms) {
        if (pref == "дист") {
            if (isDistance) {
                prioritizedClassrooms.push_back({ "дист", 0 });
                logMessage("INFO", "Добавлена аудитория: дистанционно ('дист')");
            }
            continue;
        }
        for (const auto& building : buildings_) {
            bool isGroupMatch = building.getCode() == pref ||
                std::any_of(building.getClassrooms().begin(), building.getClassrooms().end(),
                    [&pref](const Classroom& room) { return room.groupName == pref; });
            if (isGroupMatch) {
                for (const auto& room : building.getClassrooms()) {
                    if (building.getCode() != pref && room.groupName != pref) continue;
                    bool isRoomComputer = (room.type == ClassroomType::COMPUTER);
                    if (preferComputer && !isRoomComputer && examType != "Э" && examType != "Конс") {
                        logMessage("INFO", "Пропущена аудитория '" + room.name +
                            "': требуется компьютерная");
                        continue;
                    }
                    if (!preferComputer && isRoomComputer && examType == "З") {
                        logMessage("INFO", "Разрешена компьютерная аудитория '" + room.name + "' для зачёта");
                    }
                    int priority = (building.getCode() == "3-КА" ? 0 : (building.getCode() == "11-КА" ? 1 : 2));
                    prioritizedClassrooms.push_back({ room.name, priority });
                    logMessage("INFO", "Добавлена аудитория: '" + room.name +
                        "', Корпус='" + building.getCode() +
                        "', Группа='" + room.groupName +
                        "', Тип=" + (isRoomComputer ? "COMPUTER" : "REGULAR") +
                        ", Приоритет=" + std::to_string(priority));
                }
            }
        }
    }

    // --- Поиск по всем аудиториям, если предпочтения не найдены ---

    if (prioritizedClassrooms.empty() && !isDistance) {
        logMessage("INFO", "Не найдены аудитории для предпочтений, поиск по всем корпусам");
        for (const auto& building : buildings_) {
            for (const auto& room : building.getClassrooms()) {
                bool isRoomComputer = (room.type == ClassroomType::COMPUTER);
                bool isSuitable = true;
                if (preferComputer && !isRoomComputer && examType != "Э" && examType != "Конс") {
                    isSuitable = false;
                    logMessage("INFO", "Пропущена аудитория '" + room.name +
                        "': требуется компьютерная");
                }
                if (!preferComputer && isRoomComputer && examType == "З") {
                    logMessage("INFO", "Разрешена компьютерная аудитория '" + room.name + "' для зачёта");
                }
                if (isSuitable) {
                    int priority = (building.getCode() == "3-КА" ? 0 : (building.getCode() == "11-КА" ? 1 : 2));
                    prioritizedClassrooms.push_back({ room.name, priority });
                    logMessage("INFO", "Добавлена аудитория (без предпочтений): '" + room.name +
                        "', Корпус='" + building.getCode() +
                        "', Группа='" + room.groupName +
                        "', Тип=" + (isRoomComputer ? "COMPUTER" : "REGULAR") +
                        ", Приоритет=" + std::to_string(priority));
                }
            }
        }
    }

    // Fallback на дистанционный формат
    if (prioritizedClassrooms.empty() && isDistance) {
        prioritizedClassrooms.push_back({ "дист", 0 });
        logMessage("INFO", "Добавлена аудитория: дистанционно ('дист') по умолчанию");
    }

    // Поиск компьютерных аудиторий для КР/КП/ЗаО
    if (prioritizedClassrooms.empty() && !isDistance && preferComputer && (examType == "КР" || examType == "КП" || examType == "ЗаО")) {
        logMessage("INFO", "Поиск всех компьютерных аудиторий для КР/КП/ЗаО");
        for (const auto& building : buildings_) {
            for (const auto& room : building.getClassrooms()) {
                if (room.type == ClassroomType::COMPUTER) {
                    int priority = (building.getCode() == "3-КА" ? 0 : (building.getCode() == "11-КА" ? 1 : 2));
                    prioritizedClassrooms.push_back({ room.name, priority });
                    logMessage("INFO", "Добавлена компьютерная аудитория: '" + room.name +
                        "', Корпус='" + building.getCode() +
                        "', Группа='" + room.groupName +
                        "', Приоритет=" + std::to_string(priority));
                }
            }
        }
    }

    // Сортировка аудиторий по приоритету
    std::sort(prioritizedClassrooms.begin(), prioritizedClassrooms.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    for (const auto& pair : prioritizedClassrooms) {
        classrooms.push_back(pair.first);
    }

    // Логирование результата
    oss.str("");
    if (classrooms.empty()) {
        oss << "Аудитории не найдены";
    }
    else {
        oss << "Найдены аудитории: {";
        for (size_t i = 0; i < classrooms.size(); ++i) {
            oss << classrooms[i];
            if (i < classrooms.size() - 1) oss << ", ";
        }
        oss << "}";
    }
    logMessage("INFO", oss.str());

    return classrooms;
}

// Планирование консультации для экзамена
// @param group Группа или поток
// @param subject Дисциплина
// @param teacher Преподаватель
// @param date Дата консультации
// @param preferredRooms Предпочитаемые аудитории
// @return true, если консультация запланирована
bool Scheduler::scheduleConsultation(const std::string& group,
    const std::string& subject,
    const std::string& teacher,
    const std::string& date,
    const std::vector<std::string>& preferredRooms) {
    std::ostringstream oss;
    oss << "Планирование консультации: Группа='" << group
        << "', Предмет='" << subject
        << "', Преподаватель='" << teacher
        << "', Дата='" << date << "'";
    logMessage("INFO", oss.str());

    // Определение формата обучения
    bool isPartTime = group.find("в") == 0;
    bool isDistance = std::find(preferredRooms.begin(), preferredRooms.end(), "дист") != preferredRooms.end();

    // Проверка допустимости даты
    bool isHoliday = false;
    bool isSunday = DateUtils::isSunday(date);
    for (const auto& day : sessionDays_) {
        if (day.date == date && day.isHoliday) {
            isHoliday = true;
            break;
        }
    }
    if (isHoliday || isSunday) {
        logMessage("INFO", "Пропущена консультация на дату '" + date +
            "': Праздник=" + (isHoliday ? "да" : "нет") +
            ", Воскресенье=" + (isSunday ? "да" : "нет"));
        return false;
    }

    // Определение времени консультации
    double startTime, endTime;
    if (isPartTime) {
        startTime = 18.5 / 24.0; // 18:30
        endTime = 20.0 / 24.0;   // 20:00
    }
    else {
        startTime = 16.0 / 24.0; // 16:00
        endTime = 18.0 / 24.0;   // 18:00
    }

    // Проверка доступности преподавателя
    if (!isTeacherAvailable(teacher, date, startTime, endTime, "", "Конс")) {
        logMessage("INFO", "Пропущена консультация на дату '" + date +
            "' в " + Utils::formatTime(startTime) + "-" + Utils::formatTime(endTime) +
            ": преподаватель '" + teacher + "' занят");
        return false;
    }

    // Проверка доступности группы
    if (!isGroupAvailable(group, date, startTime, endTime)) {
        logMessage("INFO", "Пропущена консультация на дату '" + date +
            "' в " + Utils::formatTime(startTime) + "-" + Utils::formatTime(endTime) +
            ": группа '" + group + "' занята");
        return false;
    }

    // Выбор аудитории
    std::string selectedClassroom = isDistance ? "дист" : "";
    if (!isDistance) {
        std::vector<std::string> classrooms = getSuitableClassrooms(preferredRooms, false, "Конс");
        if (!classrooms.empty()) {
            selectedClassroom = classrooms[0];
        }
    }
    if (selectedClassroom.empty()) {
        selectedClassroom = "дист";
        logMessage("INFO", "Аудитория не найдена для консультации, используется 'дист'");
    }

    // Проверка доступности аудитории
    if (!isClassroomAvailable(selectedClassroom, date, startTime, endTime)) {
        logMessage("INFO", "Пропущена консультация на дату '" + date +
            "' в " + Utils::formatTime(startTime) + "-" + Utils::formatTime(endTime) +
            ": аудитория '" + selectedClassroom + "' занята");
        return false;
    }

    // Добавление консультации в расписание
    schedule_.push_back({
        group, subject, selectedClassroom, date, startTime, endTime, "Конс", teacher
        });
    logMessage("INFO", "Запланирована консультация: Группа='" + group +
        "', Предмет='" + subject +
        "', Дата='" + date +
        "', Время=" + Utils::formatTime(startTime) + "-" + Utils::formatTime(endTime) +
        ", Аудитория='" + selectedClassroom +
        "', Преподаватель='" + teacher + "'");
    return true;
}

// Получение текущего расписания
// @return Вектор записей расписания
const std::vector<ScheduleEntry>& Scheduler::getSchedule() const {
    return schedule_;
}

// Получение списка незапланированных задач
// @return Вектор пар (запись, причина)
const std::vector<std::pair<ScheduleEntry, std::string>>& Scheduler::getUnscheduledTasks() const {
    return unscheduledTasks_;
}

// Получение прогресса планирования
// @return Доля запланированных задач (0.0 - 1.0)
double Scheduler::getProgress() const {
    return totalTasks_ > 0 ? static_cast<double>(scheduledTasks_) / totalTasks_ : 0.0;
}

// Получение процента успешного планирования
// @return Процент запланированных задач (0.0 - 100.0)
double Scheduler::getSuccessRate() const {
    return totalTasks_ > 0 ? static_cast<double>(scheduledTasks_) / totalTasks_ * 100.0 : 0.0;
}

// Расчёт приоритета преподавателя на основе количества задач
// @param teacher Имя преподавателя
// @return Отрицательное количество задач (для приоритета в очереди)
int Scheduler::calculateTeacherPriority(const std::string& teacher) const {
    int count = 0;
    for (const auto& exam : exams_) {
        if (exam.teacher.name == teacher) count++;
    }
    for (const auto& exam : gradedExams_) {
        if (exam.teacher.name == teacher) count++;
    }
    for (const auto& exam : finalExams_) {
        if (exam.teacher.name == teacher) count++;
    }
    return -count;
}

// Объединённый список занятых слотов: новые назначения (schedule_) + фиксированные
// записи из листа "Итог" (existingSchedule_). Используется всеми проверками занятости,
// чтобы фиксированные аттестации реально блокировали аудитории/преподавателей/группы,
// а не просто помечались как isFixed и молча игнорировались при поиске конфликтов.
std::vector<ScheduleEntry> Scheduler::allBusyEntries() const {
    std::vector<ScheduleEntry> all;
    all.reserve(schedule_.size() + existingSchedule_.size());
    all.insert(all.end(), schedule_.begin(), schedule_.end());
    all.insert(all.end(), existingSchedule_.begin(), existingSchedule_.end());
    return all;
}

// Проверка наличия задачи в существующем расписании
// @return false (всегда, так как метод заглушка)
bool Scheduler::isTaskInExistingSchedule(const std::string& group,
    const std::string& subject,
    const std::string& teacher,
    const std::string& type) const {
    return false;
}

// Проверка доступности преподавателя
// @param teacher Имя преподавателя
// @param date Дата
// @param start Время начала
// @param end Время окончания
// @param classroom Аудитория
// @param type Тип аттестации
// @return true, если преподаватель доступен
bool Scheduler::isTeacherAvailable(const std::string& teacher,
    const std::string& date,
    double start,
    double end,
    const std::string& classroom,
    const std::string& type) const {
    int countZKR = 0, countZaOKP = 0, countE = 0;
    double totalHours = 0.0;
    bool isEvening = start >= 14.0 / 24.0;

    for (const auto& entry : allBusyEntries()) {
        if (entry.teacher != teacher || entry.date != date) continue;
        if (entry.startTime < end && entry.endTime > start) {
            logMessage("WARN", "Конфликт времени для преподавателя '" + teacher +
                "' на дату '" + date +
                "': существующая аттестация с " + Utils::formatTime(entry.startTime) +
                " до " + Utils::formatTime(entry.endTime) +
                ", Группа='" + entry.group +
                "', Предмет='" + entry.subject +
                "', Тип='" + entry.type +
                "', Новая аттестация с " + Utils::formatTime(start) +
                " до " + Utils::formatTime(end));
            return false;
        }
        if (entry.type == "З" || entry.type == "КР") countZKR++;
        else if (entry.type == "ЗаО" || entry.type == "КП") countZaOKP++;
        else if (entry.type == "Э") countE++;
        totalHours += (entry.endTime - entry.startTime) * 24.0;
    }

    std::ostringstream oss;
    oss << "Проверка доступности преподавателя '" << teacher
        << "' на дату '" << date
        << "' в " << Utils::formatTime(start) << "-" << Utils::formatTime(end)
        << ": З/КР=" << countZKR
        << ", ЗаО/КП=" << countZaOKP
        << ", Э=" << countE
        << ", Всего часов=" << totalHours
        << ", Вечер=" << (isEvening ? "да" : "нет");
    logMessage("INFO", oss.str());

    if (type != "Конс") {
        if ((type == "З" || type == "КР") && countZKR >= (isEvening ? 2 : 3)) {
            logMessage("WARN", "Преподаватель '" + teacher +
                "' превысил лимит З/КР на дату '" + date +
                "': countZKR=" + std::to_string(countZKR) +
                ", isEvening=" + (isEvening ? "да" : "нет"));
            return false;
        }
        if ((type == "ЗаО" || type == "КП") && countZaOKP >= (isEvening ? 1 : 2)) {
            logMessage("WARN", "Преподаватель '" + teacher +
                "' превысил лимит ЗаО/КП на дату '" + date +
                "': countZaOKP=" + std::to_string(countZaOKP) +
                ", isEvening=" + (isEvening ? "да" : "нет"));
            return false;
        }
        if (type == "Э" && countE >= (isEvening ? 1 : 2)) {
            logMessage("WARN", "Преподаватель '" + teacher +
                "' превысил лимит Э на дату '" + date +
                "': countE=" + std::to_string(countE) +
                ", isEvening=" + (isEvening ? "да" : "нет"));
            return false;
        }
    }

    totalHours += (end - start) * 24.0;
    if (totalHours > 9.0) {
        logMessage("WARN", "Преподаватель '" + teacher +
            "' превысил лимит часов на дату '" + date +
            "': totalHours=" + std::to_string(totalHours));
        return false;
    }

    return true;
}

// Проверка ограничений преподавателя
// @param teacher Имя преподавателя
// @param date Дата
// @param weekday День недели
// @return true, если преподаватель ограничен
bool Scheduler::isTeacherRestricted(const std::string& teacher,
    const std::string& date,
    const std::string& weekday) const {
    auto it = teachers_.find(teacher);
    if (it == teachers_.end()) {
        logMessage("INFO", "Преподаватель '" + teacher +
            "' не найден для проверки ограничений на дату '" + date + "'");
        return false;
    }

    const Teacher& t = it->second;
    bool restricted = false;
    std::string reason;

    std::string normalizedWeekday = normalizeWeekday(weekday);

    for (const auto& day : t.unavailableDays) {
        std::string normalizedDay = normalizeWeekday(day);
        if (normalizedDay == normalizedWeekday) {
            restricted = true;
            reason = "день недели '" + day + "' в unavailableDays (нормализован как '" + normalizedDay + "')";
            break;
        }
    }

    if (!restricted) {
        for (const auto& range : t.dateRanges) {
            if (DateUtils::isDateInRange(date, range.first, range.second)) {
                restricted = true;
                reason = "дата в диапазоне [" + range.first + "-" + range.second + "]";
                break;
            }
        }
    }

    std::ostringstream oss;
    oss << "Проверка ограничений преподавателя '" << teacher
        << "' на дату '" << date
        << "', День недели='" << weekday
        << "' (нормализован как '" << normalizedWeekday << "'): "
        << (restricted ? "ограничен" : "не ограничен");
    if (restricted) {
        oss << ", Причина='" << reason << "'";
    }
    oss << ", unavailableDays={";
    if (t.unavailableDays.empty()) {
        oss << "пусто";
    }
    else {
        for (size_t i = 0; i < t.unavailableDays.size(); ++i) {
            std::string normalizedDay = normalizeWeekday(t.unavailableDays[i]);
            oss << t.unavailableDays[i] << " (нормализован как '" << normalizedDay << "')";
            if (i < t.unavailableDays.size() - 1) oss << ", ";
        }
    }
    oss << "}, dateRanges={";
    if (t.dateRanges.empty()) {
        oss << "пусто";
    }
    else {
        for (size_t i = 0; i < t.dateRanges.size(); ++i) {
            oss << "[" << t.dateRanges[i].first << "-" << t.dateRanges[i].second << "]";
            if (i < t.dateRanges.size() - 1) oss << ", ";
        }
    }
    oss << "}";
    logMessage("INFO", oss.str());

    return restricted;
}

// Проверка доступности аудитории
// @param classroom Название аудитории
// @param date Дата
// @param start Время начала
// @param end Время окончания
// @return true, если аудитория свободна
bool Scheduler::isClassroomAvailable(const std::string& classroom,
    const std::string& date,
    double start,
    double end) const {
    if (classroom == "дист") return true; // Исправлено: "дист" вместо "ди둗"
    for (const auto& entry : allBusyEntries()) {
        if (entry.classroom == classroom && entry.date == date && entry.startTime < end && entry.endTime > start) {
            return false;
        }
    }
    return true;
}

// Проверка доступности группы
// @param groupOrFlow Название группы или потока
// @param date Дата
// @param start Время начала
// @param end Время окончания
// @return true, если группа свободна
bool Scheduler::isGroupAvailable(const std::string& groupOrFlow,
    const std::string& date,
    double start,
    double end) const {
    std::vector<std::string> groups = getFlowGroups(groupOrFlow);
    if (groups.empty()) groups.push_back(groupOrFlow);
    int countZKR = 0, countZaOKP = 0, countE = 0;
    std::vector<std::string> conflicts;

    for (const auto& entry : allBusyEntries()) {
        if (entry.date != date) continue;
        for (const auto& group : groups) {
            if (entry.group == group && entry.startTime < end && entry.endTime > start) {
                if (entry.type == "З" || entry.type == "КР") countZKR++;
                else if (entry.type == "ЗаО" || entry.type == "КП") countZaOKP++;
                else if (entry.type == "Э") countE++;
                if (entry.type == "Э" || entry.type == "ЗаО" || entry.type == "КП") {
                    conflicts.push_back("Конфликт времени: Существующая аттестация с " + Utils::formatTime(entry.startTime) +
                        " до " + Utils::formatTime(entry.endTime) +
                        ", Предмет='" + entry.subject +
                        "', Преподаватель='" + entry.teacher +
                        "', Тип='" + entry.type + "'");
                }
            }
        }
    }

    std::ostringstream oss;
    oss << "Проверка доступности группы '" << groupOrFlow
        << "' на дату '" << date
        << "' в " << Utils::formatTime(start) << "-" << Utils::formatTime(end)
        << ": З/КР=" << countZKR
        << ", ЗаО/КП=" << countZaOKP
        << ", Э=" << countE;
    logMessage("INFO", oss.str());

    if (!conflicts.empty()) {
        for (const auto& conflict : conflicts) {
            logMessage("WARN", conflict);
        }
        return false;
    }
    if (countZKR >= 2) {
        logMessage("WARN", "Группа '" + groupOrFlow +
            "' превысила лимит З/КР на дату '" + date +
            "': countZKR=" + std::to_string(countZKR));
        return false;
    }
    if (countZaOKP >= 1) {
        logMessage("WARN", "Группа '" + groupOrFlow +
            "' превысила лимит ЗаО/КП на дату '" + date +
            "': countZaOKP=" + std::to_string(countZaOKP));
        return false;
    }
    if (countE >= 1) {
        logMessage("WARN", "Группа '" + groupOrFlow +
            "' превысила лимит Э на дату '" + date +
            "': countE=" + std::to_string(countE));
        return false;
    }
    logMessage("INFO", "Группа '" + groupOrFlow + "' доступна на дату '" + date + "'");
    return true;
}

// Проверка отсутствия аттестаций перед экзаменом
// @param group Группа
// @param examDate Дата экзамена
// @param subject Дисциплина
// @param minDays Минимальное количество дней подготовки
// @return true, если нет аттестаций в период подготовки
bool Scheduler::hasNoAttestationsBeforeExam(const std::string& group,
    const std::string& examDate,
    const std::string& subject,
    int minDays) const {
    for (const auto& entry : allBusyEntries()) {
        if (entry.group != group || entry.subject == subject) continue;
        if (entry.type == "Конс") continue;
        int daysDiff = DateUtils::daysBetween(entry.date, examDate);
        if (daysDiff >= 0 && daysDiff < minDays) {
            logMessage("WARN", "Нарушение prepDays=" + std::to_string(minDays) +
                " для группы '" + group +
                "' перед экзаменом на дату '" + examDate +
                "': найдена аттестация на дату '" + entry.date +
                "', Тип='" + entry.type + "'");
            return false;
        }
    }
    return true;
}

// Проверка наличия 1-дневного интервала после аттестации
// @param group Группа
// @param targetDate Целевая дата
// @param subject Дисциплина
// @return true, если интервал соблюден
bool Scheduler::hasOneDayGapAfterAttestation(const std::string& group,
    const std::string& targetDate,
    const std::string& subject) const {
    for (const auto& entry : allBusyEntries()) {
        if (entry.group != group || entry.subject == subject || entry.type == "Конс") continue;
        int daysDiff = DateUtils::daysBetween(entry.date, targetDate);
        if (daysDiff == 0) {
            logMessage("WARN", "Нарушение 1-дневного интервала для группы '" + group +
                "' на дату '" + targetDate +
                "': найдена аттестация на ту же дату, Тип='" + entry.type + "'");
            return false;
        }
    }
    return true;
}

// Проверка, что консультация запланирована перед экзаменом
// @param consultDate Дата консультации
// @param group Группа
// @param subject Дисциплина
// @param teacher Преподаватель
// @param minDays Минимальное количество дней подготовки
// @return true, если консультация корректна
bool Scheduler::isDateBeforeExam(const std::string& consultDate,
    const std::string& group,
    const std::string& subject,
    const std::string& teacher,
    int minDays) const {
    for (const auto& entry : schedule_) {
        if (entry.group == group && entry.subject == subject && entry.teacher == teacher && entry.type == "Э") {
            std::string expectedConsultDate = DateUtils::getPreviousDay(entry.date);
            if (expectedConsultDate.empty()) {
                logMessage("WARN", "Не удалось определить дату накануне для экзамена на '" + entry.date + "'");
                return false;
            }
            if (consultDate == expectedConsultDate) {
                return true;
            }
        }
    }
    logMessage("WARN", "Не найден экзамен для группы '" + group +
        "', предмета '" + subject +
        "', преподавателя '" + teacher +
        "' на следующий день после '" + consultDate + "'");
    return false;
}

// Получение допустимых дат для консультации
// @param examDate Дата экзамена
// @param prepDays Количество дней подготовки
// @return Список допустимых дат
std::vector<std::string> Scheduler::getValidConsultationDates(const std::string& examDate,
    int prepDays) const {
    std::vector<std::string> validDates;
    std::ostringstream oss;
    oss << "Поиск допустимых дат консультации для экзамена на '" << examDate
        << "', prepDays=" << prepDays;
    logMessage("INFO", oss.str());

    for (int i = 1; i <= prepDays; ++i) {
        std::string consultDate = DateUtils::getPreviousDay(examDate, i);
        if (consultDate.empty()) {
            logMessage("WARN", "Не удалось получить дату за " + std::to_string(i) +
                " дней до '" + examDate + "'");
            continue;
        }

        bool isHoliday = false;
        bool isSunday = DateUtils::isSunday(consultDate);
        for (const auto& day : sessionDays_) {
            if (day.date == consultDate && day.isHoliday) {
                isHoliday = true;
                break;
            }
        }

        if (isHoliday || isSunday) {
            logMessage("INFO", "Пропущена дата '" + consultDate +
                "': Праздник=" + (isHoliday ? "да" : "нет") +
                ", Воскресенье=" + (isSunday ? "да" : "нет"));
            continue;
        }

        validDates.push_back(consultDate);
        logMessage("INFO", "Добавлена допустимая дата консультации: '" + consultDate + "'");
    }

    oss.str("");
    oss << "Итоговый список допустимых дат консультации: {";
    if (validDates.empty()) {
        oss << "пусто";
    }
    else {
        for (size_t i = 0; i < validDates.size(); ++i) {
            oss << validDates[i];
            if (i < validDates.size() - 1) oss << ", ";
        }
    }
    oss << "}";
    logMessage("INFO", oss.str());
    return validDates;
}

// Получение групп в потоке
// @param flowName Название потока
// @return Пустой список (заглушка)
std::vector<std::string> Scheduler::getFlowGroups(const std::string& flowName) const {
    return {};
}

// Проверка соответствия временных предпочтений
// @param pref Предпочтения по времени
// @param start Время начала
// @param end Время окончания
// @return true (заглушка)
bool Scheduler::satisfiesTimePreference(const TimePreference& pref,
    double start,
    double end) const {
    return true;
}

// Проверка допустимости даты
// @param excludedDates Список исключённых дат
// @param date Проверяемая дата
// @param examType Тип экзамена
// @param group Группа
// @return true, если дата допустима
bool Scheduler::isDateAllowed(const std::vector<std::string>& excludedDates,
    const std::string& date,
    const std::string& examType,
    const std::string& group) const {
    bool isExcluded = std::find(excludedDates.begin(), excludedDates.end(), date) != excludedDates.end();
    bool isHoliday = false;
    bool isSunday = DateUtils::isSunday(date);

    for (const auto& day : sessionDays_) {
        if (day.date == date && day.isHoliday) {
            isHoliday = true;
            break;
        }
    }

    bool allowed = !isExcluded && !isHoliday && !isSunday;

    std::ostringstream oss;
    oss << "Проверка даты '" << date
        << "' для группы '" << group
        << "', Тип='" << examType
        << "': " << (allowed ? "разрешена" : "запрещена")
        << ", Исключена=" << (isExcluded ? "да" : "нет")
        << ", Праздник=" << (isHoliday ? "да" : "нет")
        << ", Воскресенье=" << (isSunday ? "да" : "нет")
        << ", Исключенные даты={";
    if (excludedDates.empty()) {
        oss << "пусто";
    }
    else {
        for (size_t i = 0; i < excludedDates.size(); ++i) {
            oss << excludedDates[i];
            if (i < excludedDates.size() - 1) oss << ", ";
        }
    }
    oss << "}";
    logMessage("INFO", oss.str());
    return allowed;
}