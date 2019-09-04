#include "input.h"

bool input_open(atp_t *atp, input_t *input, void *config)
{
    if (input)
    {
        input->atp = atp;
        if (!input->is_open && input->vtable.open)
        {
            input->is_open = input->vtable.open(input, config);
            return input->is_open;
        }
    }
    return false;
}

bool input_update(input_t *input, time_micros_t now)
{
    bool updated = false;
    if (input && input->is_open && input->vtable.update)
    {
        updated = input->vtable.update(input, input->atp, now);
    }
    return updated;
}

void input_close(input_t *input, void *config)
{
    if (input && input->is_open && input->vtable.close)
    {
        input->vtable.close(input, config);
        input->is_open = false;
    }
}