#ifndef LOGIN_H
#define LOGIN_H

#include <QDialog>
#include "filemanager.h"

class QLineEdit;
class QLabel;
class QPushButton;

/*
 * LoginWindow
 * -----------
 * The application's entry point UI. Validates the entered credentials
 * against users.txt (via FileManager) before allowing the user into
 * the Dashboard (MainWindow).
 *
 * Default account: admin / admin123
 */
class LoginWindow : public QDialog
{
    Q_OBJECT
public:
    explicit LoginWindow(FileManager *fileManager, QWidget *parent = nullptr);

    QString loggedInUser() const { return m_loggedInUser; }

private slots:
    void onLoginClicked();
    void onTogglePasswordVisibility();

private:
    void buildUi();

    FileManager *m_fileManager;
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QLabel *m_errorLabel;
    QPushButton *m_loginButton;
    QPushButton *m_toggleVisibilityButton;
    QString m_loggedInUser;
};

#endif // LOGIN_H
