#include <stdexcept>

#include "byte_stream.hh"
#include <iostream>
using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), q() {}

void Writer::push( string data )
{
  uint64_t len = data.length() < available_capacity() ? data.length() : available_capacity();
  for ( uint64_t i = 0; i < len; ++i ) {
    q.push( data[i] );
  }
  nwrite += len;
  // Your code here.
}

void Writer::close()
{
  isclosed = true;
  // Your code here.
}

void Writer::set_error()
{
  iserror = true;
  // Your code here.
}

bool Writer::is_closed() const
{
  // Your code here.
  return isclosed;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - q.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.

  return nwrite;
}

string_view Reader::peek() const
{
  // Your code here.
  if ( q.empty() )
    return string_view( "" );
  else
    return string_view( &q.front(), 1 );
}

bool Reader::is_finished() const
{
  // Your code here.
  return isclosed && q.empty();
}

bool Reader::has_error() const
{
  // Your code here.
  return iserror;
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  uint64_t length = len < q.size() ? len : q.size();
  for ( uint64_t i = 0; i < length; ++i ) {
    q.pop();
  }
  nread += length;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return q.size();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return nread;
}
