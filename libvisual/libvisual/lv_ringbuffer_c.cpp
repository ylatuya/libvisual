#include "config.h"
#include "lv_ringbuffer.h"
#include "lv_common.h"

VisRingBuffer *visual_ringbuffer_new  (visual_size_t size)
{
    return new LV::RingBuffer (size);
}

void visual_ringbuffer_free (VisRingBuffer *self)
{
    delete self;
}

void visual_ringbuffer_clear (VisRingBuffer *self)
{
    visual_return_if_fail (self != NULL);

    self->clear ();
}

visual_size_t visual_ringbuffer_write (VisRingBuffer *self, const void *data, visual_size_t nbytes)
{
    visual_return_val_if_fail (self != NULL, 0);

    return self->write (data, nbytes);
}

visual_size_t visual_ringbuffer_read  (VisRingBuffer *self, void *data, visual_size_t nbytes)
{
    visual_return_val_if_fail (self != NULL, 0);

    return self->read (data, nbytes);
}

void visual_ringbuffer_set_size (VisRingBuffer *self, visual_size_t size)
{
    visual_return_if_fail (self != NULL);

    self->set_size (size);
}

visual_size_t visual_ringbuffer_get_size (VisRingBuffer *self)
{
    visual_return_val_if_fail (self != NULL, 0);

    return self->get_size ();
}

visual_size_t visual_ringbuffer_get_avail (VisRingBuffer *self)
{
    visual_return_val_if_fail (self != NULL, 0);

    return self->get_avail ();
}
