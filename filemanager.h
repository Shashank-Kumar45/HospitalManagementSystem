#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QString>
#include <QVector>
#include "patient.h"
#include "doctor.h"
#include "appointment.h"
#include "emergency.h"

/*
 * UserAccount
 * -----------
 * Simple local login credential record.
 */
struct UserAccount {
    QString username;
    QString password;
    QString fullName;
};

/*
 * FileManager
 * -----------
 * Centralises all offline file I/O for the application. No database engine
 * of any kind is used; every record is stored as a pipe-delimited line in a
 * plain text file in the application's working directory. Missing files are
 * created automatically and treated as "no records yet" rather than an error.
 */
class FileManager {
public:
    explicit FileManager(const QString &baseDir = QString());

    // Patients
    QVector<Patient> loadPatients() const;
    bool savePatients(const QVector<Patient> &patients) const;

    // Doctors
    QVector<Doctor> loadDoctors() const;
    bool saveDoctors(const QVector<Doctor> &doctors) const;

    // Appointments
    QVector<Appointment> loadAppointments() const;
    bool saveAppointments(const QVector<Appointment> &appts) const;

    // Emergency queue (extra persistence file, not overwritten on every keystroke,
    // only on add/remove, so the queue survives an application restart).
    QVector<EmergencyPatient> loadEmergency() const;
    bool saveEmergency(const QVector<EmergencyPatient> &patients) const;

    // Users / authentication
    QVector<UserAccount> loadUsers() const;
    bool saveUsers(const QVector<UserAccount> &users) const;
    bool validateLogin(const QString &username, const QString &password) const;

    // Settings (theme, etc.)
    QString loadTheme() const;          // returns "light" or "dark"
    bool saveTheme(const QString &theme) const;

    // Generic helper: ensures a file exists (creates it empty if missing).
    static void ensureFileExists(const QString &path);

private:
    QString m_baseDir;
    QString path(const QString &fileName) const;
};

#endif // FILEMANAGER_H
