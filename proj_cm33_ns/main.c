/*****************************************************************************
 * File Name        : main.c
 *
 * Description      : This source file contains the main routine for non-secure
 *                    application in the CM33 CPU
 *
 * Related Document : See README.md
 *
 *******************************************************************************
 * Copyright 2024-2025, Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software") is owned by Cypress Semiconductor Corporation
 * or one of its affiliates ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products.  Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 *******************************************************************************/

/*******************************************************************************
 * Header Files
 *********************************************************/
#include "cybsp.h"
#include "cy_pdl.h"
#include "retarget_io_init.h"

/*******************************************************************************
 * Macros
 *******************************************************************************/
#define RED_LED_PORT                (GPIO_PRT16)
#define RED_LED_PIN                 (7U)
#define CM55_BOOT_WAIT_TIME_USEC    (10U)
#define LPCOMP_ULP_SETTLE_TIME      (50U)
#define LPCOMP_OUTPUT_LOW           (0U)
#define LPCOMP_OUTPUT_HIGH          (1U)
#define TOGGLE_LED_PERIOD_MS        (500U)
#define LED_ON_DUR_BEFORE_HIB_IN_MS (2000U)
#define PIN_VINP                    (P10_4)
#define PIN_VINM                    (P10_5)
#define CM55_BOOT_WAIT_TIME_USEC    (10U)

/* App boot address for CM55 project */
#define CM55_APP_BOOT_ADDR          (CYMEM_CM33_0_m55_nvm_START + \
                                        CYBSP_MCUBOOT_HEADER_SIZE)
                                        
/*******************************************************************************
 * Global Variables
 *******************************************************************************/
cy_stc_lpcomp_context_t lpcomp_context;

const cy_stc_sysint_t lpcomp_irq_cfg =
{
        .intrSrc = lpcomp_0_comp_0_IRQ,
        .intrPriority = 7u
};

/*******************************************************************************
 * Function Name: main
 *******************************************************************************
 * Summary:
 * This function serves as the primary funtion for the CPU. 
 *
 * It performs the following tasks:
 * 1. System Hibernate if LP < Vref. 
 * 2. Toggle LED1 at 500ms if LP > Vref.
 * 
 * Parameters:
 *  void
 *
 * Return:
 *  int
 *
 ******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board initialization failed. Stop program execution. */
    if (CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    /* Enable global interrupts */
    __enable_irq();


    /* Initialize retarget-io middleware */
    init_retarget_io();

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    printf("************ "
            "PSOC Edge MCU: Wakeup from Hibernate using a low-power comparator "
            "************ \r\n\n");

    /* Check Reset Reason for reset when wake up from Hibernate */
    if(CY_SYSLIB_RESET_HIB_WAKEUP == (Cy_SysLib_GetResetReason() & 
                                                CY_SYSLIB_RESET_HIB_WAKEUP))
    {
        Cy_SysPm_IoUnfreeze();
        /* The reset has occurred on a wakeup from Hibernate power mode */
        printf("Wakeup from the Hibernate mode\r\n");

    }

    Cy_LPComp_Init(lpcomp_0_comp_0_HW, CY_LPCOMP_CHANNEL_0, 
                                    &lpcomp_0_comp_0_config, &lpcomp_context);

    /* Connect the local reference generator output to the comparator negative 
      input.*/
    Cy_LPComp_ConnectULPReference(lpcomp_0_comp_0_HW, CY_LPCOMP_CHANNEL_0);

    /* Enable the local reference voltage */
    Cy_LPComp_UlpReferenceEnable(lpcomp_0_comp_0_HW);

    /* Low comparator power and speed */
    Cy_LPComp_SetPower(lpcomp_0_comp_0_HW, CY_LPCOMP_CHANNEL_0, 
                                            CY_LPCOMP_MODE_ULP, &lpcomp_context);

    /* It needs 50 micro-seconds start-up time to settle in ULP mode after the 
     * block is enabled */
    Cy_SysLib_DelayUs(LPCOMP_ULP_SETTLE_TIME);

    /* CM55_APP_BOOT_ADDR must be updated if CM55 memory layout is changed.*/
    Cy_SysEnableCM55(MXCM55, CM55_APP_BOOT_ADDR, CM55_BOOT_WAIT_TIME_USEC);

    for (;;)
    {
        /* If the comparison result is high, toggles User LED1 every 500ms */
        if (LPCOMP_OUTPUT_HIGH == Cy_LPComp_GetCompare(lpcomp_0_comp_0_HW, CY_LPCOMP_CHANNEL_0))
        {
            /* Toggle User LED1 every 500ms */
            Cy_GPIO_Inv(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN);
            Cy_SysLib_Delay(TOGGLE_LED_PERIOD_MS);
            printf("In CPU Active mode, blinking USER LED1 at 500 milliseconds.\r\n\n");
        }
        else
        {
            /* Turn on USER LED1(Red) for 2 seconds to indicate the MCU entering 
             * Hibernate mode. */
            Cy_GPIO_Write(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN, 1);
            Cy_SysLib_Delay(LED_ON_DUR_BEFORE_HIB_IN_MS);
            Cy_GPIO_Write(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN, 0);
            printf("Turn on the USER LED1 for 2 seconds, de-initialize IO, "
                                          "and enter System Hibernate mode. \r\n\n");

            /* Wait for UART traffic to stop */
            while (cy_retarget_io_is_tx_active()) {};

            /* Set the low-power comparator as a wake-up source from Hibernate 
             * and jump into Hibernate */
            Cy_SysPm_SetHibernateWakeupSource((uint32_t)CY_SYSPM_HIBERNATE_LPCOMP0_HIGH);

            if(CY_SYSPM_SUCCESS != Cy_SysPm_SystemEnterHibernate())
            {
                printf("The system did not enter Hibernate mode.\r\n\r\n");
                handle_app_error();
            }
        }

    }
}
