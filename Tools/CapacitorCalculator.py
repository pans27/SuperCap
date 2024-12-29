# Redefine all necessary parameters due to session reset

# Parameters
Vin_min = 5  # Minimum input voltage (V)
Vin_max = 27  # Maximum input voltage (V)
Vout = 24  # Output voltage (V)
I_load_max_new = 46  # Maximum load current (A)
f_sw_updated = 150e3  # Updated switching frequency (150 kHz)
V_in_ripple = 0.5  # Allowable input voltage ripple (V)
V_out_ripple = 0.5  # Allowable output voltage ripple (V)

# Duty cycle with Vin_max
D_updated = Vout / (Vin_max + Vout)

# Input capacitor (C_in)
I_cin_rms_updated = I_load_max_new * D_updated * (1 - D_updated)
C_in_updated = (I_cin_rms_updated / (4 * f_sw_updated * V_in_ripple))*1000000

# Output capacitor (C_out)
C_out_updated = ((I_load_max_new * D_updated * (1 - D_updated)) / (f_sw_updated * V_out_ripple))*1000000

# Results
print("C cap =", C_in_updated, "uF")
print("C bus =", C_out_updated, "uF")
