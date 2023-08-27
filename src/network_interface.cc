#include "network_interface.hh"
#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  EthernetFrame eframe;
  eframe.header.src = ethernet_address_;
  uint32_t target_ip = next_hop.ipv4_numeric();
  if ( timer_on && waiting_ip == target_ip ) {
    return;
  }
  // found in cache
  auto it = arpcache.find( target_ip );
  if ( it != arpcache.end() ) {
    eframe.payload = serialize( dgram );
    eframe.header.dst = it->second;
    eframe.header.type = EthernetHeader::TYPE_IPv4;
  } else {
    ARPMessage arpmsg;
    arpmsg.opcode = ARPMessage::OPCODE_REQUEST;
    arpmsg.sender_ip_address = ip_address_.ipv4_numeric();
    arpmsg.sender_ethernet_address = ethernet_address_;
    arpmsg.target_ip_address = target_ip;
    arpmsg.target_ethernet_address = std::array<uint8_t, 6> {};
    eframe.payload = serialize( arpmsg );
    eframe.header.dst = ETHERNET_BROADCAST;
    eframe.header.type = EthernetHeader::TYPE_ARP;
    timer_on = true;
    cul_time = 0;
    ip_datagram.push( dgram );
    waiting_ip = target_ip;
  }
  frame_queue.push( eframe );
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if ( frame.header.dst == ethernet_address_ || frame.header.dst == ETHERNET_BROADCAST ) {
    if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
      InternetDatagram dgram;
      if ( parse( dgram, frame.payload ) ) {
        return dgram;
      }
    } else {
      ARPMessage arpmsg;
      if ( parse( arpmsg, frame.payload ) ) {

        if ( arpmsg.target_ip_address != ip_address_.ipv4_numeric() ) {
          return nullopt;
        }
        if ( arpcache.find( arpmsg.sender_ip_address ) == arpcache.end() ) {
          arpcache.insert( make_pair( arpmsg.sender_ip_address, arpmsg.sender_ethernet_address ) );
          maptimeout.push_back( std::make_pair( arpmsg.sender_ip_address, 0 ) );
        }

        // sender request
        if ( arpmsg.opcode == ARPMessage::OPCODE_REQUEST ) {
          ARPMessage reply;
          reply.opcode = ARPMessage::OPCODE_REPLY;
          reply.sender_ip_address = ip_address_.ipv4_numeric();
          reply.sender_ethernet_address = ethernet_address_;
          reply.target_ip_address = arpmsg.sender_ip_address;
          reply.target_ethernet_address = arpmsg.sender_ethernet_address;
          EthernetFrame eframe;
          eframe.payload = serialize( reply );
          eframe.header.src = ethernet_address_;
          eframe.header.dst = arpmsg.sender_ethernet_address;
          eframe.header.type = EthernetHeader::TYPE_ARP;
          frame_queue.push( eframe );
        } else if ( arpmsg.opcode == ARPMessage::OPCODE_REPLY && timer_on ) {
          waiting_ip = 0;
          timer_on = false;
          send_datagram( ip_datagram.front(), Address::from_ipv4_numeric( arpmsg.sender_ip_address ) );
          ip_datagram.pop();
        }
      }
    }
  }
  return nullopt;
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  if ( timer_on ) {
    cul_time += ms_since_last_tick;
    if ( cul_time >= 5000 ) {
      ip_datagram.pop();
      waiting_ip = 0;
      timer_on = false;
    }
  }
  auto it = maptimeout.begin();

  while ( it != maptimeout.end() ) {
    it->second += ms_since_last_tick;
    if ( it->second >= 30000 ) {
      arpcache.erase( it->first );
      maptimeout.erase( it );
    } else {
      ++it;
    }
  }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if ( !frame_queue.empty() ) {
    EthernetFrame tmp = frame_queue.front();
    frame_queue.pop();
    return tmp;
  } else {
    return nullopt;
  }
}
