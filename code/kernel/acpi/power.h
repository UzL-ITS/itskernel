#pragma once

#include <acpi/fadt.h>

// Initializes the power management from the given FADT object.
void power_init(fadt_t *fadt);

// Resets the CPU.
void power_reset();