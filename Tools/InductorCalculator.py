# Parameters
Vin_min = 5  # Minimum input voltage (V)
Vin_max = 27  # Maximum input voltage (V)
Vout = 24  # Output voltage (V)
I_load_max = 46  # Maximum load current (A)
f_sw = 150e3  # Switching frequency (Hz)
ripple_percentage = 0.3  # Assumed ripple current percentage (30% of I_load_max)

# Calculate duty cycle (D) for buck-boost mode (worst-case)
D = Vout / (Vin_min + Vout)

# Calculate ripple current (Delta_I_L)
Delta_I_L = ripple_percentage * I_load_max

# Calculate required inductance (L)
L = ((Vout * (1 - D)) / (Delta_I_L * f_sw))*1000000

# Calculate peak inductor current
I_peak = (I_load_max + (Delta_I_L / 2))*1.2
# Results
print("Inductance =", L, "uH")
print("Isat = ", I_peak, "A")
