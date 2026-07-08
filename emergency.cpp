#include "emergency.h"
#include <algorithm>

QString EmergencyPatient::priorityLabel() const
{
    switch (priorityLevel) {
        case 1: return "Critical";
        case 2: return "Serious";
        default: return "Normal";
    }
}

void EmergencyQueue::addPatient(EmergencyPatient p)
{
    p.sequence = m_sequenceCounter++;
    resetIdCounterIfHigher(p.id);
    m_heap.push(p);
}

bool EmergencyQueue::peekNext(EmergencyPatient &out) const
{
    if (m_heap.empty()) return false;
    out = m_heap.top();
    return true;
}

bool EmergencyQueue::removeNext(EmergencyPatient &out)
{
    if (m_heap.empty()) return false;
    out = m_heap.top();
    m_heap.pop();
    return true;
}

QVector<EmergencyPatient> EmergencyQueue::snapshotSortedByPriority() const
{
    // Copy the underlying heap (cheap struct) without disturbing the real queue.
    auto copy = m_heap;
    QVector<EmergencyPatient> result;
    while (!copy.empty()) {
        result.push_back(copy.top());
        copy.pop();
    }
    return result;
}

void EmergencyQueue::clear()
{
    while (!m_heap.empty()) m_heap.pop();
    m_sequenceCounter = 0;
    m_nextId = 1;
}

void EmergencyQueue::loadFromVector(const QVector<EmergencyPatient> &patients)
{
    clear();
    QVector<EmergencyPatient> sorted = patients;
    // Preserve original arrival order as much as possible when reloading from disk.
    for (auto &p : sorted) addPatient(p);
}
