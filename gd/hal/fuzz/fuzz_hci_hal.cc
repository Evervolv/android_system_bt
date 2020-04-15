/*
 * Copyright 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hal/fuzz/fuzz_hci_hal.h"
#include "fuzz/helpers.h"
#include "hci/fuzz/status_vs_complete_commands.h"

namespace bluetooth {
namespace hal {
namespace fuzz {

void FuzzHciHal::registerIncomingPacketCallback(HciHalCallbacks* callbacks) {
  callbacks_ = callbacks;
}

void FuzzHciHal::unregisterIncomingPacketCallback() {
  callbacks_ = nullptr;
}

void FuzzHciHal::sendHciCommand(HciPacket packet) {
  auto packetView = packet::PacketView<packet::kLittleEndian>(std::make_shared<std::vector<uint8_t>>(packet));
  hci::CommandPacketView command = hci::CommandPacketView::Create(packetView);
  if (!command.IsValid()) {
    return;
  }

  waiting_opcode_ = command.GetOpCode();
  waiting_for_status_ = hci::fuzz::uses_command_status(waiting_opcode_);
}

void FuzzHciHal::injectHciEvent(std::vector<uint8_t> data) {
  auto packet = packet::PacketView<packet::kLittleEndian>(std::make_shared<std::vector<uint8_t>>(data));
  hci::EventPacketView event = hci::EventPacketView::Create(packet);
  if (!event.IsValid()) {
    return;
  }

  hci::CommandCompleteView complete = hci::CommandCompleteView::Create(event);
  if (complete.IsValid()) {
    if (waiting_for_status_ || complete.GetCommandOpCode() != waiting_opcode_) {
      return;
    }
  } else if (!waiting_for_status_) {
    return;
  }

  hci::CommandStatusView status = hci::CommandStatusView::Create(event);
  if (status.IsValid()) {
    if (!waiting_for_status_ || status.GetCommandOpCode() != waiting_opcode_) {
      return;
    }
  } else if (waiting_for_status_) {
    return;
  }

  callbacks_->hciEventReceived(data);
}

void FuzzHciHal::injectAclData(std::vector<uint8_t> data) {
  auto packet = packet::PacketView<packet::kLittleEndian>(std::make_shared<std::vector<uint8_t>>(data));
  hci::AclPacketView aclPacket = hci::AclPacketView::Create(packet);
  if (!aclPacket.IsValid()) {
    return;
  }

  callbacks_->aclDataReceived(data);
}

void FuzzHciHal::injectScoData(std::vector<uint8_t> data) {
  auto packet = packet::PacketView<packet::kLittleEndian>(std::make_shared<std::vector<uint8_t>>(data));
  hci::ScoPacketView scoPacket = hci::ScoPacketView::Create(packet);
  if (!scoPacket.IsValid()) {
    return;
  }

  callbacks_->scoDataReceived(data);
}

}  // namespace fuzz
}  // namespace hal
}  // namespace bluetooth
