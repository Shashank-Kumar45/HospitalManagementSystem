#ifndef EMERGENCY_H
#define EMERGENCY_H

#include <QString>
#include <QVector>
#include <queue>
#include <vector>

/*
 * EmergencyPatient
 * ----------------
 * priorityLevel: 1 = Critical, 2 = Serious, 3 = Normal (lower number = more urgent).
 * sequence: tie-breaker so that among patients with the same priority level,
 *           whoever arrived first is still treated first (stable ordering).
 */
struct EmergencyPatient {
    int id = 0;
    QString name;
    int age = 0;
    QString gender;
    QString condition;
    int priorityLevel = 3;
    long long sequence = 0;

    QString priorityLabel() const;
};

/*
 * Comparator used by std::priority_queue. std::priority_queue is a MAX-heap
 * by default, so to get the most urgent (lowest priorityLevel, then lowest
 * sequence/earliest arrival) patient at the top, the comparator must return
 * true when 'a' is LESS urgent than 'b'.
 */
struct EmergencyComparator {
    bool operator()(const EmergencyPatient &a, const EmergencyPatient &b) const {
        if (a.priorityLevel != b.priorityLevel)
            return a.priorityLevel > b.priorityLevel; // higher number = less urgent
        return a.sequence > b.sequence; // arrived later = served later
    }
};

/*
 * EmergencyQueue
 * --------------
 * Data structure used: Priority Queue (std::priority_queue) so that the
 * most critical patient is always served next regardless of arrival order,
 * while patients of equal severity are still served FCFS among themselves.
 */
class EmergencyQueue {
public:
    void addPatient(EmergencyPatient p);
    bool peekNext(EmergencyPatient &out) const;
    bool removeNext(EmergencyPatient &out); // pop top (treat next patient)

    QVector<EmergencyPatient> snapshotSortedByPriority() const; // for table display, non-destructive
    int count() const { return static_cast<int>(m_heap.size()); }

    int nextAvailableId()  { return m_nextId++; }
    void resetIdCounterIfHigher(int id) { if (id >= m_nextId) m_nextId = id + 1; }

    void clear();
    void loadFromVector(const QVector<EmergencyPatient> &patients);

private:
    std::priority_queue<EmergencyPatient, std::vector<EmergencyPatient>, EmergencyComparator> m_heap;
    long long m_sequenceCounter = 0;
    int m_nextId = 1;
};

#endif // EMERGENCY_H
