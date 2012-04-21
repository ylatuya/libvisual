/* Libvisual - The audio visualisation framework.
 *
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "config.h"
#include "lv_ringbuffer.h"
#include "lv_common.h"
#include "gettext.h"
#include <vector>

namespace LV {

  namespace {

    template <typename T>
    T mod_add (T x, T y, T cycle)
    {
        // NOTE: this only works if x and y are positive
        return (x + y) % cycle;
    }

    template <typename T>
    T mod_sub (T x, T y, T cycle)
    {
        // NOTE: this only works if x and y are positive

        // FIXME: there should be a more direct formula
        x -= y;
        while (x < 0)
            x += cycle;

        return x;
    }

  } // anonymous

  class RingBuffer::Impl
  {
  public:

      std::vector<uint8_t> buffer;
      visual_size_t        start;
      visual_size_t        end;
      visual_size_t        avail;

      Impl (visual_size_t size)
          : buffer (size)
          , start  (0)
          , end    (0)
          , avail  (0)
      {
          // empty
      }
  };

  RingBuffer::RingBuffer (visual_size_t size)
      : m_impl (new Impl (size))
  {
      // empty
  }

  RingBuffer::~RingBuffer ()
  {
      // empty
  }

  void RingBuffer::clear ()
  {
      visual_mem_set (&m_impl->buffer[0], 0, m_impl->buffer.size ());

      m_impl->start = 0;
      m_impl->end   = 0;
      m_impl->avail = 0;
  }

  void RingBuffer::set_size (visual_size_t size)
  {
      m_impl->buffer.resize (size);

      clear ();
  }

  visual_size_t RingBuffer::get_size () const
  {
      return m_impl->buffer.size ();
  }

  visual_size_t RingBuffer::get_avail () const
  {
      return m_impl->avail;
  }

  visual_size_t RingBuffer::read (void* data, visual_size_t nbytes)
  {
      // Nothing to do if there is nothing to read
      if (nbytes == 0)
          return 0;

      // We can read only as much as what's available
      visual_size_t read_count = std::min (nbytes, m_impl->avail);
      visual_size_t read_start = m_impl->start;

      uint8_t* buffer = &m_impl->buffer[0];

      // Check if we need to read across the buffer edge
      if (read_start + read_count > m_impl->buffer.size ()) {
          // Yes, we perform the read in two chunks
          visual_size_t chunk1_size = m_impl->buffer.size () - read_start;
          visual_size_t chunk2_size = read_count - chunk1_size;
          visual_mem_copy (data, buffer + read_start, chunk1_size);
          visual_mem_copy (static_cast<uint8_t*> (data) + chunk1_size, buffer, chunk2_size);
      } else {
          // Otherwise, a single read suffices
          visual_mem_copy (data, buffer + read_start, read_count);
      }

      // Update the start position and number of unread bytes
      m_impl->start = mod_add (m_impl->start, nbytes, m_impl->buffer.size ());
      m_impl->avail -= read_count;

      return read_count;
  }

  visual_size_t RingBuffer::write (void const* data, visual_size_t nbytes)
  {
      // Nothing to do if there is nothing to write
      if (nbytes == 0)
          return 0;

      // Writing is a bit more elaborate than reading as overflows are accepted. First we calculate the minimum amount
      // of bytes we need to copy, and where in the buffer those bytes will land
      visual_size_t write_count = std::min (nbytes, m_impl->buffer.size ());
      visual_size_t write_end   = mod_add (m_impl->end, nbytes, m_impl->buffer.size ());
      visual_size_t write_start = mod_sub (write_end, write_count - 1, m_impl->buffer.size ());

      uint8_t* buffer = &m_impl->buffer[0];

      // Only the last write_count bytes of the input data need to be copied
      data = static_cast<uint8_t const*> (data) + nbytes - write_count;

      // Check if we are writing across the buffer edge
      if (write_start + write_count > m_impl->buffer.size ()) {
          // Yes, we perform the write in two chunks
          visual_size_t chunk1_size = m_impl->buffer.size () - write_start;
          visual_size_t chunk2_size = write_count - chunk1_size;
          visual_mem_copy (buffer + write_start, data, chunk1_size);
          visual_mem_copy (buffer, static_cast<uint8_t const*> (data) + chunk1_size, chunk2_size);
      } else {
          // Otherwise, a single write suffices
          visual_mem_copy (buffer + write_start, data, write_count);
      }

      bool overflowed = m_impl->avail + nbytes > m_impl->buffer.size ();

      // Update start and end positions, and number of unread bytes
      m_impl->start = overflowed ? write_start : m_impl->start;
      m_impl->end   = write_end;
      m_impl->avail = std::min (m_impl->avail + nbytes, m_impl->buffer.size ());

      return write_count;
  }

} // LV namespace
