/*
 * @brief This file contains USB HID Keyboard example using USB ROM Drivers.
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2013
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#include "sapi.h"
//#include "Teclado.h"
#include <stdint.h>
#include <string.h>
#include "usbd_rom_api.h"
#include "hid_keyboard.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/**
 * @brief Structure to hold Keyboard data
 */
typedef struct {
	USBD_HANDLE_T hUsb;	/*!< Handle to USB stack. */
	uint8_t report[KEYBOARD_REPORT_SIZE];	/*!< Last report data  */
	uint8_t tx_busy;	/*!< Flag indicating whether a report is pending in endpoint queue. */
} Keyboard_Ctrl_T;

/** Singleton instance of Keyboard control */
static Keyboard_Ctrl_T g_keyBoard;

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

extern const uint8_t Keyboard_ReportDescriptor[];
extern const uint16_t Keyboard_ReportDescSize;

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*==================[definiciones y macros]==================================*/
typedef enum
{ 
   TeclaLiberada, 
   TeclaFalling,
   TeclaApretada, 
   TeclaRising
}estadoTeclado;
estadoTeclado  estadoTecladoActual;

/*==================[definiciones de datos externos]=========================*/

uint8_t tiempoFalling = 0, tiempoRising = 0;

// Guarda la ultima tecla apretada
uint16_t key = 0;

bool_t flagProcesarDigitoX = 0;


/* Pines del teclado matricial */

// Pines conectados a las Filas --> Salidas (MODO = OUTPUT)
uint8_t keypadRowPins[4] = {
   RS232_TXD, // Row 0
   CAN_RD,    // R1
   CAN_TD,    // R2
   T_COL1     // R3
};

// Pines conectados a las Columnas --> Entradas con pull-up (MODO = INPUT_PULLUP)
uint8_t keypadColPins[4] = {
   T_FIL0,    // Column 0
   T_FIL3,    // C1
   T_FIL2,    // C2
   T_COL0     // C3
};


// Vector para mostrar tecla presionada por UART
uint16_t asciiKeypadKeys[16] = {
                                '1', '2', '3', 'A',
                                '4', '5', '6', 'B',
                                '7', '8', '9', 'C',
                                '*', '0', '#', 'D'
                               };

// Vector para mostrar tecla presionada en el display 7 segmentos
uint16_t keypadKeys[16] = {
                               1,    2,    3, 0x0a,
                               4,    5,    6, 0x0b,
                               7,    8,    9, 0x0c,
                            0x0e,    0, 0x0f, 0x0d
                          };

// Vector para mostrar tecla presionada en el display 7 segmentos
uint16_t keypadKeysExa[16] = {
                               0x1E,    0x1F,    0x20,   0x57,
                               0x21,    0x22,    0x23,   0x56,
                               0x24,    0x25,    0x26,   0x55,
                               0x53,    0x27,    0x58,   0x54
                          };                          


void actualizarEstadoTecladoMEF(void)
{
   delay(1);
   switch(estadoTecladoActual)
   {
      case TeclaLiberada:
         if( leerTecladoMatricial() )
            estadoTecladoActual = TeclaFalling;
         break;
      case TeclaFalling:
         tiempoFalling++;
         //Chequear condiciones de Transicion de estado
         if( tiempoFalling >= 50)
         {
            if ( leerTecladoMatricial() )
            {
               estadoTecladoActual = TeclaApretada;
               //uartWriteByte( UART_USB, asciiKeypadKeys[key] );
               flagProcesarDigitoX = 1;//Pongo en uno la bandera para indicar luego que se debe procesar el digito ingresado
               tecladoApretado();
            }
            else 
               estadoTecladoActual = TeclaLiberada;
            tiempoFalling = 0;
         }
         break;
      case TeclaApretada:
         if( !leerTecladoMatricial() ) //Verifico si se solto la tecla del teclado
            estadoTecladoActual = TeclaRising;
         break;
      case TeclaRising:
         tiempoRising++;
         // Chequear condiciones de transicion de estado
         if( tiempoRising >= 50 )
         {
            if( leerTecladoMatricial() )
            {
               estadoTecladoActual = TeclaLiberada;
               tecladoLiberado();
            } 
            else
               estadoTecladoActual = TeclaApretada;
            tiempoRising = 0;
         }           
         break;
      default:
         break;
   }
   
}

/* Devuelve TRUE si hay alguna tecla presionada o FALSE (0) en caso contrario.
 * Si hay tecla presionada guarda el valor en la variable key.
 * El valor es un numero de indice entre 0 y 15 */
bool_t leerTecladoMatricial( void ){

   bool_t retVal = FALSE;

   uint16_t r = 0; // Rows
   uint16_t c = 0; // Columns

   // Poner todas las filas en estado BAJO
   for( r=0; r<4; r++ ){
	  gpioWrite( keypadRowPins[r], LOW );
   }

   // Chequear todas las columnas buscando si hay alguna tecla presionada
   for( c=0; c<4; c++ ){

      // Si leo un estado BAJO en una columna entonces puede haber una tecla presionada
      if( !gpioRead( keypadColPins[c] ) )
      {

         delay( 50 ); // Anti-rebotes de 50 ms

         // Poner todas las filas en estado ALTO excepto la primera
         for( r=1; r<4; r++ ){
            gpioWrite( keypadRowPins[r], HIGH );
         }

         // Buscar que tecla esta presionada
         for( r=0; r<4; r++ ){

            // Poner la Fila[r-1] en estado ALTO y la Fila[r] en estado BAJO
            if( r>0 ){ // Exceptua el indice negativo en el array
               gpioWrite( keypadRowPins[r-1], HIGH );
            }
            gpioWrite( keypadRowPins[r], LOW );

            // Chequear la Columna[c] en Fila[r] para buscar si la tecla esta presionada
            // Si dicha tecla esta oresionada (en estado BAJO) entonces retorna
            // graba la tecla en key y retorna TRUE
            if( !gpioRead( keypadColPins[c] ) ){
               retVal = TRUE;
               key = r * 4 + c;
               /*
                  Formula de las teclas de Teclado Matricial (Keypad)
                  de 4 filas (rows) * 5 columnas (columns)

                     c0 c1 c2 c3 c4
                  r0  0  1  2  3  4
                  r1  5  6  7  8  9   Si se presiona la tecla r[i] c[j]:
                  r2 10 11 12 13 14   valor = (i) * cantidadDeColumnas + (j)
                  r3 15 16 17 18 19
               */
               return retVal;
            }
         }

      }
   }
   return retVal;
}

void inicializarEstadoTecladoMEF(void){
   estadoTecladoActual = TeclaLiberada;
}

void configurarTecladoMatricial( void ){
   
   uint8_t i = 0;
   
   // Configure Rows as Outputs
   for( i=0; i<4; i++ ){
      gpioConfig( keypadRowPins[i], GPIO_OUTPUT );
   }

   // Configure Columns as Inputs with pull-up resistors enable
   for( i=0; i<4; i++ ){
      gpioConfig( keypadColPins[i], GPIO_INPUT_PULLUP );
   }
}

void tecladoApretado( void ){
   gpioWrite( LED3, ON );
}

void tecladoLiberado( void ){
   gpioWrite( LED3, OFF );
}


/* Routine to update keyboard state */
static void Keyboard_UpdateReport(void){

   HID_KEYBOARD_CLEAR_REPORT(&g_keyBoard.report[0]);
   actualizarEstadoTecladoMEF();
   
   if( !gpioRead( TEC1 ) ){
      HID_KEYBOARD_REPORT_SET_KEY_PRESS(g_keyBoard.report, 0x39); /* 'CapsLock' */
      //HID_KEYBOARD_REPORT_SET_KEY_PRESS(g_keyBoard.report, 0x39); /* 'CapsLock' */
   }
   else if( !gpioRead( TEC2 ) ){
      HID_KEYBOARD_REPORT_SET_KEY_PRESS(g_keyBoard.report, 0x0C); /* 'i' */
   }
   else if( !gpioRead( TEC3 ) ){
      HID_KEYBOARD_REPORT_SET_KEY_PRESS(g_keyBoard.report, 0x04); /* 'a' */
   }
   else if( !gpioRead( TEC4 ) ){
      HID_KEYBOARD_REPORT_SET_KEY_PRESS(g_keyBoard.report, 0x28); /* 'ENTER' */
   }

   if( flagProcesarDigitoX == 1 ){
      switch( keypadKeysExa[key] )
      {
         case 0x1E://1
            HID_KEYBOARD_REPORT_SET_KEY_PRESS(g_keyBoard.report, 0x1E); /* '1' */
            break;
         case 0x1F://2
            HID_KEYBOARD_REPORT_SET_KEY_PRESS(g_keyBoard.report, 0x1F); /* '2' */
            break;
         case 0x20://3
            HID_KEYBOARD_REPORT_SET_KEY_PRESS(g_keyBoard.report, 0x20); /* '3' */
            break;
         case 0x21://4
            HID_KEYBOARD_REPORT_SET_KEY_PRESS(g_keyBoard.report, 0x21); /* '4' */
            break;
         case 0x22://5
            HID_KEYBOARD_REPORT_SET_KEY_PRESS(g_keyBoard.report, 0x22); /* '5' */
            break;
         case 0x23://6
            HID_KEYBOARD_REPORT_SET_KEY_PRESS(g_keyBoard.report, 0x23); /* '6' */
            break;
         case 0x24://7
            HID_KEYBOARD_REPORT_SET_KEY_PRESS(g_keyBoard.report, 0x24); /* '7' */
            break;
         case 0x25://8
            HID_KEYBOARD_REPORT_SET_KEY_PRESS(g_keyBoard.report, 0x25); /* '8' */
            break;
         case 0x26://9
            HID_KEYBOARD_REPORT_SET_KEY_PRESS(g_keyBoard.report, 0x26); /* '9' */
            break;
         case 0x27://0
            HID_KEYBOARD_REPORT_SET_KEY_PRESS(g_keyBoard.report, 0x27); /* '0' */
            break;
         case 0x57://+
            HID_KEYBOARD_REPORT_SET_KEY_PRESS(g_keyBoard.report, 0x57); /* '+' */
            break;
         case 0x56://-
            HID_KEYBOARD_REPORT_SET_KEY_PRESS(g_keyBoard.report, 0x56); /* '-' */
            break;
         case 0x55://*
            HID_KEYBOARD_REPORT_SET_KEY_PRESS(g_keyBoard.report, 0x55); /* '*' */
            break;
         case 0x54:// /
            HID_KEYBOARD_REPORT_SET_KEY_PRESS(g_keyBoard.report, 0x54); /* '/' */
            break;
         case 0x58:// Enter
            HID_KEYBOARD_REPORT_SET_KEY_PRESS(g_keyBoard.report, 0x58); /* 'Enter' */
            break;
         case 0x53:// Block Num
            HID_KEYBOARD_REPORT_SET_KEY_PRESS(g_keyBoard.report, 0x53); /* 'Block Num' */
            break;     
         default:
            break;
      }
      flagProcesarDigitoX = 0;
   }
}

/* HID Get Report Request Callback. Called automatically on HID Get Report Request */
static ErrorCode_t Keyboard_GetReport( USBD_HANDLE_T hHid,
                                       USB_SETUP_PACKET *pSetup,
                                       uint8_t * *pBuffer,
                                       uint16_t *plength ){

	/* ReportID = SetupPacket.wValue.WB.L; */
	switch (pSetup->wValue.WB.H) {

      case HID_REPORT_INPUT:
         Keyboard_UpdateReport();
         memcpy(*pBuffer, &g_keyBoard.report[0], KEYBOARD_REPORT_SIZE);
         *plength = KEYBOARD_REPORT_SIZE;
      break;

      case HID_REPORT_OUTPUT:				/* Not Supported */
      case HID_REPORT_FEATURE:			/* Not Supported */

         return ERR_USBD_STALL;
	}

	return LPC_OK;
}

/* HID Set Report Request Callback. Called automatically on HID Set Report Request */
static ErrorCode_t Keyboard_SetReport( USBD_HANDLE_T hHid,
                                       USB_SETUP_PACKET *pSetup,
                                       uint8_t * *pBuffer,
                                       uint16_t length){

   /* we will reuse standard EP0Buf */
   if (length == 0) {
      return LPC_OK;
   }

   /* ReportID = SetupPacket.wValue.WB.L; */
   switch (pSetup->wValue.WB.H){

      case HID_REPORT_OUTPUT:
         /*  If the USB host tells us to turn on the NUM LOCK LED,
          *  then turn on LED#2.
          */
         if (**pBuffer & 0x01) {
            gpioWrite( LEDB, ON );
         }
         else {
            gpioWrite( LEDB, OFF);
         }
      break;

      case HID_REPORT_INPUT:				/* Not Supported */
      case HID_REPORT_FEATURE:			/* Not Supported */

         return ERR_USBD_STALL;
   }

   return LPC_OK;
}

/* HID interrupt IN endpoint handler */
static ErrorCode_t Keyboard_EpIN_Hdlr( USBD_HANDLE_T hUsb,
                                       void *data,
                                       uint32_t event ){
   switch (event) {
      case USB_EVT_IN:
         g_keyBoard.tx_busy = 0;
         break;
   }
   return LPC_OK;
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* HID keyboard init routine */
ErrorCode_t Keyboard_init(USBD_HANDLE_T hUsb,
						  USB_INTERFACE_DESCRIPTOR *pIntfDesc,
						  uint32_t *mem_base,
						  uint32_t *mem_size){

   USBD_HID_INIT_PARAM_T hid_param;
   USB_HID_REPORT_T reports_data[1];
   ErrorCode_t ret = LPC_OK;

   /* Do a quick check of if the interface descriptor passed is the right one. */
   if( (pIntfDesc == 0) || (pIntfDesc->bInterfaceClass
       != USB_DEVICE_CLASS_HUMAN_INTERFACE)) {
      return ERR_FAILED;
   }

   /* Init HID params */
   memset((void *) &hid_param, 0, sizeof(USBD_HID_INIT_PARAM_T));
   hid_param.max_reports = 1;
   hid_param.mem_base = *mem_base;
   hid_param.mem_size = *mem_size;
   hid_param.intf_desc = (uint8_t *) pIntfDesc;

   /* user defined functions */
   hid_param.HID_GetReport = Keyboard_GetReport;
   hid_param.HID_SetReport = Keyboard_SetReport;
   hid_param.HID_EpIn_Hdlr  = Keyboard_EpIN_Hdlr;

   /* Init reports_data */
   reports_data[0].len = Keyboard_ReportDescSize;
   reports_data[0].idle_time = 0;
   reports_data[0].desc = (uint8_t *) &Keyboard_ReportDescriptor[0];
   hid_param.report_data  = reports_data;

   ret = USBD_API->hid->init(hUsb, &hid_param);

   /* update memory variables */
   *mem_base = hid_param.mem_base;
   *mem_size = hid_param.mem_size;

   /* store stack handle for later use. */
   g_keyBoard.hUsb = hUsb;
   g_keyBoard.tx_busy = 0;

   return ret;
}

/* Keyboard tasks */
void Keyboard_Tasks(void)
{
	/* check device is configured before sending report. */
	if ( USB_IsConfigured(g_keyBoard.hUsb)) {

		/* send report data */
		if (g_keyBoard.tx_busy == 0) {
			g_keyBoard.tx_busy = 1;

			/* update report based on board state */
			Keyboard_UpdateReport();
			USBD_API->hw->WriteEP( g_keyBoard.hUsb,
			                       HID_EP_IN,
			                       &g_keyBoard.report[0],
			                       KEYBOARD_REPORT_SIZE );
		}
	}
	else {
		/* reset busy flag if we get disconnected. */
		g_keyBoard.tx_busy = 0;
	}

}
