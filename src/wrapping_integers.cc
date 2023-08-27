#include "wrapping_integers.hh"
#include <iostream>
using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.

  return zero_point + n;
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{

  // Your code here.
  uint64_t tmp = checkpoint;
  tmp >>= 32;
  tmp <<= 32;
  uint64_t offset = this->raw_value_ - zero_point.raw_value_;
  tmp += offset;
  uint64_t ret = tmp;
  if ( abs( int64_t( tmp + ( 1ul << 32 ) - checkpoint ) ) < abs( int64_t( tmp - checkpoint ) ) )
    ret = tmp + ( 1ul << 32 );
  if ( tmp >= ( 1ul << 32 )
       && abs( int64_t( tmp - ( 1ul << 32 ) - checkpoint ) ) < abs( int64_t( tmp - checkpoint ) ) )
    ret = tmp - ( 1ul << 32 );

  return ret;
}
