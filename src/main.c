#include "stm32f4xx.h"
#include "stm32_f429.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ioe.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <string.h>

/* Filesystem includes */
#include "filesystem.h"
#include "fio.h"
//#include "romfs.h"

#include "clib.h"
#include "shell.h"
#include "host.h"

#include "cnc-controller.h"
#include "jogmode.h"

/* _sromfs symbol can be found in main.ld linker script
 * it contains file system structure of test_romfs directory
 */
extern const unsigned char _sromfs;

//static void setup_hardware();

volatile xSemaphoreHandle serial_tx_wait_sem = NULL;
/* Add for serial input */
volatile xQueueHandle serial_rx_queue = NULL;

/* IRQ handler to handle USART2 interruptss (both transmit and receive
 * interrupts). */
void USART1_IRQHandler()
{
	static signed portBASE_TYPE xHigherPriorityTaskWoken;

	/* If this interrupt is for a transmit... */
	if (USART_GetITStatus(USART1, USART_IT_TXE) != RESET) {
		/* "give" the serial_tx_wait_sem semaphore to notfiy processes
		 * that the buffer has a spot free for the next byte.
		 */
		xSemaphoreGiveFromISR(serial_tx_wait_sem, &xHigherPriorityTaskWoken);

		/* Diables the transmit interrupt. */
		USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
		/* If this interrupt is for a receive... */
	}else if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET){
		char msg = USART_ReceiveData(USART1);

		/* If there is an error when queueing the received byte, freeze! */
		xQueueSendToBackFromISR(serial_rx_queue, &msg, &xHigherPriorityTaskWoken);
	}
	else {
		/* Only transmit and receive interrupts should be enabled.
		 * If this is another type of interrupt, freeze.
		 */
		while(1);
	}

	if (xHigherPriorityTaskWoken) {
		taskYIELD();
	}
}

void send_byte(char ch)
{
	/* Wait until the RS232 port can receive another byte (this semaphore
	 * is "given" by the RS232 port interrupt when the buffer has room for
	 * another byte.
	 */
	while (!xSemaphoreTake(serial_tx_wait_sem, portMAX_DELAY));

	/* Send the byte and enable the transmit interrupt (it is disabled by
	 * the interrupt).
	 */
	USART_SendData(USART1, ch);
	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
}

char recv_byte()
{
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	char msg;
	while(!xQueueReceive(serial_rx_queue, &msg, portMAX_DELAY));
	return msg;
}
void command_prompt(void *pvParameters)
{
	char buf[128];
	char *argv[20];
    char hint[] = USER_NAME "@" USER_NAME "-STM32:~$ ";

	fio_printf(1, "\rWelcome to FreeRTOS Shell\r\n");
	while(1){
                fio_printf(1, "%s", hint);
		fio_read(0, buf, 127);
	
		int n=parse_command(buf, argv);

		/* will return pointer to the command function */
		cmdfunc *fptr=do_command(argv[0]);
		if(fptr!=NULL)
			fptr(n, argv);
		else
			fio_printf(2, "\r\n\"%s\" command not found.\r\n", argv[0]);
	}

}

/*
void system_logger(void *pvParameters)
{
    char buf[128];
    char output[512] = {0};
    char *tag = "\nName          State   Priority  Stack  Num\n*******************************************\n";
    int handle, error;
    const portTickType xDelay = 100000 / 100;

    handle = host_action(SYS_OPEN, "output/syslog", 4);
    if(handle == -1) {
        fio_printf(1, "Open file error!\n");
        return;
    }

    while(1) {
        memcpy(output, tag, strlen(tag));
        error = host_action(SYS_WRITE, handle, (void *)output, strlen(output));
        if(error != 0) {
            fio_printf(1, "Write file error! Remain %d bytes didn't write in the file.\n\r", error);
            host_action(SYS_CLOSE, handle);
            return;
        }
        vTaskList(buf);

        memcpy(output, (char *)(buf + 2), strlen((char *)buf) - 2);

        error = host_action(SYS_WRITE, handle, (void *)buf, strlen((char *)buf));
        if(error != 0) {
            fio_printf(1, "Write file error! Remain %d bytes didn't write in the file.\n\r", error);
            host_action(SYS_CLOSE, handle);
            return;
        }

        vTaskDelay(xDelay);
    }
    
    host_action(SYS_CLOSE, handle);
}
*/


int main()
{
    RCC_Configuration();
    GPIOA_Configuration();
    GPIOG_Configuration();
    USART1_Configuration();
	enable_rs232_interrupts();
	enable_rs232();
    
    LCD_Init();
    LTDC_Cmd(ENABLE);

    LCD_LayerInit();
    LCD_SetLayer(LCD_FOREGROUND_LAYER);
    LCD_Clear(LCD_COLOR_BLACK);
    LCD_SetColors(LCD_COLOR_RED, LCD_COLOR_BLACK);

    LCD_DrawRect(60, 25, 60, 120);
    LCD_DrawRect(60, 235, 60, 120);
    LCD_DrawRect(30, 110, 100, 60);
    LCD_DrawRect(150, 110, 100, 60);

    IOE_Config();
    //IOE_TPITConfig();

    GPIO_SetBits(GPIOG, GPIO_Pin_13); //Logic Analyser Debug Trigger

    TIMER2_Configuration();
    CNC_controller_init();
	
	fs_init();
	fio_init();
    
	
	/* Create the queue used by the serial task.  Messages for write to
	 * the RS232. */
	vSemaphoreCreateBinary(serial_tx_wait_sem);
	/* Add for serial input 
	 * Reference: www.freertos.org/a00116.html */
	serial_rx_queue = xQueueCreate(1, sizeof(char));

	/* Create a task to output text read from romfs. */
	xTaskCreate(command_prompt,
	            "CLI",
	            512 /* stack size */, NULL, tskIDLE_PRIORITY + 2, NULL);

	xTaskCreate(CNC_controller_depatch_task,
	            "CNC",
	            512 /* stack size */, NULL, tskIDLE_PRIORITY + 1, NULL);

	xTaskCreate(jogUI,
	            "JOG",
	            512 /* stack size */, NULL, tskIDLE_PRIORITY + 1, NULL);
#if 0
	/* Create a task to record system log. */
	xTaskCreate(system_logger,
	            (signed portCHAR *) "Logger",
	            1024 /* stack size */, NULL, tskIDLE_PRIORITY + 1, NULL);
#endif

	/* Start running the tasks. */
	vTaskStartScheduler();

	return 0;
}


void vApplicationTickHook()
{
}
