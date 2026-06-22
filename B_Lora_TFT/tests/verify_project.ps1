$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot

function Assert-File {
    param([string]$Path)
    if (-not (Test-Path (Join-Path $root $Path))) {
        throw "Missing required file: $Path"
    }
}

function Assert-Contains {
    param([string]$Path, [string]$Pattern, [string]$Message)
    $fullPath = Join-Path $root $Path
    $content = Get-Content -Raw $fullPath
    if ($content -notmatch $Pattern) {
        throw "$Message in $Path"
    }
}

function Assert-NotContains {
    param([string]$Path, [string]$Pattern, [string]$Message)
    $fullPath = Join-Path $root $Path
    $content = Get-Content -Raw $fullPath
    if ($content -match $Pattern) {
        throw "$Message in $Path"
    }
}

Assert-File "OLED_Stm32_W25QXX.ioc"
Assert-File "Assets/boot_anim.bin"
Assert-File "tools/convert_gif_to_boot_anim.py"
Assert-File "Core/Inc/boot_anim.h"
Assert-File "Core/Inc/uart_loader.h"
Assert-File "Core/Inc/main.h"
Assert-File "Core/Inc/stm32f1xx_hal_conf.h"
Assert-File "Core/Inc/tft.h"
Assert-File "Core/Inc/w25qxx.h"
Assert-File "Core/Src/main.c"
Assert-File "Core/Src/boot_anim.c"
Assert-File "Core/Src/uart_loader.c"
Assert-File "Core/Src/stm32f1xx_hal_msp.c"
Assert-File "Core/Src/stm32f1xx_it.c"
Assert-File "Core/Src/system_stm32f1xx.c"
Assert-File "Core/Src/tft.c"
Assert-File "Core/Src/w25qxx.c"
Assert-File "Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_spi.c"
Assert-File "Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c"
Assert-File "Drivers/CMSIS/Device/ST/STM32F1xx/Include/stm32f103xb.h"
Assert-File "MDK-ARM/OLED_Stm32_W25QXX.uvprojx"
Assert-File "MDK-ARM/OLED_Stm32_W25QXX/OLED_Stm32_W25QXX.sct"
Assert-File "MDK-ARM/startup_stm32f103xb.s"
Assert-File "README.md"
Assert-File "tools/send_boot_anim.py"

Assert-Contains "OLED_Stm32_W25QXX.ioc" "Mcu\.Name=STM32F103C8Tx" "CubeMX file must target STM32F103C8T6/C8Tx"
Assert-Contains "OLED_Stm32_W25QXX.ioc" "PA5\.Signal=SPI1_SCK" "PA5 must be SPI1 SCK"
Assert-Contains "OLED_Stm32_W25QXX.ioc" "PA7\.Signal=SPI1_MOSI" "PA7 must be SPI1 MOSI"
Assert-Contains "OLED_Stm32_W25QXX.ioc" "PA9\.Signal=USART1_TX" "PA9 must be USART1 TX"
Assert-Contains "OLED_Stm32_W25QXX.ioc" "PA10\.Signal=USART1_RX" "PA10 must be USART1 RX"
Assert-Contains "OLED_Stm32_W25QXX.ioc" "PB0\.GPIO_Label=TFT_DC" "PB0 must be TFT DC/D0"
Assert-Contains "OLED_Stm32_W25QXX.ioc" "PB1\.GPIO_Label=TFT_RES" "PB1 must be TFT reset"
Assert-Contains "OLED_Stm32_W25QXX.ioc" "PA4\.GPIO_Label=TFT_CS" "PA4 must be TFT CS"
Assert-Contains "OLED_Stm32_W25QXX.ioc" "PA3\.GPIO_Label=W25QXX_CS" "PA3 must be W25Qxx CS"

Assert-Contains "Core/Inc/tft.h" "TFT_WIDTH\s+160" "TFT width must be 160 for landscape"
Assert-Contains "Core/Inc/tft.h" "TFT_HEIGHT\s+80" "TFT height must be 80 for landscape"
Assert-Contains "Core/Inc/tft.h" "TFT_XSTART\s+1" "TFT X offset must match landscape 80x160 ST7735S"
Assert-Contains "Core/Inc/tft.h" "TFT_YSTART\s+26" "TFT Y offset must match landscape 80x160 ST7735S"
Assert-Contains "Core/Src/tft.c" "TFT_Init" "TFT driver must expose TFT_Init implementation"
Assert-Contains "Core/Src/tft.c" "TFT_FillScreen" "TFT driver must expose TFT_FillScreen implementation"
Assert-Contains "Core/Src/tft.c" "TFT_WriteDataByte\(0x68\)" "TFT MADCTL must use reversed landscape orientation"
Assert-Contains "Core/Inc/tft.h" "TFT_BeginFrameWrite" "TFT API must expose frame stream begin"
Assert-Contains "Core/Inc/tft.h" "TFT_WriteFrameData" "TFT API must expose frame stream data write"
Assert-Contains "Core/Inc/tft.h" "TFT_EndFrameWrite" "TFT API must expose frame stream end"
Assert-Contains "Core/Inc/tft.h" "TFT_DrawString" "TFT API must expose string drawing"
Assert-Contains "Core/Inc/tft.h" "TFT_DrawHexByte" "TFT API must expose hex byte drawing"
Assert-Contains "Core/Src/tft.c" "TFT_DrawString" "TFT driver must implement string drawing"
Assert-Contains "Core/Src/tft.c" "TFT_DrawHexByte" "TFT driver must implement hex byte drawing"
Assert-Contains "Core/Inc/w25qxx.h" "W25QXX_ReadID" "W25Qxx API must expose JEDEC ID read"
Assert-Contains "Core/Inc/w25qxx.h" "W25QXX_ReadData" "W25Qxx API must expose data read"
Assert-Contains "Core/Inc/w25qxx.h" "W25QXX_PageProgram" "W25Qxx API must expose page program"
Assert-Contains "Core/Inc/w25qxx.h" "W25QXX_SectorErase" "W25Qxx API must expose sector erase"
Assert-Contains "Core/Inc/w25qxx.h" "W25QXX_GetCapacityBytes" "W25Qxx API must expose capacity calculation"
Assert-Contains "Core/Src/w25qxx.c" "0x9F" "W25Qxx driver must use JEDEC ID command"
Assert-Contains "Core/Src/w25qxx.c" "0x06" "W25Qxx driver must implement write enable"
Assert-Contains "Core/Src/w25qxx.c" "0x05" "W25Qxx driver must read status register"
Assert-Contains "Core/Src/w25qxx.c" "0x20" "W25Qxx driver must implement 4KB sector erase"
Assert-Contains "Core/Src/w25qxx.c" "0x02" "W25Qxx driver must implement page program"
Assert-Contains "Core/Src/w25qxx.c" "0x03" "W25Qxx driver must implement data read"
Assert-Contains "Core/Src/w25qxx.c" "HAL_SPI_TransmitReceive" "W25Qxx driver must use full-duplex SPI receive"
Assert-Contains "Core/Inc/boot_anim.h" "BOOT_ANIM_ADDRESS" "boot animation API must define flash address"
Assert-Contains "Core/Inc/boot_anim.h" "BOOT_ANIM_Play" "boot animation API must expose play function"
Assert-Contains "Core/Src/boot_anim.c" "BANI" "boot animation player must validate asset magic"
Assert-Contains "Core/Src/boot_anim.c" "BOOT_ANIM_CHUNK_SIZE\s+1024U" "boot animation should stream frames in 1KB chunks"
Assert-Contains "Core/Src/boot_anim.c" "TFT_BeginFrameWrite" "boot animation player must stream to TFT"
Assert-Contains "Core/Src/boot_anim.c" "W25QXX_ReadData" "boot animation player must read frames from W25Qxx"
Assert-Contains "tools/convert_gif_to_boot_anim.py" "RGB565" "converter must write RGB565 data"
Assert-Contains "tools/convert_gif_to_boot_anim.py" "160, 80" "converter must target 160x80"
Assert-Contains "tools/convert_gif_to_boot_anim.py" "ImageOps\.pad" "converter must preserve full image and pad background"
Assert-Contains "tools/convert_gif_to_boot_anim.py" "color=\(0,\s*0,\s*0\)" "converter padding background must be black"
Assert-Contains "Core/Inc/uart_loader.h" "UART_LOADER_Run" "UART loader API must expose run function"
Assert-Contains "Core/Src/uart_loader.c" "BOOT_ANIM_ADDRESS" "UART loader must write animation at boot animation address"
Assert-Contains "Core/Src/uart_loader.c" "start download" "UART loader must announce download prompt"
Assert-Contains "Core/Src/uart_loader.c" "UART_LOADER_CONFIRM_TIMEOUT\s+5000U" "UART loader must wait 5 seconds for yes confirmation"
Assert-Contains "Core/Src/uart_loader.c" "ok\\r\\n" "UART loader must acknowledge yes before transfer"
Assert-Contains "Core/Src/uart_loader.c" "XMODEM READY" "UART loader must enter XMODEM receive mode"
Assert-Contains "Core/Src/uart_loader.c" "XMODEM_STX" "UART loader must support XMODEM-1K STX packets"
Assert-Contains "Core/Src/uart_loader.c" "UART_LOADER_XMODEM_1K_BLOCK_SIZE\s+1024U" "UART loader must define 1KB XMODEM block size"
Assert-Contains "Core/Src/uart_loader.c" "W25QXX_PAGE_SIZE\s+256U" "UART loader must split 1KB writes into W25Qxx pages"
Assert-Contains "Core/Src/uart_loader.c" "xmodem_crc16" "UART loader must validate XMODEM CRC"
Assert-Contains "Core/Src/uart_loader.c" "XMODEM_ACK" "UART loader must acknowledge XMODEM packets"
Assert-Contains "Core/Src/uart_loader.c" "HAL_UART_Receive" "UART loader must receive data over UART"
Assert-Contains "Core/Src/uart_loader.c" "W25QXX_PageProgram" "UART loader must page-program W25Qxx"
Assert-Contains "Core/Src/uart_loader.c" "W25QXX_SectorErase" "UART loader must erase W25Qxx sectors"
Assert-Contains "Core/Src/main.c" "UART_LOADER_Run" "main must offer UART asset loader before normal boot"
Assert-Contains "Core/Src/main.c" "MX_USART1_UART_Init" "main must initialize USART1"
Assert-Contains "Core/Src/main.c" "BaudRate\s*=\s*230400" "USART1 baud rate must be 230400"
Assert-Contains "Core/Src/main.c" "PLL\.PLLState\s*=\s*RCC_PLL_ON" "System clock must enable PLL"
Assert-Contains "Core/Src/main.c" "PLL\.PLLSource\s*=\s*RCC_PLLSOURCE_HSI_DIV2" "System clock must use HSI/2 as PLL source"
Assert-Contains "Core/Src/main.c" "PLL\.PLLMUL\s*=\s*RCC_PLL_MUL16" "System clock must run HSI PLL at 64MHz"
Assert-Contains "Core/Src/main.c" "SYSCLKSource\s*=\s*RCC_SYSCLKSOURCE_PLLCLK" "System clock must use PLL as SYSCLK"
Assert-Contains "Core/Src/main.c" "APB1CLKDivider\s*=\s*RCC_HCLK_DIV2" "APB1 must stay within STM32F103 limits at 64MHz"
Assert-Contains "Core/Src/main.c" "FLASH_LATENCY_2" "Flash latency must be 2 wait states for 64MHz"
Assert-Contains "Core/Src/main.c" "BaudRatePrescaler\s*=\s*SPI_BAUDRATEPRESCALER_4" "SPI1 must run around 16MHz from 64MHz APB2"
Assert-Contains "OLED_Stm32_W25QXX.ioc" "RCC\.SYSCLKFreq_VALUE=64000000" "CubeMX reference must record 64MHz SYSCLK"
Assert-Contains "OLED_Stm32_W25QXX.ioc" "SPI1\.BaudRatePrescaler=SPI_BAUDRATEPRESCALER_4" "CubeMX reference must record SPI1 /4 prescaler"
Assert-Contains "tools/send_boot_anim.py" "pyserial" "PC sender must document pyserial dependency"
Assert-Contains "tools/send_boot_anim.py" "start download" "PC sender must wait for STM32 download prompt"
Assert-Contains "tools/send_boot_anim.py" "yes\\r\\n" "PC sender must confirm download with yes"
Assert-Contains "tools/send_boot_anim.py" "ok" "PC sender must wait for STM32 ok acknowledgement"
Assert-Contains "tools/send_boot_anim.py" "send_xmodem" "PC sender must upload with XMODEM"
Assert-Contains "tools/send_boot_anim.py" "STX\s*=\s*0x02" "PC sender must support XMODEM-1K STX"
Assert-Contains "tools/send_boot_anim.py" "BLOCK\s*=\s*1024" "PC sender must default to 1KB XMODEM blocks"
Assert-Contains "tools/send_boot_anim.py" "default=230400" "PC sender must default to 230400 baud"
Assert-Contains "tools/send_boot_anim.py" "crc16_ccitt" "PC sender must calculate XMODEM CRC"
Assert-Contains "tools/send_boot_anim.py" "make_logger" "PC sender must write diagnostic logs"
Assert-Contains "tools/send_boot_anim.py" "read_xmodem_response" "PC sender must log XMODEM responses"
Assert-Contains "Core/Src/main.c" "BOOT_ANIM_Play" "main must play boot animation before status screen"
Assert-NotContains "Core/Src/main.c" "BOOT_ANIM_ShowImage" "main must not play boot animation as a 1-second-per-image slideshow"
Assert-NotContains "Core/Src/main.c" "HAL_Delay\(1000\);\s*//\s*1 sec delay per image" "main must not delay one second between animation frames"
Assert-Contains "Core/Src/main.c" "W25QXX_ReadID" "main must read W25Qxx JEDEC ID"
Assert-Contains "Core/Src/main.c" "W25QXX_SectorErase" "main must erase one W25Qxx sector"
Assert-Contains "Core/Src/main.c" "W25QXX_PageProgram" "main must write test data to W25Qxx"
Assert-Contains "Core/Src/main.c" "W25QXX_ReadData" "main must read back test data from W25Qxx"
Assert-Contains "Core/Src/main.c" "SNOW-W25QXX" "main must write a recognizable test string"
Assert-Contains "Core/Src/main.c" "RW OK" "main must display write/read success"
Assert-Contains "Core/Src/main.c" "RW FAIL" "main must display write/read failure"
Assert-Contains "Core/Src/main.c" "W25QXX TEST" "main must display W25Qxx test title"
Assert-Contains "Core/Src/main.c" "FF FF FF" "main must show FF failure hint"
Assert-Contains "Core/Src/main.c" "00 00 00" "main must show 00 failure hint"
Assert-Contains "Core/Src/stm32f1xx_hal_msp.c" "__HAL_RCC_SPI1_CLK_ENABLE" "MSP init must enable SPI1 clock"
Assert-Contains "Core/Src/stm32f1xx_hal_msp.c" "GPIO_PIN_5\s*\|\s*GPIO_PIN_7" "MSP init must configure SPI SCK and MOSI"
Assert-Contains "Core/Src/stm32f1xx_it.c" "SysTick_Handler" "interrupt file must provide SysTick_Handler for HAL_Delay"
Assert-Contains "Core/Inc/stm32f1xx_hal_conf.h" "#define HAL_SPI_MODULE_ENABLED" "HAL config must enable SPI"
Assert-Contains "Core/Inc/stm32f1xx_hal_conf.h" "#define HAL_UART_MODULE_ENABLED" "HAL config must enable UART"
Assert-Contains "Core/Inc/stm32f1xx_hal_conf.h" "#define HAL_DMA_MODULE_ENABLED" "HAL config must enable DMA types required by SPI HAL"
Assert-Contains "MDK-ARM/OLED_Stm32_W25QXX.uvprojx" "STM32F103C8" "Keil project must target STM32F103C8"
Assert-Contains "MDK-ARM/OLED_Stm32_W25QXX.uvprojx" "boot_anim\.c" "Keil project must include boot animation player"
Assert-Contains "MDK-ARM/OLED_Stm32_W25QXX.uvprojx" "uart_loader\.c" "Keil project must include UART loader"
Assert-Contains "MDK-ARM/OLED_Stm32_W25QXX.uvprojx" "stm32f1xx_hal_uart\.c" "Keil project must include HAL UART driver"
Assert-Contains "MDK-ARM/OLED_Stm32_W25QXX.uvprojx" "tft\.c" "Keil project must include TFT driver"
Assert-Contains "MDK-ARM/OLED_Stm32_W25QXX.uvprojx" "w25qxx\.c" "Keil project must include W25Qxx driver"
Assert-Contains "MDK-ARM/OLED_Stm32_W25QXX.uvprojx" "stm32f1xx_hal_spi\.c" "Keil project must include HAL SPI driver"
Assert-Contains "MDK-ARM/OLED_Stm32_W25QXX.uvprojx" "stm32f1xx_hal_dma\.c" "Keil project must include HAL DMA driver"
Assert-Contains "MDK-ARM/OLED_Stm32_W25QXX/OLED_Stm32_W25QXX.sct" "0x08000000" "Scatter file must place code at STM32 flash base"
Assert-Contains "MDK-ARM/OLED_Stm32_W25QXX/OLED_Stm32_W25QXX.sct" "0x20000000" "Scatter file must place data at STM32 SRAM base"

$toolRoot = "D:\Project\Tool"
if (-not (Test-Path $toolRoot)) {
    throw "Tool package folder must exist: $toolRoot"
}
foreach ($toolFile in @("image_manager_gui.py", "send_boot_anim.py", "convert_gif_to_boot_anim.py", "README.md")) {
    if (-not (Test-Path (Join-Path $toolRoot $toolFile))) {
        throw "Tool package missing $toolFile"
    }
}

$guiPath = Join-Path $toolRoot "image_manager_gui.py"
$guiContent = Get-Content -Raw $guiPath
if ($guiContent -notmatch "SERIAL_BAUD\s*=\s*230400") {
    if ($guiContent -notmatch "DEFAULT_BAUD\s*=\s*230400") {
        throw "GUI tool must default to 230400 baud"
    }
}
if ($guiContent -notmatch "XMODEM_STX\s*=\s*0x02") {
    throw "GUI tool must support XMODEM-1K STX"
}
if ($guiContent -notmatch "XMODEM_BLOCK\s*=\s*1024") {
    throw "GUI tool must default to 1KB XMODEM blocks"
}
if ($guiContent -notmatch "ImageOps\.pad") {
    throw "GUI tool must preserve full image and pad background"
}
if (($guiContent -notmatch "color=\(0,\s*0,\s*0\)") -and ($guiContent -notmatch "color=bg_color")) {
    throw "GUI tool padding background must be configurable or black"
}
if ($guiContent -match "ImageOps\.fit") {
    throw "GUI tool must not crop images with ImageOps.fit"
}

$tftContent = Get-Content -Raw (Join-Path $root "Core/Src/tft.c")
$writeFrameMatch = [regex]::Match($tftContent, "void\s+TFT_WriteFrameData\s*\([^)]*\)\s*\{(?<body>.*?)\n\}", [System.Text.RegularExpressions.RegexOptions]::Singleline)
if (-not $writeFrameMatch.Success) {
    throw "TFT_WriteFrameData implementation not found"
}
$writeFrameBody = $writeFrameMatch.Groups["body"].Value
if (($writeFrameBody -notmatch "TFT_Select\(") -or ($writeFrameBody -notmatch "TFT_Unselect\(")) {
    throw "TFT_WriteFrameData must select and unselect TFT for each chunk because W25Qxx shares the SPI bus"
}

$bootAnim = Join-Path $root "Assets/boot_anim.bin"
$bootAnimBytes = [System.IO.File]::ReadAllBytes($bootAnim)
if ($bootAnimBytes.Length -lt 32) {
    throw "Boot animation asset is too small"
}
$magic = [System.Text.Encoding]::ASCII.GetString($bootAnimBytes, 0, 4)
if ($magic -ne "BANI") {
    throw "Boot animation asset must start with BANI magic"
}
$width = [BitConverter]::ToUInt16($bootAnimBytes, 4)
$height = [BitConverter]::ToUInt16($bootAnimBytes, 6)
$frames = [BitConverter]::ToUInt16($bootAnimBytes, 8)
$frameBytes = [BitConverter]::ToUInt32($bootAnimBytes, 12)
if (($width -ne 160) -or ($height -ne 80) -or ($frames -ne 50) -or ($frameBytes -ne 25600)) {
    throw "Boot animation header must be 160x80, 50 frames, 25600 bytes per frame"
}

Write-Host "Project static verification passed."
