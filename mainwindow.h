#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <unordered_map>
#include "filemanager.h"
#include "patient.h"
#include "doctor.h"
#include "appointment.h"
#include "emergency.h"

class QListWidget;
class QStackedWidget;
class QTableWidget;
class QLineEdit;
class QSpinBox;
class QComboBox;
class QDateEdit;
class QTextEdit;
class QLabel;
class QPushButton;
class QListWidgetItem;

/*
 * MainWindow
 * ----------
 * The Dashboard shown after a successful login. Hosts every module
 * (Patients, Doctors, Appointments, Emergency, Reports, Search, Settings)
 * as a page inside a QStackedWidget, switched via a navigation QListWidget.
 *
 * In-memory data structures owned here:
 *  - PatientList       : linked list + hash index + undo stack (patients)
 *  - DoctorManager      : hash map (doctors)
 *  - AppointmentQueue   : FIFO queue (appointments, FCFS)
 *  - EmergencyQueue      : priority queue (emergency cases)
 *  - m_searchIndex       : unordered_map<QString,int> mapping a lower-cased
 *                          "name" key to a patient ID for instant search,
 *                          rebuilt whenever patient data changes.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(FileManager *fileManager, const QString &loggedInUser, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Navigation
    void onNavChanged(QListWidgetItem *current);
    void onLogout();

    // Patients
    void onAddPatient();
    void onUpdatePatient();
    void onDeletePatient();
    void onClearPatientForm();
    void onPatientTableSelectionChanged();
    void onPatientSearchTextChanged(const QString &text);
    void onUndoDeletePatient();

    // Doctors
    void onAddDoctor();
    void onUpdateDoctor();
    void onDeleteDoctor();
    void onClearDoctorForm();
    void onDoctorTableSelectionChanged();
    void onDoctorSearchTextChanged(const QString &text);

    // Appointments
    void onBookAppointment();
    void onCancelAppointment();
    void onServeNextAppointment();
    void refreshAppointmentTable();

    // Emergency
    void onAddEmergencyPatient();
    void onRemoveNextEmergencyPatient();
    void refreshEmergencyTable();

    // Search (global)
    void onGlobalSearch();

    // Reports
    void refreshReports();
    void onExportReportCsv();
    void onPrintReport();

    // Settings
    void onToggleTheme();
    void onChangePassword();

    // About / Help
    void onAbout();
    void onHelp();

private:
    // Construction helpers
    void buildUi();
    QWidget *buildPatientsPage();
    QWidget *buildDoctorsPage();
    QWidget *buildAppointmentsPage();
    QWidget *buildEmergencyPage();
    QWidget *buildReportsPage();
    QWidget *buildSearchPage();
    QWidget *buildSettingsPage();
    void buildMenuToolStatusBars();

    // Data refresh helpers
    void refreshPatientTable(const QVector<Patient> &patients);
    void refreshDoctorTable(const QVector<Doctor> &doctors);
    void rebuildPatientSearchIndex();
    void loadAllData();
    void persistPatients();
    void persistDoctors();
    void persistAppointments();
    void persistEmergency();

    // Validation helpers
    bool validatePatientForm(QString *error) const;
    bool validateDoctorForm(QString *error) const;
    bool isValidPhone(const QString &phone) const;

    void applyTheme(const QString &theme);
    void setStatusMessage(const QString &msg, int timeoutMs = 4000);

    // Core data
    FileManager *m_fileManager;
    QString m_loggedInUser;
    QString m_currentTheme;

    PatientList m_patients;
    DoctorManager m_doctors;
    AppointmentQueue m_appointments;
    EmergencyQueue m_emergency;
    std::unordered_map<QString, int> m_searchIndex; // lowercase name fragment -> patient id (last match)

    int m_editingPatientId = -1;
    int m_editingDoctorId = -1;

    // Navigation / pages
    QListWidget *m_navPanel;
    QStackedWidget *m_stack;

    // Patients page widgets
    QTableWidget *m_patientTable;
    QLineEdit *m_pIdEdit, *m_pNameEdit, *m_pPhoneEdit, *m_pAddressEdit, *m_pSearchEdit;
    QSpinBox *m_pAgeSpin;
    QComboBox *m_pGenderCombo, *m_pBloodCombo;
    QLineEdit *m_pDiseaseEdit;
    QDateEdit *m_pAdmissionDate;
    QPushButton *m_pAddBtn, *m_pUpdateBtn, *m_pDeleteBtn, *m_pClearBtn, *m_pUndoBtn;

    // Doctors page widgets
    QTableWidget *m_doctorTable;
    QLineEdit *m_dIdEdit, *m_dNameEdit, *m_dSpecializationEdit, *m_dPhoneEdit, *m_dSearchEdit;
    QSpinBox *m_dExperienceSpin;
    QPushButton *m_dAddBtn, *m_dUpdateBtn, *m_dDeleteBtn, *m_dClearBtn;

    // Appointments page widgets
    QTableWidget *m_appointmentTable;
    QLineEdit *m_aPatientIdEdit, *m_aDoctorIdEdit, *m_aTimeEdit, *m_aCancelIdEdit;
    QDateEdit *m_aDateEdit;
    QPushButton *m_aBookBtn, *m_aCancelBtn, *m_aServeNextBtn;
    QLabel *m_aNextLabel;

    // Emergency page widgets
    QTableWidget *m_emergencyTable;
    QLineEdit *m_eNameEdit, *m_eConditionEdit;
    QSpinBox *m_eAgeSpin;
    QComboBox *m_eGenderCombo, *m_ePriorityCombo;
    QPushButton *m_eAddBtn, *m_eRemoveNextBtn;
    QLabel *m_eNextLabel;

    // Reports page widgets
    QLabel *m_rTotalPatients, *m_rTotalDoctors, *m_rTodayAppointments, *m_rEmergencyCases;
    QLabel *m_rMaleCount, *m_rFemaleCount, *m_rAverageAge;
    QTextEdit *m_rSummaryText;

    // Search page widgets
    QLineEdit *m_searchGlobalEdit;
    QComboBox *m_searchTypeCombo;
    QTableWidget *m_searchResultTable;

    // Settings page widgets
    QPushButton *m_settingsThemeBtn;
    QLineEdit *m_settingsOldPass, *m_settingsNewPass;
};

#endif // MAINWINDOW_H
