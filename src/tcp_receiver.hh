#pragma once

#include "reassembler.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

class TCPReceiver
{
private:
  uint16_t capacity = 0;
  uint32_t isn = 0;
  bool syn_flag = false;
  bool fin_flag = false;
  uint64_t abs_seqno = 0;
  uint64_t expected_abs_seqno = 0;
  size_t length = 0;

public:
  /*
   * The TCPReceiver receives TCPSenderMessages, inserting their payload into the Reassembler
   * at the correct stream index.
   */
  void receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream );

  /* The TCPReceiver sends TCPReceiverMessages back to the TCPSender. */
  TCPReceiverMessage send( const Writer& inbound_stream ) const;
};
