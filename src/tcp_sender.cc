#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) )
  , initial_RTO_ms_( initial_RTO_ms )
  , cur_RTO_ms( initial_RTO_ms )
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return in_flight;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return retransmission_num;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  // Your code here.
  if ( ready_send.size() == 0 ) {
    return nullopt;
  } else {
    TCPSenderMessage msg( ready_send.front() );
    ready_send.pop_front();
    timer_on = true;
    return msg;
  }
}

void TCPSender::push( Reader& outbound_stream )
{
  // Your code here.
  //  - ( abs_seqno - abs_rcv_ackno )
  size_t virtual_window_size = window_size > 0 ? window_size : 1;
  while ( in_flight < virtual_window_size && !fin_flag ) {
    TCPSenderMessage msg;
    if ( !syn_flag ) {
      syn_flag = true;
      msg.SYN = true;
      msg.seqno = isn_;
    } else {
      msg.seqno = Wrap32::wrap( abs_seqno, isn_ );
    }
    size_t size = min( min( virtual_window_size - in_flight, TCPConfig::MAX_PAYLOAD_SIZE ),
                       outbound_stream.bytes_buffered() );
    read( outbound_stream, size, msg.payload );
    if ( outbound_stream.is_finished() && msg.sequence_length() + in_flight < virtual_window_size ) {
      msg.FIN = true;
      fin_flag = true;
    }
    abs_seqno += msg.sequence_length();
    in_flight += msg.sequence_length();
    if ( msg.sequence_length() == 0 ) {
      return;
    }
    outstanding_segment.push( msg );
    ready_send.push_back( msg );
    // remain -= msg.sequence_length();
  }
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  // Your code here.
  TCPSenderMessage msg;
  msg.seqno = Wrap32::wrap( abs_seqno, isn_ );
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.

  window_size = msg.window_size;
  if ( msg.ackno.has_value() ) {
    abs_rcv_ackno = msg.ackno->unwrap( isn_, abs_rcv_ackno );
    // error, impossible
    if ( abs_rcv_ackno > abs_seqno ) {
      return;
    }
    while ( !outstanding_segment.empty() ) {
      TCPSenderMessage sendmsg = outstanding_segment.front();
      if ( sendmsg.seqno.unwrap( isn_, abs_seqno ) + sendmsg.sequence_length() <= abs_rcv_ackno ) {
        in_flight -= sendmsg.sequence_length();
        outstanding_segment.pop();
        if ( outstanding_segment.empty() ) {
          timer_on = false;
        } else {
          timer_on = true;
        }
        //  reset timer
        retransmission_num = 0;
        cur_RTO_ms = initial_RTO_ms_;
        cul_RTO_ms = 0;
      } else {
        break;
      }
    }
  }
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // timeout
  cul_RTO_ms += ms_since_last_tick;
  if ( timer_on && cur_RTO_ms <= cul_RTO_ms ) {
    ready_send.push_front( outstanding_segment.front() );
    ++retransmission_num;
    if ( window_size > 0 ) {
      cur_RTO_ms = pow( 2, retransmission_num ) * initial_RTO_ms_;
    } else {
      cur_RTO_ms = initial_RTO_ms_;
    }
    cul_RTO_ms = 0;
  }
}
