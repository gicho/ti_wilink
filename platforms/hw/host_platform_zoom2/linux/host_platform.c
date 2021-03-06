/*
 * host_platform.c
 *
 * Copyright(c) 1998 - 2009 Texas Instruments. All rights reserved.      
 * All rights reserved.                                                  
 *                                                                       
 * Redistribution and use in source and binary forms, with or without    
 * modification, are permitted provided that the following conditions    
 * are met:                                                              
 *                                                                       
 *  * Redistributions of source code must retain the above copyright     
 *    notice, this list of conditions and the following disclaimer.      
 *  * Redistributions in binary form must reproduce the above copyright  
 *    notice, this list of conditions and the following disclaimer in    
 *    the documentation and/or other materials provided with the         
 *    distribution.                                                      
 *  * Neither the name Texas Instruments nor the names of its            
 *    contributors may be used to endorse or promote products derived    
 *    from this software without specific prior written permission.      
 *                                                                       
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS   
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT     
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT      
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT   
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "tidef.h"
#include <linux/kernel.h>
#include <asm/io.h>
#include <mach/tc.h>
#include <linux/delay.h>

#include "host_platform.h"
#include "ioctl_init.h"
#include "WlanDrvIf.h"

#include "Device1273.h"


#define OS_API_MEM_ADRR  	0x0000000
#define OS_API_REG_ADRR  	0x300000

#define SDIO_ATTEMPT_LONGER_DELAY_LINUX  150

extern int g_external_board;
/*--------------------------------------------------------------------------------------*/

static void pad_config(unsigned long pad_addr, u32 andmask, u32 ormask)
{
	int val;
	u32 *addr;

	addr = (u32 *) ioremap(pad_addr, 4);
	if (!addr) {
		printk(KERN_ERR "OMAP3430_pad_config: ioremap failed with addr %lx\n", pad_addr);
		return;
	}

	val =  __raw_readl(addr);
	val &= andmask;
	val |= ormask;
	__raw_writel(val, addr);

	iounmap(addr);
}

static int OMAP3430_TNETW_Power(int power_on)
{
	int pmena_gpio = (g_external_board ? PMENA_GPIO_MCS8 : PMENA_GPIO);

	if (power_on) {
		omap_set_gpio_dataout(pmena_gpio, 1);
	} else {
		omap_set_gpio_dataout(pmena_gpio, 0);
	}

	return 0;    
}

/*-----------------------------------------------------------------------------

Routine Name:

        hPlatform_hardResetTnetw

Routine Description:

        set the GPIO to low after awaking the TNET from ELP.

Arguments:

        OsContext - our adapter context.


Return Value:

        None

-----------------------------------------------------------------------------*/

int hPlatform_hardResetTnetw(void)
{
  int err;

    /* Turn power OFF*/
  if ((err = OMAP3430_TNETW_Power(0)) == 0)
  {
    mdelay(500);
    /* Turn power ON*/
    err = OMAP3430_TNETW_Power(1);
    mdelay(50);
  }
  return err;

} /* hPlatform_hardResetTnetw() */

/* Turn device power off */
int hPlatform_DevicePowerOff (void)
{
    int err;
    
    err = OMAP3430_TNETW_Power(0);
    
    mdelay(10);
    
    return err;
}

/*--------------------------------------------------------------------------------------*/

/* Turn device power off according to a given delay */
int hPlatform_DevicePowerOffSetLongerDelay(void)
{
    int err;
    
    err = OMAP3430_TNETW_Power(0);
    
    mdelay(SDIO_ATTEMPT_LONGER_DELAY_LINUX);
    
    return err;
}

/* Turn device power on */
int hPlatform_DevicePowerOn (void)
{
    int err;

    err = OMAP3430_TNETW_Power(1);

    /* New Power Up Sequence */
    mdelay(15);
    err = OMAP3430_TNETW_Power(0);
    mdelay(1);

    err = OMAP3430_TNETW_Power(1);

    /* Should not be changed, 50 msec cause failures */
    mdelay(70);

    return err;
}

/*--------------------------------------------------------------------------------------*/

int hPlatform_Wlan_Hardware_Init(void)
{
    if(g_external_board)
    {
         pad_config(CONTROL_PADCONF_CAM_D1_MCS8, 0xFFFFFFF0, 0x0000011C); /* EN = 106 */
         pad_config(CONTROL_PADCONF_MCBSP1_CLKX_MCS8, 0xFFF0FFFF, 0x011C0000); /* IRQ = 153 */
    }
    else
    {
        /* choose gpio 101, pull up */
        /* Setting MUX Mode 4 , Pull bits 0 */
        /* Should set (x is don't change):	xxxx xxxx xxxx xxxx xxxx xxxx xxx1 1000 */
        pad_config(CONTROL_PADCONF_CAM_D1, 0xFFE0FFFF, 0x001C0000);

        /* choose gpio 162, pull up, activated */
        /* Setting MUX Mode 4 , Pull bits 3 */
        /* Should set (x is don't change):	xxxx xxxx xxxx xxxx xxxx xxxx xxx1 1100 */
        pad_config(CONTROL_PADCONF_MCBSP1_CLKX, 0xFFFFFFF0, 0x0000011C);
    }
	
	/*
	  * set pull up on all SDIO lines
	  * Setting MUX Mode of 0, and pull bits to 3
	  */

	/* set for mmc2_cmd - second half of the padconf register
	  * Should set (x is don't change):  xxxx xxxx xxx1 1000 xxxx xxxx xxxx xxxx */
	pad_config(CONTROL_PADCONF_MMC3_CMD, 0xFFFFFFF0, 0x0000011B);

	pad_config(CONTROL_PADCONF_MMC3_CLK, 0xFFF0FFE0,0x001C011A);
	
	/* set for mmc3_dat0 and dat1 - both parts of the padconf register
	  * Should set (x is don't change):  xxxx xxxx xxx1 1000 xxxx xxxx xxx1 1000 */
	pad_config(CONTROL_PADCONF_MMC3_DAT0, 0xFFF0FFF0, 0x011A011A);

	pad_config(CONTROL_PADCONF_MMC3_DAT2, 0xFFFFFFF0, 0x0000011A);

	pad_config(CONTROL_PADCONF_MMC3_DAT3, 0xFFF0FFFF, 0x011A0000);
	
	#define CONTROL_PADCONF_MMC2_DAT4       0x48002164    /* set AE4 to mmc2_dat4  set AH3 to mmc2_dat5 */
	pad_config(CONTROL_PADCONF_MMC2_DAT4, 0xFFF0FFF0, 0x00180018);
	
	#define CONTROL_PADCONF_MMC2_DAT6       0x48002168    /* set AF3 to mmc2_dat6  set AE3 to mmc2_dat7 */
	pad_config(CONTROL_PADCONF_MMC2_DAT6, 0xFFF0FFF0, 0x00180018);

	return 0;
}

/*-----------------------------------------------------------------------------

Routine Name:

        InitInterrupt

Routine Description:

        this function init the interrupt to the Wlan ISR routine.

Arguments:

        tnet_drv - Golbal Tnet driver pointer.


Return Value:

        status

-----------------------------------------------------------------------------*/

int hPlatform_initInterrupt(void *tnet_drv, void* handle_add ) 
{
	TWlanDrvIfObj *drv = tnet_drv;
    int rc;

	int irq_gpio = (g_external_board ? IRQ_GPIO_MCS8 : IRQ_GPIO);
	int pmena_gpio = (g_external_board ? PMENA_GPIO_MCS8 : PMENA_GPIO);

	if (drv->irq == 0 || handle_add == NULL)
	{
	  print_err("hPlatform_initInterrupt() bad param drv->irq=%d handle_add=0x%x !!!\n",drv->irq,(int)handle_add);
	  return -EINVAL;
	}
	printk("hPlatform_initInterrupt: call omap_request_gpio %d\n", irq_gpio);
	if (omap_request_gpio(irq_gpio) != 0) 
    {
	    print_err("hPlatform_initInterrupt() omap_request_gpio() FAILED !!\n");
		return -EINVAL;
	}
	omap_set_gpio_direction(irq_gpio, GPIO_DIR_INPUT);
#ifdef USE_IRQ_ACTIVE_HIGH
	printk("hPlatform_initInterrupt: call request_irq IRQF_TRIGGER_RISING\n");
	if ((rc = request_irq(drv->irq, handle_add, IRQF_TRIGGER_RISING, drv->netdev->name, drv)))
#else
	printk("hPlatform_initInterrupt: call request_irq IRQF_TRIGGER_FALLING\n");
	if ((rc = request_irq(drv->irq, handle_add, IRQF_TRIGGER_FALLING, drv->netdev->name, drv)))
#endif
	{
	    print_err("TIWLAN: Failed to register interrupt handler\n");
		omap_free_gpio(irq_gpio);
		return rc;
	}

	if (omap_request_gpio(pmena_gpio) != 0) 
	{
		printk(KERN_ERR "%s:OMAP2430_TNETW_Power() omap_request_gpio FAILED\n",__FILE__);
	    omap_free_gpio(irq_gpio);
		return -EINVAL;
	};
	omap_set_gpio_direction(pmena_gpio, GPIO_DIR_OUTPUT);

	return rc;

} /* hPlatform_initInterrupt() */

/*--------------------------------------------------------------------------------------*/

void hPlatform_freeInterrupt(void) 
{
	int irq_gpio = (g_external_board ? IRQ_GPIO_MCS8 : IRQ_GPIO);

	omap_free_gpio(irq_gpio);
   
}

/****************************************************************************************
 *                        hPlatform_hwGetRegistersAddr()                                 
 ****************************************************************************************
DESCRIPTION:	

ARGUMENTS:		

RETURN:			

NOTES:         	
*****************************************************************************************/
void*
hPlatform_hwGetRegistersAddr(
        TI_HANDLE OsContext
        )
{
	return (void*)OS_API_REG_ADRR;
}

/****************************************************************************************
 *                        hPlatform_hwGetMemoryAddr()                                 
 ****************************************************************************************
DESCRIPTION:	

ARGUMENTS:		

RETURN:			

NOTES:         	
*****************************************************************************************/
void*
hPlatform_hwGetMemoryAddr(
        TI_HANDLE OsContext
        )
{
	return (void*)OS_API_MEM_ADRR;
}


void hPlatform_Wlan_Hardware_DeInit(void)
{
	int pmena_gpio = (g_external_board ? PMENA_GPIO_MCS8 : PMENA_GPIO);
	omap_free_gpio(pmena_gpio);

}

