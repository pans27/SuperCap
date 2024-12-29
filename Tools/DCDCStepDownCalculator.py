"""
    !!!!!!!!! Not used in actual schematic!!!!!!!!!!!
"""
# Check TPS54202.pdf for reference
# Python script for TPS54202 circuit design calculations

def calculate_tps54202_components(input_voltage, output_voltage, output_current, switching_frequency=500e3, v_ref=0.596):
    """
    Calculate component values for TPS54202 circuit design.

    Parameters:
        input_voltage (float): Input voltage in volts.
        output_voltage (float): Output voltage in volts.
        output_current (float): Output current in amps.
        switching_frequency (float): Switching frequency in Hz (default 500 kHz).
        v_ref (float): Feedback reference voltage in volts (default 0.596 V).

    Returns:
        dict: Calculated component values.
    """
    # Feedback resistor values (R2 = 100k Ohm recommended)
    R2 = 100e3
    R3 = R2 * v_ref / (output_voltage - v_ref)

    # Inductor calculation
    k_ind = 0.3  # 30% ripple current
    L_min = (output_voltage * (input_voltage - output_voltage)) / (
        k_ind * output_current * switching_frequency * input_voltage
    )

    # Input capacitors
    C_in_min = 10e-6  # Minimum input capacitance in Farads
    C_in_hf = 0.1e-6  # High-frequency decoupling capacitor in Farads

    # Output capacitor
    delta_v_out = 30e-3  # Allowed output ripple voltage in volts
    C_out_min = (k_ind * output_current) / (8 * switching_frequency * delta_v_out)

    # Bootstrap capacitor
    C_boot = 0.1e-6  # Recommended bootstrap capacitor in Farads

    return {
        "Feedback Resistors": {"R2 (Ohms)": R2, "R3 (Ohms)": R3},
        "Inductor": {"L_min (Henries)": L_min},
        "Input Capacitors": {"C_in_min (Farads)": C_in_min, "C_in_hf (Farads)": C_in_hf},
        "Output Capacitors": {"C_out_min (Farads)": C_out_min},
        "Bootstrap Capacitor": {"C_boot (Farads)": C_boot},
    }

# Parameters
input_voltage = 28  # Input voltage in volts
output_voltage = 3.3  # Output voltage in volts
output_current = 2  # Output current in amps

# Calculate component values
components = calculate_tps54202_components(input_voltage, output_voltage, output_current)

# Print results
for component, values in components.items():
    print(f"{component}:")
    for key, value in values.items():
        print(f"  {key}: {value:.4e}")
