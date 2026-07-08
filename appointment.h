#ifndef APPOINTMENT_H
#define APPOINTMENT_H

#include <QString>
#include <QVector>
#include <deque>

/*
 * Appointment
 * -----------
 * Plain data structure representing one booked appointment.
 */
struct Appointment {
    int appointmentId = 0;
    int patientId = 0;
    QString patientName;
    int doctorId = 0;
    QString doctorName;
    QString date;     // yyyy-MM-dd
    QString timeSlot; // free text, e.g. "10:30 AM"
    QString status = "Scheduled"; // Scheduled / Cancelled / Completed

    QString toLine() const;
    static Appointment fromLine(const QString &line, bool *ok = nullptr);
};

/*
 * AppointmentQueue
 * -----------------
 * Data structure used: Queue (FIFO) implemented with std::deque.
 * Appointments are served strictly First-Come-First-Serve: new bookings
 * are pushed to the back, and the "next" appointment to be attended is
 * always taken from the front. Cancelling removes a specific appointment
 * from wherever it sits in the queue (still O(n) but the booking/serving
 * order itself always remains FCFS for everyone else).
 */
class AppointmentQueue {
public:
    bool bookAppointment(const Appointment &a, QString *error = nullptr);
    bool cancelAppointment(int appointmentId, QString *error = nullptr);
    bool peekNext(Appointment &out) const;     // front of queue
    bool serveNext(Appointment &out);          // pops the front (mark completed)

    QVector<Appointment> all() const;          // queue order, front first
    QVector<Appointment> todays(const QString &todayStr) const;

    int count() const { return static_cast<int>(m_queue.size()); }
    int nextAvailableId() const;

    void clear();
    void loadFromVector(const QVector<Appointment> &appts);

private:
    std::deque<Appointment> m_queue;
};

#endif // APPOINTMENT_H
