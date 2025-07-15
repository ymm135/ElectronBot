/**
 * @file random_flash_utils.cpp
 * @brief STM32 Flash模拟EEPROM实现
 * @details 使用Flash最后一页模拟EEPROM功能，提供字节级读写接口
 *          支持缓冲区模式，减少Flash擦写次数，延长使用寿命
 */
#ifndef __STM32_EEPROM_HPP
#define __STM32_EEPROM_HPP

#include <string.h>
#include "random_flash_utils.h"

#ifdef __cplusplus
extern "C" {

#endif

/* 配置Flash Bank编号 - 根据不同STM32系列自动选择 */
#if !defined(FLASH_BANK_NUMBER) && \
    (defined(STM32F0xx) || defined(STM32F1xx) || defined(STM32G4xx) || \
     defined(STM32H7xx) || defined(STM32L4xx) || defined(STM32L5xx))
/* For STM32F0xx, FLASH_BANK_1 is not defined only FLASH_BANK1_END is defined */
#if defined(STM32F0xx)
#define FLASH_BANK_1 1U
#endif
#if defined(FLASH_BANK_2)
#define FLASH_BANK_NUMBER   FLASH_BANK_2
#else
#define FLASH_BANK_NUMBER   FLASH_BANK_1
#endif /* FLASH_BANK_2 */
#ifndef FLASH_BANK_NUMBER
#error "FLASH_BANK_NUMBER could not be defined"
#endif
#endif /* !FLASH_BANK_NUMBER */

/* 配置Flash数据扇区 - 用于F2/F4/F7/H7系列 */
#if defined(STM32F2xx) || defined(STM32F4xx) || defined(STM32F7xx) || \
    defined(STM32H7xx)
#if !defined(FLASH_DATA_SECTOR)
#define FLASH_DATA_SECTOR   ((uint32_t)(FLASH_SECTOR_TOTAL - 1))
#else
#ifndef FLASH_BASE_ADDRESS
#error "FLASH_BASE_ADDRESS have to be defined when FLASH_DATA_SECTOR is defined"
#endif
#endif /* !FLASH_DATA_SECTOR */
#endif /* STM32F2xx || STM32F4xx || STM32F7xx */

/* 配置Flash页编号 - 用于G0/G4/L4/L5/WB系列 */
#if !defined(FLASH_PAGE_NUMBER) && \
    (defined (STM32G0xx) || defined(STM32G4xx) || defined (STM32L4xx) || \
     defined (STM32L5xx) || defined(STM32WBxx))
#define FLASH_PAGE_NUMBER   ((uint32_t)((FLASH_SIZE / FLASH_PAGE_SIZE) - 1))
#endif /* !FLASH_PAGE_NUMBER */

/* 配置Flash结束地址 - 根据不同系列自动计算 */

#define FLASH_END  FLASH_BANK1_END

#if !defined(FLASH_END)
#if defined (STM32F0xx) || defined (STM32F1xx)
#if defined (FLASH_BANK2_END) && (FLASH_BANK_NUMBER == FLASH_BANK_2)
#define FLASH_END  FLASH_BANK2_END
#elif defined (FLASH_BANK1_END) && (FLASH_BANK_NUMBER == FLASH_BANK_1)
#define FLASH_END  FLASH_BANK1_END
#endif

#elif defined (STM32F3xx)
static inline uint32_t get_flash_end(void)
{
  uint32_t size;
  switch ((*((uint16_t *)FLASH_SIZE_DATA_REGISTER))) {
    case 0x200U:
      size = 0x0807FFFFU;
      break;
    case 0x100U:
      size = 0x0803FFFFU;
      break;
    case 0x80U:
      size = 0x0801FFFFU;
      break;
    case 0x40U:
      size = 0x0800FFFFU;
      break;
    case 0x20U:
      size = 0x08007FFFU;
      break;
    default:
      size = 0x08003FFFU;
      break;
  }
  return size;
}
#define FLASH_END  get_flash_end()
#elif defined(STM32G0xx) || defined(STM32G4xx) || defined (STM32L4xx) || \
      defined (STM32L5xx) || defined(STM32WBxx)
/* If FLASH_PAGE_NUMBER is defined by user, this is not really end of the flash */
#define FLASH_END  ((uint32_t)(FLASH_BASE + (((FLASH_PAGE_NUMBER +1) * FLASH_PAGE_SIZE))-1))
#elif defined(EEPROM_RETRAM_MODE)
#define FLASH_END  ((uint32_t)(EEPROM_RETRAM_START_ADDRESS + EEPROM_RETRAM_MODE_SIZE -1))
#elif defined(DATA_EEPROM_END)
#define FLASH_END DATA_EEPROM_END
#endif
#ifndef FLASH_END
#error "FLASH_END could not be defined"
#endif
#endif /* FLASH_END */

/* 配置Flash基地址 - 默认使用最后一页避免覆盖程序数据 */
#ifndef FLASH_BASE_ADDRESS
/*
 * 默认使用Flash最后一页存储数据
 * 防止覆盖程序代码
 */
#if defined(EEPROM_RETRAM_MODE)
#define FLASH_BASE_ADDRESS  EEPROM_RETRAM_START_ADDRESS
#else
#define FLASH_BASE_ADDRESS  ((uint32_t)((FLASH_END + 1) - FLASH_PAGE_SIZE))
#endif
#ifndef FLASH_BASE_ADDRESS
#error "FLASH_BASE_ADDRESS could not be defined"
#endif
#endif /* FLASH_BASE_ADDRESS */

#if !defined(DATA_EEPROM_BASE)
// EEPROM缓冲区，8字节对齐，用于减少Flash擦写次数
static uint8_t eeprom_buffer[E2END + 1] __attribute__((aligned(8))) = {0};
#endif

/**
  * @brief  从模拟EEPROM（Flash）读取一个字节
  * @param  pos 要读取的地址
  * @retval 从EEPROM读取的数据字节
  * @details 如果有硬件EEPROM则直接读取，否则从缓冲区读取
  */
uint8_t eeprom_read_byte(const uint32_t pos)
{
#if defined(DATA_EEPROM_BASE)
    __IO uint8_t data = 0;
    if (pos <= (DATA_EEPROM_END - DATA_EEPROM_BASE)) {
      /* 硬件EEPROM模式：pos是相对地址 */
      data = *(__IO uint8_t *)(DATA_EEPROM_BASE + pos);
    }
    return (uint8_t)data;
#else
    eeprom_buffer_fill();                                   // 填充缓冲区
    return eeprom_buffered_read_byte(pos);                  // 从缓冲区读取
#endif /* _EEPROM_BASE */
}

/**
  * @brief  向模拟EEPROM（Flash）写入一个字节
  * @param  pos 要写入的地址
  * @param  value 要写入的值
  * @details 如果有硬件EEPROM则直接写入，否则写入缓冲区并刷新到Flash
  */
void eeprom_write_byte(uint32_t pos, uint8_t value)
{
#if defined(DATA_EEPROM_BASE)
    /* 硬件EEPROM模式：pos是相对地址 */
    if (pos <= (DATA_EEPROM_END - DATA_EEPROM_BASE)) {
      if (HAL_FLASHEx_DATAEEPROM_Unlock() == HAL_OK) {
        HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE, (pos + DATA_EEPROM_BASE), (uint32_t)value);
        HAL_FLASHEx_DATAEEPROM_Lock();
      }
    }
#else
    eeprom_buffered_write_byte(pos, value);                 // 写入缓冲区
    eeprom_buffer_flush();                                  // 刷新到Flash
#endif /* _EEPROM_BASE */
}

#if !defined(DATA_EEPROM_BASE)

/**
  * @brief  从EEPROM缓冲区读取一个字节
  * @param  pos 要读取的地址
  * @retval 从缓冲区读取的数据字节
  */
uint8_t eeprom_buffered_read_byte(const uint32_t pos)
{
    return eeprom_buffer[pos];
}

/**
  * @brief  向EEPROM缓冲区写入一个字节
  * @param  pos 要写入的地址
  * @param  value 要写入的值
  */
void eeprom_buffered_write_byte(uint32_t pos, uint8_t value)
{
    eeprom_buffer[pos] = value;
}

/**
  * @brief  将Flash中的数据复制到缓冲区
  * @details 从Flash基地址读取数据到RAM缓冲区，提高读取速度
  */
void eeprom_buffer_fill(void)
{
    memcpy(eeprom_buffer, (uint8_t*) (FLASH_BASE_ADDRESS), E2END + 1);
}

#if defined(EEPROM_RETRAM_MODE)

/**
  * @brief  将缓冲区内容写入Flash（RETRAM模式）
  * @details RETRAM模式下直接内存拷贝，无需擦除操作
  */
void eeprom_buffer_flush(void)
{
  memcpy((uint8_t *)(FLASH_BASE_ADDRESS), eeprom_buffer, E2END + 1);
}

#else /* defined(EEPROM_RETRAM_MODE) */

/**
  * @brief  将缓冲区内容写入Flash（标准模式）
  * @details 先擦除Flash页，然后按字/双字写入数据
  *          支持不同STM32系列的Flash操作
  */
void eeprom_buffer_flush(void)
{
    FLASH_EraseInitTypeDef EraseInitStruct;                 // Flash擦除配置结构
    uint32_t offset = 0;                                    // 缓冲区偏移量
    uint32_t address = FLASH_BASE_ADDRESS;                  // 当前写入地址
    uint32_t address_end = FLASH_BASE_ADDRESS + E2END;      // 结束地址
#if defined (STM32F0xx) || defined (STM32F1xx) || defined (STM32F3xx) || \
    defined (STM32G0xx) || defined (STM32G4xx) || \
    defined (STM32L4xx) || defined (STM32L5xx) || defined (STM32WBxx)
    uint32_t pageError = 0;                                 // 页擦除错误标志
    uint64_t data = 0;                                      // 64位数据缓冲

    /* 擦除Flash页 */
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;      // 页擦除模式
#if defined (STM32F1xx) || defined (STM32G4xx) || defined (STM32L4xx) || \
    defined (STM32L5xx)
    EraseInitStruct.Banks = FLASH_BANK_NUMBER;              // 设置Flash Bank
#endif
#if defined (STM32G0xx) || defined (STM32G4xx) || defined (STM32L4xx) || \
    defined (STM32L5xx) || defined (STM32WBxx)
    EraseInitStruct.Page = FLASH_PAGE_NUMBER;               // 设置页号
#else
    EraseInitStruct.PageAddress = FLASH_BASE_ADDRESS;       // 设置页地址
#endif
    EraseInitStruct.NbPages = 1;                            // 擦除1页

    if (HAL_FLASH_Unlock() == HAL_OK)                       // 解锁Flash
    {
#if defined (STM32G0xx) || defined (STM32G4xx) || defined (STM32L4xx) || \
      defined (STM32L5xx) || defined (STM32WBxx)
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);         // 清除所有错误标志
#else
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR); // 清除错误标志
#endif
        if (HAL_FLASHEx_Erase(&EraseInitStruct, &pageError) == HAL_OK) // 执行页擦除
        {
            while (address <= address_end)                     // 循环写入数据
            {
                // 从缓冲区读取64位数据
                data = *((uint64_t*) ((uint8_t*) eeprom_buffer + offset));

                if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, data) == HAL_OK) // 写入双字
                {
                    address += 8;                               // 地址递增8字节
                    offset += 8;                                // 偏移量递增8字节
                } else
                {
                    address = address_end + 1;                  // 写入失败，退出循环
                }
            }
        }
        HAL_FLASH_Lock();                                       // 锁定Flash
    }
#else
    uint32_t SectorError = 0;                               // 扇区擦除错误标志
#if defined(STM32H7xx)
    uint64_t data[4] = {0x0000};                            // H7系列使用256位数据
#else
    uint32_t data = 0;                                      // 其他系列使用32位数据
#endif

    /* 擦除Flash扇区 */
#if defined(STM32H7xx)
    EraseInitStruct.Banks = FLASH_BANK_NUMBER;
#endif
    EraseInitStruct.NbPages = 1;

    HAL_FLASH_Unlock();

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) == HAL_OK)
    {
        while (address <= address_end)
        {
#if defined(STM32H7xx)
            /* 256 bits */
            memcpy(&data, eeprom_buffer + offset, 8 * sizeof(uint32_t));
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, address, (uint32_t)data) == HAL_OK) {
              address += 32;
              offset += 32;
#else
            memcpy(&data, eeprom_buffer + offset, sizeof(uint32_t));
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, data) == HAL_OK)
            {
                address += 4;
                offset += 4;
#endif
            } else
            {
                address = address_end + 1;
            }
        }
    }
    HAL_FLASH_Lock();
#endif
}

#endif /* defined(EEPROM_RETRAM_MODE) */

#endif /* ! DATA_EEPROM_BASE */

#ifdef __cplusplus
}
#endif
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
