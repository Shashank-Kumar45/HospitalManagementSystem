#include <QApplication>
#include <QFile>
#include <QTextStream>
#include "login.h"
#include "mainwindow.h"
#include "filemanager.h"

/*
 * main.cpp
 * --------
 * Entry point of the Hospital Management System.
 * Initialises a single FileManager (offline persistence layer), shows the
 * Login window, and only opens the Dashboard (MainWindow) once the user has
 * authenticated successfully.
 */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("Hospital Management System");
    QApplication::setOrganizationName("HMS");

    // Apply the default (light) theme stylesheet before showing any window.
    FileManager fileManager;
    QString theme = fileManager.loadTheme();
    QFile styleFile(theme == "dark" ? ":/styles_dark.qss" : ":/styles.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&styleFile);
        app.setStyleSheet(stream.readAll());
    }

    LoginWindow login(&fileManager);
    if (login.exec() != QDialog::Accepted) {
        return 0; // user closed/cancelled the login dialog
    }

    MainWindow window(&fileManager, login.loggedInUser());
    window.show();

    return app.exec();
}
