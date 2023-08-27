#include "reassembler.hh"
#include <iostream>
using namespace std;
void Reassembler::cache_insert( uint64_t first_index, string data )
{
  cache[first_index] = data;
  pendingbytes += data.size();
}
void Reassembler::cache_remove( map<uint64_t, string>::iterator it )
{
  pendingbytes -= it->second.size();
  cache.erase( it->first );
}
void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  // Your code here.
  if ( is_last_substring ) {
    iseof = true;
  }
  // 超过最大直接return，或者截掉
  uint64_t startnum = 0;
  if ( expectedindex > first_index ) {
    startnum = expectedindex - first_index;
    first_index += startnum;
  }
  if ( first_index >= expectedindex + output.available_capacity() )
    return;
  // 截去已经push的和超过capacity的
  if ( startnum < data.size() ) {
    data = data.substr( startnum, expectedindex + output.available_capacity() - first_index );
  } else {
    data = "";
  }

  // 先不管那么多，直接在cache里合并字串
  auto up = cache.upper_bound( first_index );
  if ( up != cache.begin() ) {
    --up;
  }
  if ( !cache.empty() ) {
    // merge
    uint64_t last_index = first_index + data.size();
    auto it = up;
    while ( it != cache.end() && it->first < last_index ) {
      uint64_t itfirst_index = it->first;
      string itdata = it->second;
      uint64_t itlast_index = itfirst_index + itdata.size();

      if ( itlast_index >= last_index ) {
        // larger,no need to insert
        if ( itfirst_index <= first_index ) {
          return;
        }
        // overlapping
        else {
          data += itdata.substr( last_index - itfirst_index );
          up = it;
          ++it;
          cache_remove( up );
        }
      } else if ( itlast_index < last_index && itlast_index > first_index ) {
        // overlapping
        if ( itfirst_index < first_index ) {
          data = itdata + data.substr( itlast_index - first_index );
          first_index = itfirst_index;
          up = it;
          ++it;
          cache_remove( up );
        }
        // smaller
        else {
          up = it;
          ++it;
          cache_remove( up );
        }
      }
      // no overlapping
      else {
        ++it;
      }
    }
  }

  // 统一insert
  cache_insert( first_index, data );
  auto it = cache.begin();
  while ( it != cache.end() && it->first == expectedindex ) {
    output.push( it->second );
    expectedindex += it->second.size();
    cache_remove( it );
    it = cache.begin();
  }

  if ( iseof && cache.empty() ) {
    output.close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return pendingbytes;
}
