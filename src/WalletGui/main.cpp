// Copyright (c) 2022-2023, Dynex Developers
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Parts of this project are originally copyright by:
// Copyright (c) 2012-2017 The CN developers
// Copyright (c) 2012-2017 The Bytecoin developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2014-2018 The Monero project
// Copyright (c) 2014-2018 The Forknote developers
// Copyright (c) 2018-2019 The TurtleCoin developers
// Copyright (c) 2016-2022 The Karbo developers

#include <QApplication>
#include <QCommandLineParser>
#include <QLocale>
#include <QLockFile>
#include <QMessageBox>
#include <QSplashScreen>
#include <QIcon>
#include <QStyleFactory>
#include <QRegularExpression>
#include <QFontDatabase>
#include <QLayout>
#include <QSystemTrayIcon>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>

#include "CommandLineParser.h"
#include "CurrencyAdapter.h"
#include "LoggerAdapter.h"
#include "NodeAdapter.h"
#include "Settings.h"
#include "SignalHandler.h"
#include "WalletAdapter.h"
#include "LogFileWatcher.h"

#include "gui/MainWindow.h"
#include "gui/ConnectionSettings.h"

#if defined (Q_OS_WIN)
 #include <windows.h>
 #include <winuser.h>
#endif

//#define DEBUG 1

using namespace WalletGui;

QSplashScreen* splash(nullptr);

const QRegularExpression LOG_SPLASH_REG_EXP("\\] ");

void newLogString(const QString& _string) {
  if (!splash || splash->isHidden()) return;
  QRegularExpressionMatch match = LOG_SPLASH_REG_EXP.match(_string);
  if (match.hasMatch()) {
    QString message = _string.mid(match.capturedEnd());
    splash->showMessage(message, Qt::AlignLeft | Qt::AlignBottom, Qt::white);
  }
}

#ifdef Q_OS_WIN
QSystemTrayIcon* trayIcon(nullptr);

void trayActivated(QSystemTrayIcon::ActivationReason _reason) {
  //if (trayIcon) trayIcon->hide();
  if (splash && !splash->isVisible()) splash->show();
}
#endif

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  app.setApplicationName(CurrencyAdapter::instance().getCurrencyName() + "wallet");
  app.setApplicationVersion(Settings::instance().getVersion());
  app.setQuitOnLastWindowClosed(false);

#ifndef Q_OS_MAC
  QApplication::setStyle(QStyleFactory::create("Fusion"));
#endif

  CommandLineParser cmdLineParser(nullptr);
  Settings::instance().setCommandLineParser(&cmdLineParser);
  bool cmdLineParseResult = cmdLineParser.process(app.arguments());
  Settings::instance().load();

#ifdef Q_OS_WIN
  if(!cmdLineParseResult) {
    QMessageBox::critical(nullptr, QObject::tr("Error"), cmdLineParser.getErrorText());
    return app.exec();
  } else if (cmdLineParser.hasHelpOption()) {
    QMessageBox *msg = new QMessageBox(QMessageBox::Information, QObject::tr("Help"), cmdLineParser.getHelpText(), QMessageBox::Ok, nullptr);
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    msg->setFont(font);
    QSpacerItem* horizontalSpacer = new QSpacerItem(650, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QGridLayout* layout = (QGridLayout*)msg->layout();
    layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());
    msg->exec();
    return app.exec();
  }
#endif

  QString dataDirPath = Settings::instance().getDataDir().absolutePath();
  if (!QDir().exists(dataDirPath)) {
    QDir().mkpath(dataDirPath);
  }

  LoggerAdapter::instance().init();

  QLockFile lockFile(Settings::instance().getDataDir().absoluteFilePath(QApplication::applicationName() + ".lock"));
  if (!lockFile.tryLock()) {
    QMessageBox::warning(nullptr, QObject::tr("Fail"), QString("%1 wallet already running").arg(CurrencyAdapter::instance().getCurrencyDisplayName()));
    return 0;
  }

  QLocale::setDefault(QLocale::c());

  SignalHandler::instance().init();
  QObject::connect(&SignalHandler::instance(), &SignalHandler::quitSignal, &app, &QApplication::quit);

  if (splash == nullptr) {
    splash = new QSplashScreen(QPixmap(":images/splash"), Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint | Qt::FramelessWindowHint); // Qt::WindowTransparentForInput
    splash->setWindowIcon(QIcon(":images/dynex"));
#if defined (Q_OS_WIN)
    int exstyle = GetWindowLong(reinterpret_cast<HWND>(splash->winId()), GWL_EXSTYLE);
    SetWindowLong(reinterpret_cast<HWND>(splash->winId()), GWL_EXSTYLE, exstyle & ~WS_EX_TOOLWINDOW);
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
      trayIcon = new QSystemTrayIcon(QPixmap(":images/dynex"), &app);
      QObject::connect(trayIcon, &QSystemTrayIcon::activated, &app, trayActivated);
      trayIcon->show();
    }
#endif
  }
  if (!splash->isVisible()) {
    splash->show();
  }
  splash->showMessage(QObject::tr("Loading blockchain..."), Qt::AlignLeft | Qt::AlignBottom, Qt::white);

  app.processEvents();

  LogFileWatcher* logWatcher = new LogFileWatcher(Settings::instance().getDataDir().absoluteFilePath(QCoreApplication::applicationName() + ".log"), &app);
  QObject::connect(logWatcher, &LogFileWatcher::newLogStringSignal, &app, &newLogString);
  
  qRegisterMetaType<DynexCN::TransactionId>("DynexCN::TransactionId");
  qRegisterMetaType<quintptr>("quintptr");

  if (!NodeAdapter::instance().init()) {
    LoggerAdapter::instance().log("Node init failed");
    splash->hide();
#if defined (Q_OS_WIN)
    trayIcon->hide();
#endif
    QMessageBox::critical(nullptr, QObject::tr("Error"), QObject::tr("Node initialization failed.\nCheck connection settings."));
    ConnectionSettingsDialog dlg(nullptr);
    dlg.exec();
    return 1;
  }

  QObject::connect(QApplication::instance(), &QApplication::aboutToQuit, []() {
    MainWindow::instance().quit();
#if defined (Q_OS_WIN)
    trayIcon->show();
#endif
    QCoreApplication::processEvents();
    if (WalletAdapter::instance().isOpen()) {
      WalletAdapter::instance().close();
    }
    NodeAdapter::instance().deinit();
#if defined (Q_OS_WIN)
    trayIcon->hide();
    trayIcon->deleteLater();
    trayIcon = nullptr;
#endif
  });

  splash->finish(&MainWindow::instance());
  if (logWatcher != nullptr) {
    logWatcher->deleteLater();
    logWatcher = nullptr;
  }
  splash->deleteLater();
  splash = nullptr;
#if defined (Q_OS_WIN)
  trayIcon->hide();
#endif

  MainWindow::instance().show();
  QString wallet = Settings::instance().getWalletFile();
  if (!wallet.isEmpty()) {
    Settings::instance().setWalletFile(wallet);
  } else {
    WalletAdapter::instance().setWalletFile(Settings::instance().getDataDir().absoluteFilePath(QCoreApplication::applicationName() + ".wallet"));
    WalletAdapter::instance().createWallet();
  }
  WalletAdapter::instance().open("");

  return app.exec();
}
