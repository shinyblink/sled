#include "epicardium.h"
#include "trng.h"

int epic_trng_read(uint8_t *dest, size_t size)
{
	if (dest == NULL)
		return -EFAULT;

	TRNG_Read(MXC_TRNG, dest, size);

	return 0;
}
