# EE186-Lab3

## Getting Ready
(a)

| Sensor | 7-bit address | Address byte including R/W bit `b` |
|---|---|---|
| Accelerometer | `0x19` | `0011001b` |
| Magnetometer | `0x1E` | `0011110b` |

`b = 0` for writing; `b = 1` for reading.

(b)

The device address selects the sensor; the sub-address selects a register inside it, such as a configuration register or an acceleration output register.

(c)

Write `0x57` to `CTRL_REG1_A` at address `0x20`. This selects 100 Hz and enables all three axes.


0101 | 0 | 1 | 1 | 1
rate   LP  Z   Y   X


- `0101`: 100 measurements per second.
- `0`: disable low-power mode.
- `111`: enable Z, Y, and X.

Thus `01010111₂ = 0x57`. 100 Hz is our chosen rate

(d)

| Axis | Low-byte register | High-byte register |
|---|---|---|
| X | `0x28` = `00101000` | `0x29` = `00101001` |
| Y | `0x2A` = `00101010` | `0x2B` = `00101011` |
| Z | `0x2C` = `00101100` | `0x2D` = `00101101` |


## Writing the Driver



