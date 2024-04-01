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

#include <QCloseEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QFileInfo>
#include <QFontDatabase>

#include <Common/Util.h>

#include "AnimatedLabel.h"
#include "ChangePasswordDialog.h"
#include "CurrencyAdapter.h"
#include "ExitWidget.h"
#include "MainWindow.h"
#include "NewPasswordDialog.h"
#include "NodeAdapter.h"
#include "PasswordDialog.h"
#include "Settings.h"
#include "WalletAdapter.h"
#include "WalletEvents.h"
#include "PrivateKeysDialog.h"
#include "ImportKeyDialog.h"
#include "ImportKeysDialog.h"
#include "RestoreFromMnemonicSeedDialog.h"
#include "MnemonicSeedDialog.h"
#include "ConnectionSettings.h"
#include "CommandLineParser.h"
#include "AddressBookModel.h"
#include "BalanceProofDialog.h"

#include "version.h"

#include "ui_mainwindow.h"


namespace WalletGui {

MainWindow* MainWindow::m_instance = nullptr;

MainWindow& MainWindow::instance() {
  if (m_instance == nullptr) {
    m_instance = new MainWindow;
  }

  return *m_instance;
}

MainWindow::MainWindow() : QMainWindow(), m_ui(new Ui::MainWindow), m_trayIcon(nullptr), m_tabActionGroup(new QActionGroup(this)), m_isAboutToQuit(false) {
  m_ui->setupUi(this);
  m_connectionStateIconLabel = new QLabel(this);
  m_encryptionStateIconLabel = new QLabel(this);
  m_synchronizationStateIconLabel = new AnimatedLabel(this);
  m_syncProgressBar = new QProgressBar();

  connectToSignals();
  initUi();

  walletClosed();
}

MainWindow::~MainWindow() {
}

void MainWindow::connectToSignals() {
  connect(&WalletAdapter::instance(), &WalletAdapter::openWalletWithPasswordSignal, this, &MainWindow::askForWalletPassword, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::changeWalletPasswordSignal, this, &MainWindow::encryptWallet, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletSynchronizationProgressUpdatedSignal,
    this, &MainWindow::walletSynchronizationInProgress, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletSynchronizationCompletedSignal, this, &MainWindow::walletSynchronized
    , Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletStateChangedSignal, this, &MainWindow::setStatusBarText);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletInitCompletedSignal, this, &MainWindow::walletOpened);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &MainWindow::walletClosed);
  connect(&NodeAdapter::instance(), &NodeAdapter::peerCountUpdatedSignal, this, &MainWindow::peerCountUpdated, Qt::QueuedConnection);
  connect(m_ui->m_exitAction, &QAction::triggered, qApp, &QApplication::quit);
}

void MainWindow::initUi() {
  updateTitle(false);
#ifdef Q_OS_WIN32
  if (QSystemTrayIcon::isSystemTrayAvailable()) {
    m_trayIcon = new QSystemTrayIcon(QPixmap(":images/dynex"), this);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::trayActivated);
  }
#endif

  m_ui->m_overviewFrame->hide();
  m_ui->m_sendFrame->hide();
  m_ui->m_receiveFrame->hide();
  m_ui->m_transactionsFrame->hide();
  m_ui->m_addressBookFrame->hide();

  m_tabActionGroup->addAction(m_ui->m_overviewAction);
  m_tabActionGroup->addAction(m_ui->m_sendAction);
  m_tabActionGroup->addAction(m_ui->m_receiveAction);
  m_tabActionGroup->addAction(m_ui->m_transactionsAction);
  m_tabActionGroup->addAction(m_ui->m_addressBookAction);

  m_syncProgressBar->hide();
  m_syncProgressBar->setRange(0, 100);
  m_syncProgressBar->setValue(0);
  m_syncProgressBar->setFormat("");
  m_syncProgressBar->setTextVisible(true);

  QString styles(" QProgressBar { border: none; text-align: left; background: transparent; } QProgressBar::chunk { background: #00bfff; }");
#ifdef Q_OS_MAC
  const QPalette palette;
  bool darkMode = palette.color(QPalette::WindowText).lightness() > palette.color(QPalette::Window).lightness();
  styles.append(darkMode ? " QProgressBar { color: white; }" : " QProgressBar { color: black; }");
  styles.append(" QProgressBar { margin-left: 2px; } QProgressBar::chunk { border-radius: 6px; }");
#endif
  m_syncProgressBar->setStyleSheet(styles);

  m_ui->m_overviewAction->toggle();
  encryptedFlagChanged(false);
  statusBar()->addPermanentWidget(m_syncProgressBar, 1);
  statusBar()->addPermanentWidget(m_connectionStateIconLabel);
  statusBar()->addPermanentWidget(m_encryptionStateIconLabel);
  statusBar()->addPermanentWidget(m_synchronizationStateIconLabel);
  qobject_cast<AnimatedLabel*>(m_synchronizationStateIconLabel)->setSprite(QPixmap(":icons/sync_sprite"), QSize(16, 16), 5, 24);
  m_connectionStateIconLabel->setPixmap(QPixmap(":icons/disconnected").scaled(16, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

  m_ui->menuRecent_wallets->setVisible(false);
  QAction* recentWalletAction = 0;
  for(int i = 0; i < maxRecentFiles; ++i){
    recentWalletAction = new QAction(this);
    recentWalletAction->setVisible(false);
    QObject::connect(recentWalletAction, SIGNAL(triggered()), this, SLOT(openRecent()));
    recentFileActionList.append(recentWalletAction);
  }
  for(int i = 0; i < maxRecentFiles; ++i)
    m_ui->menuRecent_wallets->addAction(recentFileActionList.at(i));
  updateRecentActionList();

#ifdef Q_OS_MAC
  installDockHandler();
#endif

#ifndef Q_OS_WIN
  m_ui->m_minimizeToTrayAction->deleteLater();
  m_ui->m_closeToTrayAction->deleteLater();
#endif
  // restore states
  QTimer::singleShot(0, this, SLOT(restoreCheckboxes()));

#ifdef Q_OS_WIN
  QDir::setCurrent(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
#else
  QDir::setCurrent(QDir::homePath());
#endif
}

void MainWindow::restoreCheckboxes() {
  m_ui->m_startOnLoginAction->setChecked(Settings::instance().isStartOnLoginEnabled());
  m_ui->m_globalAddressBookAction->setChecked(Settings::instance().isGlobalAddressBookEnabled());
#ifdef Q_OS_WIN
  m_ui->m_minimizeToTrayAction->setChecked(Settings::instance().isMinimizeToTrayEnabled());
  m_ui->m_closeToTrayAction->setChecked(Settings::instance().isCloseToTrayEnabled());
#endif

}

#ifdef Q_OS_WIN
void MainWindow::minimizeToTray(bool _on) {
  if (_on) {
    hide();
    m_trayIcon->show();
  } else {
    showNormal();
    activateWindow();
    m_trayIcon->hide();
  }
}
#endif

void MainWindow::scrollToTransaction(const QModelIndex& _index) {
  m_ui->m_transactionsAction->setChecked(true);
  m_ui->m_transactionsFrame->scrollToTransaction(_index);
}

void MainWindow::quit() {
  if (!m_isAboutToQuit) {
    ExitWidget* exitWidget = new ExitWidget(nullptr);
    exitWidget->show();
    m_isAboutToQuit = true;
#ifdef Q_OS_WIN
    if (m_trayIcon) m_trayIcon->hide();
#endif
    close();
  }
}

#ifdef Q_OS_MAC
void MainWindow::restoreFromDock() {
  if (m_isAboutToQuit) {
    return;
  }

  showNormal();
}
#endif

void MainWindow::closeEvent(QCloseEvent* _event) {
#ifdef Q_OS_WIN
  if (m_isAboutToQuit) {
    QMainWindow::closeEvent(_event);
    return;
  } else if (Settings::instance().isCloseToTrayEnabled()) {
    minimizeToTray(true);
    _event->ignore();
  } else {
    QApplication::quit();
    return;
  }
#elif defined(Q_OS_LINUX)
  if (!m_isAboutToQuit) {
    QApplication::quit();
    return;
  }
#endif
  QMainWindow::closeEvent(_event);

}

#ifdef Q_OS_WIN
void MainWindow::changeEvent(QEvent* _event) {
  QMainWindow::changeEvent(_event);
  if (!QSystemTrayIcon::isSystemTrayAvailable()) {
    QMainWindow::changeEvent(_event);
    return;
  }

  switch (_event->type()) {
  case QEvent::WindowStateChange:
    if(Settings::instance().isMinimizeToTrayEnabled()) {
      minimizeToTray(isMinimized());
    }
    break;
  default:
    break;
  }

  QMainWindow::changeEvent(_event);
}
#endif

bool MainWindow::event(QEvent* _event) {
  switch (static_cast<WalletEventType>(_event->type())) {
    case WalletEventType::ShowMessage:
    showMessage(static_cast<ShowMessageEvent*>(_event)->messageText(), static_cast<ShowMessageEvent*>(_event)->messageType());
    return true;
  }

  return QMainWindow::event(_event);
}

void MainWindow::createWallet() {
  QString filePath = QFileDialog::getSaveFileName(this, tr("New wallet file"), nullptr, tr("Wallets (*.wallet)"));
  if (!filePath.isEmpty() && !filePath.endsWith(".wallet")) {
    filePath.append(".wallet");
  }

  if (!filePath.isEmpty()) {
    if (WalletAdapter::instance().isOpen()) {
      WalletAdapter::instance().close();
    }
    WalletAdapter::instance().setWalletFile(filePath);
    WalletAdapter::instance().createWallet();
  }
}

void MainWindow::createNonDeterministicWallet() {
  QString filePath = QFileDialog::getSaveFileName(this, tr("New wallet file"), nullptr, tr("Wallets (*.wallet)"));
  if (!filePath.isEmpty() && !filePath.endsWith(".wallet")) {
    filePath.append(".wallet");
  }

  if (!filePath.isEmpty()) {
    if (WalletAdapter::instance().isOpen()) {
      WalletAdapter::instance().close();
    }
    WalletAdapter::instance().setWalletFile(filePath);
    WalletAdapter::instance().createNonDeterministic();
  }
}

void MainWindow::openWallet() {
  QString filePath = QFileDialog::getOpenFileName(this, tr("Open .wallet/.keys file"), nullptr, tr("Wallet (*.wallet *.keys)"));
  if (!filePath.isEmpty()) {
    if (WalletAdapter::instance().isOpen()) {
      WalletAdapter::instance().close();
    }

    WalletAdapter::instance().setWalletFile(filePath);
    WalletAdapter::instance().open("");
  }
}

void MainWindow::closeWallet() {
  if (WalletAdapter::instance().isOpen()) {
    WalletAdapter::instance().close();
  }
}

void MainWindow::backupWallet() {
  QString filePath = QFileDialog::getSaveFileName(this, tr("Backup wallet to..."), nullptr, tr("Wallets (*.wallet)"));
  if (!filePath.isEmpty() && !filePath.endsWith(".wallet")) {
    filePath.append(".wallet");
  }

  if (!filePath.isEmpty() && !QFile::exists(filePath)) {
    WalletAdapter::instance().backup(filePath);
  }
}

void MainWindow::encryptWallet() {
  if (Settings::instance().isEncrypted()) {
    bool error = false;
    do {
      ChangePasswordDialog dlg(this);
      if (dlg.exec() == QDialog::Rejected) {
        return;
      }

      QString oldPassword = dlg.getOldPassword();
      QString newPassword = dlg.getNewPassword();
      error = !WalletAdapter::instance().changePassword(oldPassword, newPassword);
    } while (error);
  } else {
    NewPasswordDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
      QString password = dlg.getPassword();
      if (password.isEmpty()) {
        return;
      }

      encryptedFlagChanged(WalletAdapter::instance().changePassword("", password));
    }
  }
}

void MainWindow::openLogFile() {
  QString pathLog = Settings::instance().getDataDir().absoluteFilePath(QApplication::applicationName() + ".log");
  if (!pathLog.isEmpty()) {
    QDesktopServices::openUrl(QUrl::fromLocalFile(pathLog));
  }
}

void MainWindow::resetWallet() {
  Q_ASSERT(WalletAdapter::instance().isOpen());
  if (QMessageBox::warning(this, tr("Warning"), tr("Your wallet will be reset and restored from blockchain.\n"
    "Are you sure?"), QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Ok) {
    WalletAdapter::instance().reset();
    WalletAdapter::instance().open("");
  }
}

void MainWindow::importKey() {
  ImportKeyDialog dlg(this);
  if (dlg.exec() == QDialog::Accepted) {
    QString filePath = dlg.getFilePath();
    if (filePath.isEmpty()) {
      return;
    }
    if (!filePath.endsWith(".wallet")) {
      filePath.append(".wallet");
    }

    DynexCN::AccountKeys keys = dlg.getAccountKeys();

    if (WalletAdapter::instance().isOpen()) {
      WalletAdapter::instance().close();
    }
    WalletAdapter::instance().setWalletFile(filePath);

    quint32 syncHeight = dlg.getSyncHeight();
    if (syncHeight != 0) {
      WalletAdapter::instance().createWithKeys(keys, syncHeight);
    } else {
      WalletAdapter::instance().createWithKeys(keys);
    }
  }
}

void MainWindow::importKeys() {
  ImportKeysDialog dlg(this);
  if (dlg.exec() == QDialog::Accepted) {
    QString filePath = dlg.getFilePath();
    if (filePath.isEmpty()) {
      return;
    }
    if (!filePath.endsWith(".wallet")) {
      filePath.append(".wallet");
    }

    DynexCN::AccountKeys keys = dlg.getAccountKeys();

    if (WalletAdapter::instance().isOpen()) {
      WalletAdapter::instance().close();
    }
    WalletAdapter::instance().setWalletFile(filePath);

    quint32 syncHeight = dlg.getSyncHeight();
    if (syncHeight != 0) {
      WalletAdapter::instance().createWithKeys(keys, syncHeight);
    } else {
      WalletAdapter::instance().createWithKeys(keys);
    }
  }
}

void MainWindow::restoreFromMnemonicSeed() {
  RestoreFromMnemonicSeedDialog dlg(this);
  if (dlg.exec() == QDialog::Accepted) {
    QString filePath = dlg.getFilePath();
    if (filePath.isEmpty()) {
      return;
    }
    if (!filePath.endsWith(".wallet")) {
      filePath.append(".wallet");
    }

    DynexCN::AccountKeys keys = dlg.getAccountKeys();

    if (WalletAdapter::instance().isOpen()) {
      WalletAdapter::instance().close();
    }
    WalletAdapter::instance().setWalletFile(filePath);

    quint32 syncHeight = dlg.getSyncHeight();
    if (syncHeight != 0) {
      WalletAdapter::instance().createWithKeys(keys, syncHeight);
    } else {
      WalletAdapter::instance().createWithKeys(keys);
    }
  }
}

void MainWindow::showPrivateKeys() {
  PrivateKeysDialog dlg(this);
  dlg.walletOpened();
  dlg.exec();
}

void MainWindow::showMnemonicSeed() {
  MnemonicSeedDialog dlg(this);
  dlg.walletOpened();
  dlg.exec();
}

void MainWindow::balanceProof() {
  BalanceProofDialog dlg(this);
  dlg.walletOpened();
  dlg.exec();
}

void MainWindow::openRecent() {
  QAction *action = qobject_cast<QAction *>(sender());
  if (action) {
    QString filePath = action->data().toString();
    if (!filePath.isEmpty() && QFile::exists(filePath)) {
      if (WalletAdapter::instance().isOpen()) {
        WalletAdapter::instance().close();
      }
      WalletAdapter::instance().setWalletFile(filePath);
      WalletAdapter::instance().open("");
    } else {
      QMessageBox::warning(this, tr("Wallet file not found"), tr("The recent wallet file is missing. Probably it was removed."), QMessageBox::Ok);
    }
  }
}

void MainWindow::updateRecentActionList() {
  QStringList recentFilePaths = Settings::instance().getRecentWalletsList();
  if (recentFilePaths.isEmpty())
    m_ui->menuRecent_wallets->setVisible(false);

  if(recentFilePaths.size() != 0) {
    int itEnd = 0;
    if (recentFilePaths.size() <= maxRecentFiles)
      itEnd = recentFilePaths.size();
    else
      itEnd = maxRecentFiles;

    for (int i = 0; i < itEnd; ++i) {
      QString strippedName = QFileInfo(recentFilePaths.at(i)).absoluteFilePath();
      recentFileActionList.at(i)->setText(strippedName);
      recentFileActionList.at(i)->setData(recentFilePaths.at(i));
      recentFileActionList.at(i)->setVisible(true);
    }
    for (int i = itEnd; i < maxRecentFiles; ++i)
      recentFileActionList.at(i)->setVisible(false);
  } else {
    m_ui->menuRecent_wallets->setVisible(false);
  }
}

void MainWindow::aboutQt() {
  QMessageBox::aboutQt(this);
}

void MainWindow::DisplayCmdLineHelp() {
    CommandLineParser cmdLineParser(nullptr);
    QMessageBox *msg = new QMessageBox(QMessageBox::Information, QObject::tr("Help"),
                       cmdLineParser.getHelpText(),
                       QMessageBox::Ok, this);
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    msg->setFont(font);
    QSpacerItem* horizontalSpacer = new QSpacerItem(650, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QGridLayout* layout = (QGridLayout*)msg->layout();
    layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());
    msg->exec();
}

void MainWindow::openConnectionSettings() {
  ConnectionSettingsDialog dlg(&MainWindow::instance());
  dlg.exec();
}

void MainWindow::setStartOnLogin(bool _on) {
  Settings::instance().setStartOnLoginEnabled(_on);
}

void MainWindow::setMinimizeToTray(bool _on) {
#ifdef Q_OS_WIN
  Settings::instance().setMinimizeToTrayEnabled(_on);
#endif
}

void MainWindow::setCloseToTray(bool _on) {
#ifdef Q_OS_WIN
  Settings::instance().setCloseToTrayEnabled(_on);
#endif
}

void MainWindow::setGlobalAddressBook(bool _on) {
  Settings::instance().setGlobalAddressBookEnabled(_on);
  AddressBookModel::instance().reload();
}

void MainWindow::about() {
  const QString translatedTextAboutQtText = tr(
    "<h2>Dynex wallet v%1</h2>"
    "<h3><a href=\"%2\">%2</a></h3>"
    "<h3>Dynex is a next-generation platform for neuromorphic computing</h3>"
    "<p>%3</p>" 
    "<p>Parts of this project are originally copyright by:"
    "<br>Copyright (c) 2012-2017 The CN developers"
    "<br>Copyright (c) 2012-2017 The Bytecoin developers"
    "<br>Copyright (c) 2014-2017 XDN developers"
    "<br>Copyright (c) 2014-2018 The Monero project"
    "<br>Copyright (c) 2014-2018 The Forknote developers"
    "<br>Copyright (c) 2018-2019 The TurtleCoin developers"
    "<br>Copyright (c) 2016-2022 The Karbo developers</p>"
    "<p><a href=\"%4\">%4</a></p>"
    ).arg(Settings::instance().getVersion(), QStringLiteral(CN_PROJECT_SITE), QStringLiteral(CN_PROJECT_COPYRIGHT), 
    	QStringLiteral("http://opensource.org/licenses/MIT"));

  QMessageBox::about(this, tr("About wallet"), translatedTextAboutQtText);
}

void MainWindow::setStatusBarText(const QString& _text) {
  m_statusBarText = _text;
  if (m_syncProgressBar->isHidden()) {
    statusBar()->showMessage(m_statusBarText);
  } else {
    statusBar()->clearMessage();
    m_syncProgressBar->setFormat(QString("  ") + m_statusBarText);
  }
}

void MainWindow::showMessage(const QString& _text, QtMsgType _type) {
  switch (_type) {
  case QtCriticalMsg:
    QMessageBox::critical(this, tr("Wallet error"), _text);
    break;
  case QtWarningMsg:
    QMessageBox::warning(this, tr("Warning"), _text);
    break;
  case QtDebugMsg:
    QMessageBox::information(this, tr("Wallet"), _text);
    break;
  default:
    break;
  }
}

void MainWindow::askForWalletPassword(bool _error) {
  if (m_isAboutToQuit) return;
  PasswordDialog dlg(_error, this);
  if (dlg.exec() == QDialog::Accepted) {
    QString password = dlg.getPassword();
    WalletAdapter::instance().open(password);
  }
}

void MainWindow::encryptedFlagChanged(bool _encrypted) {
  m_ui->m_encryptWalletAction->setEnabled(!_encrypted);
  m_ui->m_changePasswordAction->setEnabled(_encrypted);
  QString encryptionIconPath = _encrypted ? ":icons/encrypted" : ":icons/decrypted";
  QPixmap encryptionIcon = QPixmap(encryptionIconPath).scaled(16, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  m_encryptionStateIconLabel->setPixmap(encryptionIcon);
  QString encryptionLabelTooltip = _encrypted ? tr("Encrypted") : tr("Not encrypted");
  m_encryptionStateIconLabel->setToolTip(encryptionLabelTooltip);
}

void MainWindow::peerCountUpdated(quint64 _peerCount) {
  QString connectionIconPath = _peerCount > 0 ? ":icons/connected" : ":icons/disconnected";
  QPixmap connectionIcon = QPixmap(connectionIconPath).scaled(16, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  m_connectionStateIconLabel->setPixmap(connectionIcon);
  m_connectionStateIconLabel->setToolTip(QString(tr("%1 peers").arg(_peerCount)));
}

void MainWindow::walletSynchronizationInProgress(uint32_t _current, uint32_t _total) {
  qobject_cast<AnimatedLabel*>(m_synchronizationStateIconLabel)->startAnimation();
  m_synchronizationStateIconLabel->setToolTip(tr("Synchronization in progress"));
  if (_current < _total) {
    m_syncProgressBar->setValue(double(_current)/double(_total)*100.0);
    if (_total - _current > 1000 && m_syncProgressBar->isHidden()) {
      m_syncProgressBar->setFormat(m_statusBarText);
      statusBar()->clearMessage();
      m_syncProgressBar->show();
    }
  }
  m_ui->m_proofAction->setEnabled(false);
}

void MainWindow::walletSynchronized(int _error, const QString& _error_text) {
  QPixmap syncIcon = QPixmap(":icons/synced").scaled(16, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  qobject_cast<AnimatedLabel*>(m_synchronizationStateIconLabel)->stopAnimation();
  m_synchronizationStateIconLabel->setPixmap(syncIcon);
  QString syncLabelTooltip = _error > 0 ? tr("Not synchronized") : tr("Synchronized");
  m_synchronizationStateIconLabel->setToolTip(syncLabelTooltip);
  m_syncProgressBar->hide();
  statusBar()->showMessage(m_statusBarText);
  if (WalletAdapter::instance().getActualBalance() > 0) {
    m_ui->m_proofAction->setEnabled(true);
  }
}

void MainWindow::walletOpened(bool _error, const QString& _error_text) {
  if (!_error) {
    updateTitle(true);
    m_encryptionStateIconLabel->show();
    m_synchronizationStateIconLabel->show();
    m_ui->m_backupWalletAction->setEnabled(true);
    m_ui->m_closeWalletAction->setEnabled(true);
    m_ui->m_showPrivateKey->setEnabled(true);
    m_ui->m_resetAction->setEnabled(true);
    if (WalletAdapter::instance().isDeterministic()) {
       m_ui->m_showMnemonicSeedAction->setEnabled(true);
    }
    if (WalletAdapter::instance().getActualBalance() > 0) {
        m_ui->m_proofAction->setEnabled(true);
    }
    encryptedFlagChanged(Settings::instance().isEncrypted());
    QList<QAction*> tabActions = m_tabActionGroup->actions();
    Q_FOREACH(auto action, tabActions) {
      action->setEnabled(true);
    }

    m_ui->m_overviewAction->trigger();
    m_ui->m_overviewFrame->show();

    updateRecentActionList();
    WalletAdapter::instance().autoBackup();

    QString wallet(Settings::instance().getWalletFile());
    if (!wallet.isEmpty()) {
      QFileInfo fi(wallet);
      QDir::setCurrent(fi.canonicalPath());
    }
  } else {
    walletClosed();
  }
}

void MainWindow::walletClosed() {
  updateTitle(false);
  m_ui->m_backupWalletAction->setEnabled(false);
  m_ui->m_encryptWalletAction->setEnabled(false);
  m_ui->m_changePasswordAction->setEnabled(false);
  m_ui->m_closeWalletAction->setEnabled(false);
  m_ui->m_showPrivateKey->setEnabled(false);
  m_ui->m_resetAction->setEnabled(false);
  m_ui->m_showMnemonicSeedAction->setEnabled(false);
  m_ui->m_proofAction->setEnabled(false);
  m_ui->m_overviewFrame->hide();
  m_ui->m_sendFrame->hide();
  m_ui->m_transactionsFrame->hide();
  m_ui->m_addressBookFrame->hide();
  m_syncProgressBar->hide();
  statusBar()->showMessage(m_statusBarText);
  m_encryptionStateIconLabel->hide();
  m_synchronizationStateIconLabel->hide();
  QList<QAction*> tabActions = m_tabActionGroup->actions();
  Q_FOREACH(auto action, tabActions) {
    action->setEnabled(false);
  }
  updateRecentActionList();
}

#ifdef Q_OS_WIN
void MainWindow::trayActivated(QSystemTrayIcon::ActivationReason _reason) {
  showNormal();
  m_trayIcon->hide();
}
#endif

void MainWindow::updateTitle(bool showWallet) {
  QString wallet;
  if (showWallet) wallet = Settings::instance().getWalletFile();

  if (!wallet.isEmpty()) {
    QFileInfo fi(wallet);
    wallet = fi.baseName();
  }
  if (wallet.isEmpty()) {
    setWindowTitle(QString("%1 Wallet %2").arg(CurrencyAdapter::instance().getCurrencyDisplayName()).arg(Settings::instance().getVersion()));
  } else {
    setWindowTitle(QString("%1 Wallet %2 - %3").arg(CurrencyAdapter::instance().getCurrencyDisplayName()).arg(Settings::instance().getVersion()).arg(wallet));
  }
}

}
