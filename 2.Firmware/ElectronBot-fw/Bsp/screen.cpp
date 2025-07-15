#include "screen.h"

/**
 * @brief LCD屏幕初始化
 * @param _orientation 屏幕方向（0°、90°、180°、270°）
 * @note 初始化240x240 ST7789V LCD控制器，包括复位、配置寄存器和显示参数
 */
void Screen::Init(Orientation_t _orientation)
{
    // 初始化片选信号
    LCD_CS_GPIO_Port->BSRR = (uint32_t) LCD_CS_Pin << 16U;
    ChipSelect(true);

    // 硬件复位序列
    HAL_Delay(5);
    Reset(true);        // 拉低复位信号
    HAL_Delay(10);
    Reset(false);       // 释放复位信号
    HAL_Delay(120);     // 等待复位完成

    /* ST7789V初始化序列 */
    WriteCommand(0xEF);

    WriteCommand(0xEB);
    Write1Byte(0x14);

    WriteCommand(0xFE);
    WriteCommand(0xEF);

    WriteCommand(0xEB);
    Write1Byte(0x14);

    WriteCommand(0x84);
    Write1Byte(0x40);

    WriteCommand(0x85);
    Write1Byte(0xFF);

    WriteCommand(0x86);
    Write1Byte(0xFF);

    WriteCommand(0x87);
    Write1Byte(0xFF);

    WriteCommand(0x88);
    Write1Byte(0x0A);

    WriteCommand(0x89);
    Write1Byte(0x21);

    WriteCommand(0x8A);
    Write1Byte(0x00);

    WriteCommand(0x8B);
    Write1Byte(0x80);

    WriteCommand(0x8C);
    Write1Byte(0x01);

    WriteCommand(0x8D);
    Write1Byte(0x01);

    WriteCommand(0x8E);
    Write1Byte(0xFF);

    WriteCommand(0x8F);
    Write1Byte(0xFF);


    WriteCommand(0xB6);
    Write1Byte(0x00);
    Write1Byte(0x00);

    // 设置显示方向（MADCTL寄存器）
    WriteCommand(0x36);
    switch (_orientation)
    {
        case DEGREE_0:      // 0度（正常方向）
            Write1Byte(0x18);
            break;
        case DEGREE_90:     // 90度顺时针旋转
            Write1Byte(0x28);
            break;
        case DEGREE_180:    // 180度旋转
            Write1Byte(0x48);
            break;
        case DEGREE_270:    // 270度顺时针旋转
            Write1Byte(0x88);
            break;
    }

    // 设置颜色模式（COLMOD寄存器）
    WriteCommand(0x3A);
    switch (colorMode)
    {
        case BIT_12:        // 12位颜色模式
            Write1Byte(0x03);
            break;
        case BIT_16:        // 16位颜色模式（RGB565）
            Write1Byte(0x05);
            break;
        case BIT_18:        // 18位颜色模式
            Write1Byte(0x06);
            break;
    }

    WriteCommand(0x90);
    Write1Byte(0x08);
    Write1Byte(0x08);
    Write1Byte(0x08);
    Write1Byte(0x08);

    WriteCommand(0xBD);
    Write1Byte(0x06);

    WriteCommand(0xBC);
    Write1Byte(0x00);

    WriteCommand(0xFF);
    Write1Byte(0x60);
    Write1Byte(0x01);
    Write1Byte(0x04);

    WriteCommand(0xC3);
    Write1Byte(0x13);
    WriteCommand(0xC4);
    Write1Byte(0x13);

    WriteCommand(0xC9);
    Write1Byte(0x22);

    WriteCommand(0xBE);
    Write1Byte(0x11);

    WriteCommand(0xE1);
    Write1Byte(0x10);
    Write1Byte(0x0E);

    WriteCommand(0xDF);
    Write1Byte(0x21);
    Write1Byte(0x0c);
    Write1Byte(0x02);

    WriteCommand(0xF0);
    Write1Byte(0x45);
    Write1Byte(0x09);
    Write1Byte(0x08);
    Write1Byte(0x08);
    Write1Byte(0x26);
    Write1Byte(0x2A);

    WriteCommand(0xF1);
    Write1Byte(0x43);
    Write1Byte(0x70);
    Write1Byte(0x72);
    Write1Byte(0x36);
    Write1Byte(0x37);
    Write1Byte(0x6F);

    WriteCommand(0xF2);
    Write1Byte(0x45);
    Write1Byte(0x09);
    Write1Byte(0x08);
    Write1Byte(0x08);
    Write1Byte(0x26);
    Write1Byte(0x2A);

    WriteCommand(0xF3);
    Write1Byte(0x43);
    Write1Byte(0x70);
    Write1Byte(0x72);
    Write1Byte(0x36);
    Write1Byte(0x37);
    Write1Byte(0x6F);

    WriteCommand(0xED);
    Write1Byte(0x1B);
    Write1Byte(0x0B);

    WriteCommand(0xAE);
    Write1Byte(0x77);

    WriteCommand(0xCD);
    Write1Byte(0x63);

    WriteCommand(0x70);
    Write1Byte(0x07);
    Write1Byte(0x07);
    Write1Byte(0x04);
    Write1Byte(0x0E);
    Write1Byte(0x0F);
    Write1Byte(0x09);
    Write1Byte(0x07);
    Write1Byte(0x08);
    Write1Byte(0x03);

    WriteCommand(0xE8);
    Write1Byte(0x34);

    WriteCommand(0x62);
    Write1Byte(0x18);
    Write1Byte(0x0D);
    Write1Byte(0x71);
    Write1Byte(0xED);
    Write1Byte(0x70);
    Write1Byte(0x70);
    Write1Byte(0x18);
    Write1Byte(0x0F);
    Write1Byte(0x71);
    Write1Byte(0xEF);
    Write1Byte(0x70);
    Write1Byte(0x70);

    WriteCommand(0x63);
    Write1Byte(0x18);
    Write1Byte(0x11);
    Write1Byte(0x71);
    Write1Byte(0xF1);
    Write1Byte(0x70);
    Write1Byte(0x70);
    Write1Byte(0x18);
    Write1Byte(0x13);
    Write1Byte(0x71);
    Write1Byte(0xF3);
    Write1Byte(0x70);
    Write1Byte(0x70);

    WriteCommand(0x64);
    Write1Byte(0x28);
    Write1Byte(0x29);
    Write1Byte(0xF1);
    Write1Byte(0x01);
    Write1Byte(0xF1);
    Write1Byte(0x00);
    Write1Byte(0x07);

    WriteCommand(0x66);
    Write1Byte(0x3C);
    Write1Byte(0x00);
    Write1Byte(0xCD);
    Write1Byte(0x67);
    Write1Byte(0x45);
    Write1Byte(0x45);
    Write1Byte(0x10);
    Write1Byte(0x00);
    Write1Byte(0x00);
    Write1Byte(0x00);

    WriteCommand(0x67);
    Write1Byte(0x00);
    Write1Byte(0x3C);
    Write1Byte(0x00);
    Write1Byte(0x00);
    Write1Byte(0x00);
    Write1Byte(0x01);
    Write1Byte(0x54);
    Write1Byte(0x10);
    Write1Byte(0x32);
    Write1Byte(0x98);

    WriteCommand(0x74);
    Write1Byte(0x10);
    Write1Byte(0x85);
    Write1Byte(0x80);
    Write1Byte(0x00);
    Write1Byte(0x00);
    Write1Byte(0x4E);
    Write1Byte(0x00);

    WriteCommand(0x98);
    Write1Byte(0x3e);
    Write1Byte(0x07);

    WriteCommand(0x35);     // 使能撕裂效应信号
    WriteCommand(0x21);     // 显示反转开启

    WriteCommand(0x11);     // 退出睡眠模式
    HAL_Delay(120);         // 等待退出睡眠
    WriteCommand(0x29);     // 开启显示
    HAL_Delay(20);          // 等待显示稳定

    ChipSelect(false);      // 释放片选

    SetBackLight(1);        // 开启背光
}


/**
 * @brief 设置LCD显示窗口
 * @param _startX 起始X坐标
 * @param _endX 结束X坐标
 * @param _startY 起始Y坐标
 * @param _endY 结束Y坐标
 * @note 设置后续写入像素数据的显示区域
 */
void Screen::SetWindow(uint16_t _startX, uint16_t _endX, uint16_t _startY, uint16_t _endY)
{
    ChipSelect(true);

    uint8_t data[4];

    // 设置列地址范围（X坐标）
    WriteCommand(0x2A); // COL_ADDR_SET
    data[0] = (_startX >> 8) & 0xFF;    // 起始X高字节
    data[1] = _startX & 0xFF;           // 起始X低字节
    data[2] = (_endX >> 8) & 0xFF;      // 结束X高字节
    data[3] = _endX & 0xFF;             // 结束X低字节
    WriteData(data, sizeof(data));

    // 设置行地址范围（Y坐标）
    WriteCommand(0x2B); // ROW_ADDR_SET
    data[0] = (_startY >> 8) & 0xFF;    // 起始Y高字节
    data[1] = _startY & 0xFF;           // 起始Y低字节
    data[2] = (_endY >> 8) & 0xFF;      // 结束Y高字节
    data[3] = _endY & 0xFF;             // 结束Y低字节
    WriteData(data, sizeof(data));

    ChipSelect(false);
}


/**
 * @brief 写入帧缓冲数据到LCD
 * @param _buffer 像素数据缓冲区指针
 * @param _len 数据长度（字节）
 * @param _isAppend 是否为追加模式（继续写入）
 * @note 使用DMA传输提高效率，需要先调用SetWindow设置显示区域
 */
void Screen::WriteFrameBuffer(uint8_t* _buffer, uint32_t _len, bool _isAppend)
{
    isBusy = true;          // 标记LCD忙碌状态

    ChipSelect(true);
    // 选择写入命令：追加模式或新写入模式
    _isAppend ?
    WriteCommand(0x3C) :    // MEM_WR_CONT（继续写入显存）
    WriteCommand(0x2C);     // MEM_WR（写入显存）
    WriteData(_buffer, _len, true);  // 使用DMA传输像素数据

    // 注意：使用DMA时需要等待传输完成
    ChipSelect(false);
}


/**
 * @brief 控制LCD片选信号
 * @param _enable true=选中LCD，false=释放LCD
 * @note 当前实现中片选信号被注释掉，可能使用硬件片选
 */
void Screen::ChipSelect(bool _enable)
{
//    _enable ? LCD_CS_GPIO_Port->BSRR = (uint32_t) LCD_CS_Pin << 16U :
//            LCD_CS_GPIO_Port->BSRR = LCD_CS_Pin;
}

/**
 * @brief 控制LCD复位信号
 * @param _enable true=复位有效（低电平），false=复位释放（高电平）
 */
void Screen::Reset(bool _enable)
{
    _enable ? LCD_RES_GPIO_Port->BSRR = (uint32_t) LCD_RES_Pin << 16U :
            LCD_RES_GPIO_Port->BSRR = LCD_RES_Pin;
}

/**
 * @brief 设置数据/命令选择信号
 * @param _isData true=数据模式，false=命令模式
 * @note DC信号控制SPI传输的是命令还是数据
 */
void Screen::SetDataOrCommand(bool _isData)
{
    _isData ? LCD_DC_GPIO_Port->BSRR = LCD_DC_Pin :
            LCD_DC_GPIO_Port->BSRR = (uint32_t) LCD_DC_Pin << 16U;
}

/**
 * @brief 写入LCD命令
 * @param _cmd 命令字节
 */
void Screen::WriteCommand(uint8_t _cmd)
{
    SetDataOrCommand(false);    // 设置为命令模式
    HAL_SPI_Transmit(spi, &_cmd, 1, 100);  // 通过SPI发送命令
}

/**
 * @brief 写入单字节数据
 * @param _data 数据字节
 */
void Screen::Write1Byte(uint8_t _data)
{
    SetDataOrCommand(true);     // 设置为数据模式
    HAL_SPI_Transmit(spi, &_data, 1, 100); // 通过SPI发送数据
}

/**
 * @brief 写入多字节数据
 * @param _data 数据缓冲区指针
 * @param _len 数据长度
 * @param _useDma 是否使用DMA传输
 */
void Screen::WriteData(uint8_t* _data, uint32_t _len, bool _useDma)
{
    SetDataOrCommand(true);     // 设置为数据模式
    _useDma ? HAL_SPI_Transmit_DMA(spi, _data, _len) :      // DMA传输
    HAL_SPI_Transmit(spi, _data, _len, 100);                // 阻塞传输
}

/**
 * @brief 设置LCD背光亮度
 * @param _val 亮度值（0.0-1.0）
 * @note 当前实现为简单的开/关控制
 */
void Screen::SetBackLight(float _val)
{
    if (_val < 0) _val = 0;
    else if (_val > 1.0f) _val = 1.0f;

    // 简单的背光控制：大于0就开启背光
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);
}


