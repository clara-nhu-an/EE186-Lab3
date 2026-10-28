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

<img width="674" height="521" alt="image" src="https://github.com/user-attachments/assets/160d832a-0c6a-4d98-96ff-654b844aabab" />

Code source is in lab3 part4.c file


## Measuring Gravity

(a)

https://github.com/user-attachments/assets/92135fe9-c43b-4182-a0a3-919eafd4cd77

Code is in lab3 part5a.c file 



(b)
Code is in part5b.c file 

I modified the implementation to read all six acceleration bytes using a single call to `HAL_I2C_Mem_Read()`. The registers are consecutive: X occupies `0x28–0x29`, Y occupies `0x2A–0x2B`, and Z occupies `0x2C–0x2D`.

The function arguments change as follows:

- MemAddress: `0x28 | 0x80 = 0xA8`. Setting bit 7 of the sub-address enables automatic register-address increment.
- pData: points to a six-byte array, `uint8_t data[6]`.
- Size: changes from `1` to `6`, receiving all three axes in one read.
- DevAddress: remains `0x19 << 1`, the accelerometer address in STM32 HAL format.
- MemAddSize: remains `I2C_MEMADD_SIZE_8BIT`, because the sub-address is still eight bits.

The sensor returns the bytes in the order X-low, X-high, Y-low, Y-high, Z-low, Z-high. Each pair is combined and interpreted as a signed 16-bit two’s-complement value. With the configured ±2 g, high-resolution mode, the conversion remains acceleration in g = raw / 16000.


this is the added code:

```
uint8_t data[6];

HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
    &hi2c1,
    ACCEL_ADDRESS << 1,
    0x28u | 0x80u,
    I2C_MEMADD_SIZE_8BIT,
    data,
    sizeof(data),
    100
);
```






