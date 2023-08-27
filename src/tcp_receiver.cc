#include "tcp_receiver.hh"
using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.
  // 第一个syn
  length = message.sequence_length();
  if ( message.SYN ) {
    isn = message.seqno.get_rawval();
    syn_flag = true;
    // syn并且有data
    if ( !message.payload.empty() ) {
      reassembler.insert( 0, string( message.payload ), message.FIN, inbound_stream );
    }
    if ( message.FIN ) {
      inbound_stream.close();
    }
  } else if ( !syn_flag ) {
    return;
  } else {
    abs_seqno = message.seqno.unwrap( Wrap32( isn ), abs_seqno );
    reassembler.insert( abs_seqno - 1, string( message.payload ), message.FIN, inbound_stream );
  }
  expected_abs_seqno = reassembler.get_expected_index() + 1;
  // 卡的最久的一个地方，必须是已经close的才可以加一
  if ( inbound_stream.is_closed() ) {
    ++expected_abs_seqno;
  }
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  TCPReceiverMessage msg;
  msg.window_size = min( 65535UL, inbound_stream.available_capacity() );
  if ( this->syn_flag ) {
    msg.ackno = Wrap32::wrap( expected_abs_seqno, Wrap32( isn ) );
  }

  return msg;
}
