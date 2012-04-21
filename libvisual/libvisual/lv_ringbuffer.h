/* Libvisual - The audio visualisation framework.
 *
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Chong Kai Xiong <kaixiong@codeleft.sg>
 *          Dennis Smit (ds@nerds-incorporated.org>
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

#ifndef _LV_RINGBUFFER_H
#define _LV_RINGBUFFER_H

#include <libvisual/lvconfig.h>
#include <libvisual/lv_defines.h>

/**
 * @defgroup VisRingBuffer VisRingBuffer
 * @{
 */

#ifdef __cplusplus

#include <libvisual/lv_scoped_ptr.hpp>

namespace LV {

  class RingBuffer
  {
  public:

	  explicit RingBuffer (visual_size_t size);

	  ~RingBuffer ();

      void clear ();

      void set_size (visual_size_t size);

	  visual_size_t get_size () const;

      visual_size_t get_avail () const;

      visual_size_t write (void const* data, visual_size_t nbytes);

      visual_size_t read (void* data, visual_size_t nbytes);

  private:

	  class Impl;

	  ScopedPtr<Impl> m_impl;

      RingBuffer (RingBuffer const&);
      RingBuffer& operator= (RingBuffer const&);
  };

} // LV namespace

#endif /* __cplusplus */

#ifdef __cplusplus
typedef LV::RingBuffer VisRingBuffer;
#else
typedef struct _VisRingBuffer VisRingBuffer;
struct _VisRingBuffer;
#endif

LV_BEGIN_DECLS

LV_API VisRingBuffer *visual_ringbuffer_new  (visual_size_t size);
LV_API void           visual_ringbuffer_free (VisRingBuffer *ringbuffer);

LV_API void          visual_ringbuffer_clear (VisRingBuffer *ringbuffer);
LV_API visual_size_t visual_ringbuffer_write (VisRingBuffer *ringbuffer, const void *data, visual_size_t nbytes);
LV_API visual_size_t visual_ringbuffer_read  (VisRingBuffer *ringbuffer, void *data, visual_size_t nbytes);

LV_API void          visual_ringbuffer_set_size (VisRingBuffer *ringbuffer, visual_size_t size);
LV_API visual_size_t visual_ringbuffer_get_size (VisRingBuffer *ringbuffer);

LV_END_DECLS

/**
 * @}
 */

#endif /* _LV_RINGBUFFER_H */
