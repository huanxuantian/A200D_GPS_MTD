/**
 * This file is strip for n32g43x_flash.c for flash control only function for slm boot size
 */
#include "n32g43x_flash.h"


/* Flash Control Register bits */
#define CTRL_Set_PG       ((uint32_t)0x00000001)
#define CTRL_Reset_PG     ((uint32_t)0x00003FFE)
#define CTRL_Set_PER      ((uint32_t)0x00000002)
#define CTRL_Reset_PER    ((uint32_t)0x00003FFD)
#define CTRL_Set_MER      ((uint32_t)0x00000004)
#define CTRL_Reset_MER    ((uint32_t)0x00003FFB)
#define CTRL_Set_OPTPG    ((uint32_t)0x00000010)
#define CTRL_Reset_OPTPG  ((uint32_t)0x00003FEF)
#define CTRL_Set_OPTER    ((uint32_t)0x00000020)
#define CTRL_Reset_OPTER  ((uint32_t)0x00003FDF)
#define CTRL_Set_START    ((uint32_t)0x00000040)
#define CTRL_Set_LOCK     ((uint32_t)0x00000080)
#define CTRL_Reset_SMPSEL ((uint32_t)0x00003EFF)
#define CTRL_SMPSEL_SMP1  ((uint32_t)0x00000000)
#define CTRL_SMPSEL_SMP2  ((uint32_t)0x00000100)

/* FLASH Keys */
#define FLASH_KEY1   ((uint32_t)0x45670123)
#define FLASH_KEY2   ((uint32_t)0xCDEF89AB)

/* Delay definition */
#define EraseTimeout   ((uint32_t)0x000B0000)
#define ProgramTimeout ((uint32_t)0x00002000)

void FLASH_Unlock(void)
{
    /* Unlocks the FLASH Program Erase Controller */
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
}

/**
 * @brief  Locks the FLASH Program Erase Controller.
 * @note   This function can be used for N32G43x devices.
 *         - For N32G43x devices this function Locks Bank.
 *           to FLASH_Lock function.
 */
void FLASH_Lock(void)
{
    /* Set the Lock Bit to lock the FLASH Program Erase Controller */
    FLASH->CTRL |= CTRL_Set_LOCK;
}

/**
 * @brief  Erases a specified FLASH page.
 * @note   This function can be used for N32G43x devices.
 * @param Page_Address The page address to be erased.
 * @return FLASH Status: The returned value can be: FLASH_BUSY, FLASH_ERR_RDKEY,
 *         FLASH_ERR_PG, FLASH_ERR_PV, FLASH_ERR_WRP, FLASH_COMPL,
 *         FLASH_ERR_EV or FLASH_TIMEOUT.
 */
FLASH_STS FLASH_EraseOnePage(uint32_t Page_Address)
{
    FLASH_STS status = FLASH_COMPL;
    /* Check the parameters */
    assert_param(IS_FLASH_ADDRESS(Page_Address));

    /* Clears the FLASH's pending flags */
    FLASH_ClearFlag(FLASH_STS_CLRFLAG);
    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOpt(EraseTimeout);

    if (status == FLASH_COMPL)
    {
        /* if the previous operation is completed, proceed to erase the page */
        FLASH->CTRL |= CTRL_Set_PER;
        FLASH->ADD = Page_Address;
        FLASH->CTRL |= CTRL_Set_START;

        /* Wait for last operation to be completed */
        status = FLASH_WaitForLastOpt(EraseTimeout);

        /* Disable the PER Bit */
        FLASH->CTRL &= CTRL_Reset_PER;
    }

    /* Return the Erase Status */
    return status;
}

/**
 * @brief  Programs a word at a specified address.
 * @note   This function can be used for N32G43x devices.
 * @param Address specifies the address to be programmed.
 * @param Data specifies the data to be programmed.
 * @return FLASH Status: The returned value can be: FLASH_BUSY, FLASH_ERR_RDKEY,
 *         FLASH_ERR_PG, FLASH_ERR_PV, FLASH_ERR_WRP, FLASH_COMPL,
 *         FLASH_ERR_EV, FLASH_ERR_ADD or FLASH_TIMEOUT.
 */
FLASH_STS FLASH_ProgramWord(uint32_t Address, uint32_t Data)
{
    FLASH_STS status  = FLASH_COMPL;

    /* Check the parameters */
    assert_param(IS_FLASH_ADDRESS(Address));

    if((Address & (uint32_t)0x3) != 0)
    {
        /* The programming address is not a multiple of 4 */
        status = FLASH_ERR_ADD;
        return status;
    }

    /* Clears the FLASH's pending flags */
    FLASH_ClearFlag(FLASH_STS_CLRFLAG);
    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOpt(ProgramTimeout);

    if (status == FLASH_COMPL)
    {
        /* if the previous operation is completed, proceed to program the new word */
        FLASH->CTRL |= CTRL_Set_PG;

        *(__IO uint32_t*)Address = (uint32_t)Data;
        /* Wait for last operation to be completed */
        status = FLASH_WaitForLastOpt(ProgramTimeout);

        /* Disable the PG Bit */
        FLASH->CTRL &= CTRL_Reset_PG;
    }

    /* Return the Program Status */
    return status;
}


/**
 * @brief  Clears the FLASH's pending flags.
 * @note   This function can be used for N32G43x devices.
 * @param FLASH_FLAG specifies the FLASH flags to clear.
 *   This parameter can be any combination of the following values:
 *     @arg FLASH_FLAG_PGERR FLASH Program error flag
 *     @arg FLASH_FLAG_PVERR FLASH Program Verify ERROR flag
 *     @arg FLASH_FLAG_WRPERR FLASH Write protected error flag
 *     @arg FLASH_FLAG_EOP FLASH End of Operation flag
 *     @arg FLASH_FLAG_EVERR FLASH Erase Verify ERROR flag
 */
void FLASH_ClearFlag(uint32_t FLASH_FLAG)
{
    /* Check the parameters */
    assert_param(IS_FLASH_CLEAR_FLAG(FLASH_FLAG));

    /* Clear the flags */
    FLASH->STS |= FLASH_FLAG;
}

/**
 * @brief  Returns the FLASH Status.
 * @note   This function can be used for N32G43x devices, it is equivalent
 *         to FLASH_GetBank1Status function.
 * @return FLASH Status: The returned value can be: FLASH_BUSY, FLASH_ERR_RDKEY,
 *         FLASH_ERR_PG, FLASH_ERR_PV, FLASH_ERR_WRP, FLASH_COMPL,
 *         FLASH_ERR_EV or FLASH_TIMEOUT.
 */
FLASH_STS FLASH_GetSTS(void)
{
    FLASH_STS flashstatus = FLASH_COMPL;

    if ((FLASH->STS & FLASH_FLAG_BUSY) == FLASH_FLAG_BUSY)
    {
        flashstatus = FLASH_BUSY;
    }
    else
    {
        if ((FLASH->STS & FLASH_FLAG_PGERR) != 0)
        {
            flashstatus = FLASH_ERR_PG;
        }
        else
        {
            if ((FLASH->STS & FLASH_FLAG_PVERR) != 0)
            {
                flashstatus = FLASH_ERR_PV;
            }
            else
            {
                if ((FLASH->STS & FLASH_FLAG_WRPERR) != 0)
                {
                    flashstatus = FLASH_ERR_WRP;
                }
                else
                {
                    if ((FLASH->STS & FLASH_FLAG_EVERR) != 0)
                    {
                        flashstatus = FLASH_ERR_EV;
                    }
                    else
                    {
                        flashstatus = FLASH_COMPL;
                    }
                }
            }
        }
    }

    /* Return the Flash Status */
    return flashstatus;
}

/**
 * @brief  Waits for a Flash operation to complete or a TIMEOUT to occur.
 * @note   This function can be used for N32G43x devices,
 *         it is equivalent to FLASH_WaitForLastBank1Operation..
 * @param Timeout FLASH programming Timeout
 * @return FLASH Status: The returned value can be: FLASH_BUSY, FLASH_ERR_RDKEY,
 *         FLASH_ERR_PG, FLASH_ERR_PV, FLASH_ERR_WRP, FLASH_COMPL,
 *         FLASH_ERR_EV or FLASH_TIMEOUT.
 */
FLASH_STS FLASH_WaitForLastOpt(uint32_t Timeout)
{
    FLASH_STS status = FLASH_COMPL;

    /* Check for the Flash Status */
    status = FLASH_GetSTS();
    /* Wait for a Flash operation to complete or a TIMEOUT to occur */
    while ((status == FLASH_BUSY) && (Timeout != 0x00))
    {
        status = FLASH_GetSTS();
        Timeout--;
    }
    if (Timeout == 0x00)
    {
        status = FLASH_TIMEOUT;
    }
    /* Return the operation status */
    return status;
}

