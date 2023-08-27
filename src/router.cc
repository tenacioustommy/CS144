#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";
  Routeinfo route {};
  route.route_prefix = route_prefix;
  route.prefix_length = prefix_length;
  route.next_hop = next_hop;
  route.interface_index = interface_num;
  routetable.push_back( route );
}
void Router::route()
{
  for ( auto& current_interface : interfaces_ ) {
    std::optional<InternetDatagram> dgram = current_interface.maybe_receive();
    while ( dgram.has_value() && dgram->header.ttl > 1 ) {
      dgram->header.ttl--;
      // very very important!!!,tmd卡了老子6H
      dgram->header.compute_checksum();
      uint32_t dst = dgram->header.dst;
      auto it = routetable.end();
      std::optional<uint8_t> max_length {};
      for ( auto tableit = routetable.begin(); tableit != routetable.end(); ++tableit ) {
        if ( tableit->prefix_length == 0
             || ( tableit->route_prefix >> ( 32 - tableit->prefix_length ) )
                  == ( dst >> ( 32 - tableit->prefix_length ) ) ) {
          if ( !max_length.has_value() || *max_length < tableit->prefix_length ) {
            max_length = tableit->prefix_length;
            it = tableit;
          }
        }
      }
      if ( it != routetable.end() ) {
        interface( it->interface_index )
          .send_datagram( *dgram, it->next_hop.value_or( Address::from_ipv4_numeric( dst ) ) );
      }

      dgram = current_interface.maybe_receive();
    }
  }
}
