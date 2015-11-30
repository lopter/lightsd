#include <sys/tree.h>
#include <endian.h>
#include <stdbool.h>
#include <stdint.h>

#include <event2/util.h>

#include "lifx/wire_proto.h"

int
main(void)
{
    union u { float f; uint32_t i; };
    union u value = { .i = 0x11223344 };
    union u new_value = { .f = lgtd_lifx_wire_htolefloat(value.f) };
    return new_value.i != 0x44332211;
}
