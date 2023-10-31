#include "output.h"

bool output_open(void *data, output_t *output, void *config)
{
    if (output)
    {
        output->data = data;
        if (!output->is_open && output->vtable.open)
        {
            output->is_open = output->vtable.open(output, config);
            return output->is_open;
        }
    }

    return false;
}

bool output_update(output_t *output, time_micros_t now)
{
    bool updated = false;
    if (output && output->is_open)
    {
        updated = output->vtable.update(output, output->data, now);
    }
    return updated;
}
 
void output_close(output_t *output, void *config)
{
    if (output && output->is_open && output->vtable.close)
    {
        output->vtable.close(output, config);
        output->is_open = false;
    }
}