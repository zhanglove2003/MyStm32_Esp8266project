# OLED_Stm32_W25QXX

STM32F103C8T6 + 0.96 inch SPI TFT + W25Qxx JEDEC ID test. The display is configured as 160x80 landscape.

## Wiring

| STM32F103C8T6 | TFT | W25Qxx |
| --- | --- | --- |
| 3.3V | VCC | VCC |
| GND | GND | GND |
| PA5 / SPI1_SCK | SCL | CLK |
| PA7 / SPI1_MOSI | SDA | DI |
| PA6 / SPI1_MISO | - | DO |
| PA4 | CS | - |
| PA3 | - | CS |
| PB0 | D0 / DC | - |
| PB1 | RES | - |
| PB10 or 3.3V | BLK | - |

## Files

- `OLED_Stm32_W25QXX.ioc`: CubeMX project for STM32F103C8T6, MDK-ARM V5 target.
- `Core/Src/main.c`: HAL init and W25Qxx JEDEC ID display loop.
- `Core/Src/boot_anim.c`: boot animation player, streaming RGB565 frames from W25Qxx.
- `Core/Src/uart_loader.c`: USART1 loader for writing `boot_anim.bin` into W25Qxx.
- `Core/Src/tft.c`: ST7735/ST7735S 160x80 landscape SPI TFT driver.
- `Core/Inc/tft.h`: TFT dimensions, offsets, color constants, and API.
- `Core/Src/w25qxx.c`: W25Qxx JEDEC ID read driver.
- `Assets/boot_anim.bin`: converted 160x80 RGB565 boot animation asset.
- `tools/convert_gif_to_boot_anim.py`: GIF to RGB565 boot animation converter.
- `tools/send_boot_anim.py`: PC sender for the USART1 loader.

## How to build

1. Open `MDK-ARM/OLED_Stm32_W25QXX.uvprojx` with Keil uVision.
2. Build the `OLED_Stm32_W25QXX` target.
3. Flash the generated hex to the STM32F103C8T6 board.
4. Run the board.

`OLED_Stm32_W25QXX.ioc` is kept as the CubeMX reference for the pinout. If you regenerate with CubeMX later, keep `Core/Src/tft.c`, `Core/Inc/tft.h`, and the test loop in `Core/Src/main.c`.

The test should show the W25Qxx JEDEC ID on the TFT, for example `EF 40 17`.

If the screen shows `FF FF FF`, first check W25Qxx `DO -> PA6`, `CS -> PA3`, power, and common ground. If it shows `00 00 00`, check power, module orientation, `CLK -> PA5`, and `DI -> PA7`. If the image is shifted, adjust `TFT_XSTART` and `TFT_YSTART` in `Core/Inc/tft.h`.

## Boot animation asset

Run this converter after changing the GIF:

```powershell
python tools/convert_gif_to_boot_anim.py
```

Write `Assets/boot_anim.bin` to W25Qxx address `0x000000`. The firmware checks the `BANI` header and plays the animation before showing the W25Qxx test screen. If the header is missing, the firmware skips the animation and continues normally.

USART1 loader wiring:

| USB-TTL | STM32F103C8T6 |
| --- | --- |
| TXD | PA10 |
| RXD | PA9 |
| GND | GND |

The loader now uses `230400 8N1` and XMODEM-1K by default. The packaged PC tools are in `D:\Project\Tool`.

Run the GUI tool:

```powershell
cd D:\Project\Tool
python image_manager_gui.py
```

Or send an existing animation from the command line:

```powershell
python D:\Project\Tool\send_boot_anim.py COM5 --file D:\Project\OLED_Stm32_W25QXX\Assets\boot_anim.bin
```

Use your actual USB-TTL COM port. After reset, the STM32 prints `start download`. The sender replies `yes`, waits for `ok` and `XMODEM READY`, then uploads the animation with XMODEM-1K CRC. If `230400` is unstable on your USB-TTL wiring, first fall back to `115200` on both firmware and PC tools.
