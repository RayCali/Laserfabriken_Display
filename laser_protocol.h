#pragma once

typedef struct {
    float power_pct;             // 0–100 %
    float laser_wavelength_nm;   // 1540.00–1560.00 nm
    float crystal_wavelength_nm; // 1540.00–1560.00 nm
    bool  laser_on;
} struct_message;
