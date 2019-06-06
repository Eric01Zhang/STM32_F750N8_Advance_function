#include "bsp_QSPI_FLASH.h" 


 static void Error_Handle(void);


/**
 * @brief  QSPI_Flash 引脚初始化，时钟初始化
 * @param  QSPI的hand
 * @retval 
 */
void QSPI_FLASH_Init(QSPI_HandleTypeDef *hqspi)
{
    
    /*使能QSPI以及GPIO的时钟,使能中断，*/
    HAL_QSPI_DeInit(hqspi);
    
    /*QSPI flash 模式配置*/
    hqspi->Instance = QUADSPI;
    (*hqspi).Init.ClockPrescaler     = 2;
    hqspi->Init.FifoThreshold      = 4;
    hqspi->Init.SampleShifting     = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    hqspi->Init.FlashSize          = POSITION_VAL(0x1000000) - 1;
    hqspi->Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_2_CYCLE;
    hqspi->Init.ClockMode          = QSPI_CLOCK_MODE_0;
    hqspi->Init.FlashID            = QSPI_FLASH_ID_1;
    hqspi->Init.DualFlash          = QSPI_DUALFLASH_DISABLE;
    /*初始化配置*/
    if (HAL_QSPI_Init(hqspi) != HAL_OK)
    {
        //Error_Handler();
    }   

    /*初始化QSPI接口*/
    BSP_QSPI_Init(hqspi);
}


/**
 * @brief  QSPI_Flash 存储器初始化
 * @param  none 
 * @retval 存储器状态
 */
uint8_t BSP_QSPI_Init(QSPI_HandleTypeDef *hqspi)
{
    QSPI_CommandTypeDef s_command;
	  uint8_t value[5] = {0xaa,0xaa,0xaa,0xaa,0xaa};
    
    /* QSPI存储器复位*/
    
    /* 使能写操作*/
    if( QSPI_WriteEnable(hqspi) != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    /* 设置四路使能的状态寄存器，使能四通道的IO2和IO3*/
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       =  WRITE_STATUS_REG_CMD;
    s_command.AddressMode       =  QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode =  QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          =  QSPI_DATA_1_LINE;
    s_command.DummyCycles       =  0;
    s_command.NbData            =  1;
    s_command.DdrMode           =  QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  =  QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          =  QSPI_SIOO_INST_EVERY_CMD;

    /* 配置命令 */
    if( HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    /* 传输数据 */
    if( HAL_QSPI_Transmit(hqspi, value,HAL_QPSI_TIMEOUT_DEFAULT_VALUE)!= HAL_OK)
    {
        return QSPI_ERROR;
    }
    return QSPI_OK; 
}

/**
 * @brief  从QSPI中读取大量数据
 * @param  pDdata：  指向要读取的数据的指针
 * @para   ReadAddr: 读取起始地址 
 * @para   Size:     要读取数据的大小
 * @retval 存储器状态  
 */
uint8_t BSP_QSPI_Read(QSPI_HandleTypeDef *hqspi,uint8_t* pData, uint32_t ReadAddr, uint32_t Size)
{
    QSPI_CommandTypeDef s_command;

    /*  初始化命令*/
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = READ_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
    s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
    s_command.Address           = ReadAddr;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_1_LINE;
    s_command.NbData            = Size;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    /* 配置命令*/
    if(HAL_QSPI_Command(hqspi,&s_command,HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    /*接受数据*/
    if(HAL_QSPI_Receive(hqspi, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }
    return QSPI_OK;
}


/**
 * @brief  写入大量数据
 * @param  pDdata：  指向要写入的数据的指针
 * @para   WriteAddr:写起始地址
 * @para   Size:     要写入的数据大小
 * @retval 存储器状态  
 * @note   首先要有一个指针指向写入数据， 并确定数据的首地址，大小，根据写入地址
 *         地址及大小判断存储器的页面，然后通过库函数HAL_QSPI_Command()发送配置命令，
 *         然后调用库函数HAL_QSPI_Trandmit 逐页写入，最后等待操作完成
 */
uint8_t BSP_QSPI_Write(QSPI_HandleTypeDef *hqspi,uint8_t *pData, uint32_t WriteAddr, uint32_t Size)
{
    QSPI_CommandTypeDef s_command;
    uint32_t end_addr, current_size, current_addr;

    /* 计算写入地址与页面末尾之间的大小*/
    current_addr = 0;
    while(current_addr <= WriteAddr)
    {
        current_addr += QSPI_PLASH_PAGE_SIZE;
    }

    current_size =  current_addr - WriteAddr;

    /*检查数据大小和页面的位置*/
    if(current_size > Size)
    {
        current_size = Size;
    }

    /* 初始化地址变量*/
    current_addr = WriteAddr;
    end_addr     = WriteAddr+Size;

    /* 初始化程序命令 */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = PAGE_PROG_CMD;
    s_command.AddressMode = QSPI_ADDRESS_1_LINE;
    s_command.AddressSize = QSPI_ADDRESS_24_BITS;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_4_LINES;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  
    /* 逐页执行写入 */
    do {
    s_command.Address = current_addr;
    s_command.NbData = current_size;

     /* 启用写操作 */
    if (QSPI_WriteEnable(hqspi) != QSPI_OK) 
    {
    return QSPI_ERROR;
    }

        /* 配置命令 */
    if(HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) 
    {
        return QSPI_ERROR;
    }

    /* 传输数据 */
    if(HAL_QSPI_Transmit(hqspi, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

        /* 配置自动轮询模式等待程序结束 */
    if(QSPI_AutoPollingMemReady(hqspi) != QSPI_OK) 
    {
    return QSPI_ERROR;
    }

    /* 更新下一页编程的地址和大小变量 */
    current_addr += current_size;
    pData += current_size;
    current_size = ((current_addr + QSPI_PLASH_PAGE_SIZE) > end_addr) ? (end_addr-current_addr) : QSPI_PLASH_PAGE_SIZE;   
    } while (current_addr < end_addr);
    return QSPI_OK;
}


/** 
 * @brief  读取芯片的ID
 * @brief  none
 * @retval FLASH ID 
 */
 uint32_t QSPI_FLASH_ReadID(QSPI_HandleTypeDef *hqspi)
 {
    QSPI_CommandTypeDef s_command;
    uint32_t Temp = 0;
    uint8_t pData[3];

     /*读取ID*/
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = READ_ID_CMD;
    s_command.AddressMode = QSPI_ADDRESS_1_LINE;
    s_command.AddressSize = QSPI_ADDRESS_24_BITS;
    s_command.DataMode = QSPI_DATA_1_LINE;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DummyCycles = 0;
    s_command.NbData = 3;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
 
    if(HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
   {
        return QSPI_ERROR;
   }
   if(HAL_QSPI_Receive(hqspi, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)!= HAL_OK) 
   {
      return QSPI_ERROR;
   }
   Temp = ( pData[2] | pData[1]<<8 )| ( pData[0]<<16 );
   return Temp;
}
 
/** 
 * @brief  读取芯片的ID
 * @brief  none
 * @retval FLASH ID 
 */
uint8_t QSPI_WriteEnable(QSPI_HandleTypeDef *hqspi)
{
  QSPI_CommandTypeDef     sCommand;
  QSPI_AutoPollingTypeDef sConfig;

  /* Enable write operations ------------------------------------------ */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = WRITE_ENABLE_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
     return QSPI_ERROR; 
  }
  
  /* Configure automatic polling mode to wait for write enabling ---- */  
  sConfig.Match           = 0x02;
  sConfig.Mask            = 0x02;
  sConfig.MatchMode       = QSPI_MATCH_MODE_AND;
  sConfig.StatusBytesSize = 1;
  sConfig.Interval        = 0x10;
  sConfig.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

  sCommand.Instruction    = READ_STATUS_REG_CMD;
  sCommand.DataMode       = QSPI_DATA_1_LINE;

  if (HAL_QSPI_AutoPolling(hqspi, &sCommand, &sConfig, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
     return QSPI_ERROR; 
  }
	return QSPI_OK;
}

/**
  * @brief  This function reads the SR of the memory and awaits the EOP.
  * @param  hqspi: QSPI handle
  * @retval None
  */
uint8_t QSPI_AutoPollingMemReady(QSPI_HandleTypeDef *hqspi)
{
  QSPI_CommandTypeDef     sCommand;
  QSPI_AutoPollingTypeDef sConfig;

  /* Configure automatic polling mode to wait for memory ready ------ */  
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = READ_STATUS_REG_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_1_LINE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode         = QSPI_SIOO_INST_EVERY_CMD;

  sConfig.Match           = 0x00;
  sConfig.Mask            = 0x01;
  sConfig.MatchMode       = QSPI_MATCH_MODE_AND;
  sConfig.StatusBytesSize = 1;
  sConfig.Interval        = 0x10;
  sConfig.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

  if (HAL_QSPI_AutoPolling_IT(hqspi, &sCommand, &sConfig) != HAL_OK)
  {
     return QSPI_ERROR;
  }
  return QSPI_OK;
}
 

 
  /**
 * @brief：               擦除 QSPI 存储器的指定块
 * @param BlockAddress:   需要擦除的块地址
 * @retval：              QSPI 存储器状态
 */
 uint8_t BSP_QSPI_Erase_Block(QSPI_HandleTypeDef *hqspi,uint32_t BlockAddress)
 {
    QSPI_CommandTypeDef s_command;

    /* 初始化擦除命令 */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = SECTOR_ERASE_CMD;
    s_command.AddressMode = QSPI_ADDRESS_1_LINE;
    s_command.AddressSize = QSPI_ADDRESS_24_BITS;
    s_command.Address = BlockAddress;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* 启用写操作 */
    if (QSPI_WriteEnable(hqspi) != QSPI_OK)
    {
       return QSPI_ERROR;
    }
 
    /* 发送命令 */
    if(HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) 
    {
        return QSPI_ERROR;    }
    
    /* 配置自动轮询模式等待擦除结束 */
    if (QSPI_AutoPollingMemReady(hqspi) != QSPI_OK) 
        return QSPI_ERROR;
		return QSPI_OK;
 }
 
 
  

 
 
 






