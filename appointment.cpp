#include "appointment.h"
#include <QStringList>
#include <algorithm>

QString Appointment::toLine() const
{
    auto esc = [](QString s) { return s.replace('|', '/'); };
    return QString("%1|%2|%3|%4|%5|%6|%7|%8")
        .arg(appointmentId)
        .arg(patientId)
        .arg(esc(patientName))
        .arg(doctorId)
        .arg(esc(doctorName))
        .arg(esc(date))
        .arg(esc(timeSlot))
        .arg(esc(status));
}

Appointment Appointment::fromLine(const QString &line, bool *ok)
{
    Appointment a;
    QStringList parts = line.split('|');
    if (parts.size() < 8) {
        if (ok) *ok = false;
        return a;
    }
    bool idOk = false, pidOk = false, didOk = false;
    a.appointmentId = parts[0].toInt(&idOk);
    a.patientId = parts[1].toInt(&pidOk);
    a.patientName = parts[2];
    a.doctorId = parts[3].toInt(&didOk);
    a.doctorName = parts[4];
    a.date = parts[5];
    a.timeSlot = parts[6];
    a.status = parts[7];
    if (ok) *ok = idOk && pidOk && didOk;
    return a;
}

bool AppointmentQueue::bookAppointment(const Appointment &a, QString *error)
{
    for (const auto &existing : m_queue) {
        if (existing.appointmentId == a.appointmentId) {
            if (error) *error = QString("Appointment ID %1 already exists.").arg(a.appointmentId);
            return false;
        }
    }
    m_queue.push_back(a); // enqueue - FCFS
    return true;
}

bool AppointmentQueue::cancelAppointment(int appointmentId, QString *error)
{
    for (auto it = m_queue.begin(); it != m_queue.end(); ++it) {
        if (it->appointmentId == appointmentId) {
            it->status = "Cancelled";
            m_queue.erase(it);
            return true;
        }
    }
    if (error) *error = QString("Appointment ID %1 was not found.").arg(appointmentId);
    return false;
}

bool AppointmentQueue::peekNext(Appointment &out) const
{
    if (m_queue.empty()) return false;
    out = m_queue.front();
    return true;
}

bool AppointmentQueue::serveNext(Appointment &out)
{
    if (m_queue.empty()) return false;
    out = m_queue.front();
    out.status = "Completed";
    m_queue.pop_front(); // dequeue
    return true;
}

QVector<Appointment> AppointmentQueue::all() const
{
    QVector<Appointment> result;
    for (const auto &a : m_queue) result.push_back(a);
    return result;
}

QVector<Appointment> AppointmentQueue::todays(const QString &todayStr) const
{
    QVector<Appointment> result;
    for (const auto &a : m_queue) {
        if (a.date == todayStr) result.push_back(a);
    }
    return result;
}

int AppointmentQueue::nextAvailableId() const
{
    int maxId = 0;
    for (const auto &a : m_queue) maxId = std::max(maxId, a.appointmentId);
    return maxId + 1;
}

void AppointmentQueue::clear()
{
    m_queue.clear();
}

void AppointmentQueue::loadFromVector(const QVector<Appointment> &appts)
{
    clear();
    for (const auto &a : appts) m_queue.push_back(a);
}
