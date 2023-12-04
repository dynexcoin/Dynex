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

#pragma once

#include <QLabel>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QObject>

class QActionGroup;

namespace Ui {
class MainWindow;
}

namespace WalletGui {

class MainWindow : public QMainWindow {
  Q_OBJECT
  Q_DISABLE_COPY(MainWindow)

public:
  static MainWindow& instance();
  void scrollToTransaction(const QModelIndex& _index);
  void quit();

protected:
  void closeEvent(QCloseEvent* _event) Q_DECL_OVERRIDE;
  bool event(QEvent* _event) Q_DECL_OVERRIDE;

private:
  QScopedPointer<Ui::MainWindow> m_ui;
  QLabel* m_connectionStateIconLabel;
  QLabel* m_encryptionStateIconLabel;
  QLabel* m_synchronizationStateIconLabel;
  QSystemTrayIcon* m_trayIcon;
  QActionGroup* m_tabActionGroup;
  bool m_isAboutToQuit;
  QList<QAction*> recentFileActionList;
  static MainWindow* m_instance;

  MainWindow();
  ~MainWindow();

  void connectToSignals();
  void initUi();

  void minimizeToTray(bool _on);
  void setStatusBarText(const QString& _text);
  void showMessage(const QString& _text, QtMsgType _type);
  void askForWalletPassword(bool _error);
  void encryptedFlagChanged(bool _encrypted);
  void peerCountUpdated(quint64 _peer_count);
  void walletSynchronizationInProgress();
  void walletSynchronized(int _error, const QString& _error_text);
  void walletOpened(bool _error, const QString& _error_text);
  void walletClosed();
  void updateRecentActionList();
  void updateTitle(bool showWallet);

  Q_SLOT void createWallet();
  Q_SLOT void openWallet();
  Q_SLOT void backupWallet();
  Q_SLOT void encryptWallet();
  Q_SLOT void aboutQt();
  Q_SLOT void about();
  Q_SLOT void DisplayCmdLineHelp();
  Q_SLOT void setStartOnLogin(bool _on);
  Q_SLOT void setMinimizeToTray(bool _on);
  Q_SLOT void setCloseToTray(bool _on);
  Q_SLOT void restoreCheckboxes();
  Q_SLOT void createNonDeterministicWallet();
  Q_SLOT void closeWallet();
  Q_SLOT void importKey();
  Q_SLOT void importKeys();
  Q_SLOT void resetWallet();
  Q_SLOT void showPrivateKeys();
  Q_SLOT void openRecent();
  Q_SLOT void openLogFile();
  Q_SLOT void showMnemonicSeed();
  Q_SLOT void restoreFromMnemonicSeed();
  Q_SLOT void openConnectionSettings();

#ifdef Q_OS_MAC
public:
  void restoreFromDock();

private:
  void installDockHandler();
#elif defined(Q_OS_WIN)
protected:
  void changeEvent(QEvent* _event) Q_DECL_OVERRIDE;

private:
  void trayActivated(QSystemTrayIcon::ActivationReason _reason);
#endif
};

}
