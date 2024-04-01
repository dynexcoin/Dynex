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

#include "AnimatedLabel.h"

namespace WalletGui {

AnimatedLabel::AnimatedLabel(QWidget* _parent) : QLabel(_parent), m_spriteVerticalSpace(0) {
  connect(&m_animationTimer, &QTimer::timeout, this, &AnimatedLabel::timeout);
}

AnimatedLabel::~AnimatedLabel() {
}

void AnimatedLabel::setSprite(const QPixmap& _spritePixmap, const QSize& _frameSize, quint32 _verticalSpace, quint32 _frequency) {
  m_spritePixmap = _spritePixmap;
  m_spriteFrameSize = _frameSize;
  m_spriteVerticalSpace = _verticalSpace;
  m_animationTimer.setInterval(1000 / _frequency);
  m_frameRect.setTopLeft(QPoint(0, 0));
  m_frameRect.setBottomRight(m_frameRect.topLeft() += QPoint(_frameSize.width(), _frameSize.height()));
}

void AnimatedLabel::startAnimation() {
  if (m_animationTimer.isActive()) {
    return;
  }

  m_animationTimer.start();
}

void AnimatedLabel::stopAnimation() {
  m_animationTimer.stop();
}

void AnimatedLabel::timeout() {
  setPixmap(m_spritePixmap.copy(m_frameRect));
  m_frameRect.translate(QPoint(0, m_spriteVerticalSpace + m_spriteFrameSize.height()));
  if (m_frameRect.bottom() >= m_spritePixmap.height()) {
    m_frameRect.moveTop(0);
  }
}

}
