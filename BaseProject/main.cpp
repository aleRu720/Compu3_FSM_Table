#include "mbed.h"
#include <stdbool.h>

#define NUMBUTT  4

#define MAXLED      4

#define HEARBEATTIME    1000

/**
 * @brief Defino el intervalo entre lecturas para "filtrar" el ruido del Botón
 * 
 */
#define DEBOUNCETIME    40

/**
 * @brief Tipo de dato FUNCION, con parámetros genéricos (void *) "casteables" a cualquier tipo de datos
 * Se va a usar como callback para la estructura de los botones.
 */
typedef void (*callAction)(void *param);

DigitalOut hearbeatLed(PC_13);

/**
 * @brief Enumeración que contiene los estados de la máquina de estados(MEF) que se implementa para 
 * el "debounce" 
 * El botón está configurado como PullUp, establece un valor lógico 1 cuando no se presiona
 * y cambia a valor lógico 0 cuando es presionado.
 */
typedef enum{
    BUTTON_DOWN,    //0
    BUTTON_UP,      //1
    BUTTON_FALLING, //2
    BUTTON_RISING   //3
}_eButtonState;

/**
 * @brief Enumeración que contiene los eventos del botón
 * Presionado=0, no_presionado=1, o nada=2 
 * Esto permite cambiar el orden o el valor en caso de PullUp o PullDown * 
 */
typedef enum{
    EV_PRESSED,
    EV_NOT_PRESSED,
    EV_NONE
}_eEvent;

/**
 * @brief Estructura para la tabla de la FSM
 * Este enfoque genera una tabla(variable), en lugar de un switch/case,
 * Esto permite que la FSM sea más flexible.
 */
typedef struct{
    _eButtonState currentState;
    _eEvent event;
    _eButtonState nextState;
}_sFsmEntry;

/**
 * @brief Tabla de la FSM
 * La combinación de BOTÓN y EVENTO, Genera el llamado a la función y cambia al próximo evento.
 * Se pueden realizar tantas combinaciones como séan necesarias. * 
 */
_sFsmEntry fsmTable[] = {   
                            {BUTTON_UP,         EV_PRESSED,     BUTTON_FALLING },
                            {BUTTON_FALLING,    EV_NOT_PRESSED, BUTTON_UP      },
                            {BUTTON_FALLING,    EV_PRESSED,     BUTTON_DOWN    }, //callback a la funcion por flanco descendente
                            {BUTTON_RISING,     EV_NOT_PRESSED, BUTTON_UP      }, //callback a la funcion por flanco ascendente
                            {BUTTON_DOWN,       EV_NOT_PRESSED, BUTTON_RISING  },
                            {BUTTON_RISING,     EV_PRESSED,     BUTTON_DOWN    }
};

//_sFsmEntry fsmTable[4];
/**
 * @brief Estructura del Botón
 * Similar a la de la FSM, pero esta, además incorpora el callback (callAction action)
 * 
 */
typedef struct{
    _eButtonState currentState;  //!< Estado actual del botón
    _eEvent event;               //!< Evento PRESSED - NOT_PRESSED - NONE
    callAction action;           //!< callback, se puede llamara a diferentes funciones por cada botón si fuese necesario
    uint16_t mask;               //!< Mascara para filtrara el valor del BusIn
}_sButtons;

_sButtons myButtons[NUMBUTT];

/**
 * @brief Función que recorre la tabñla de la FSM
 * 
 * @param index Parámetro para identificar los botones
 */
void updateDebounceFsm(uint8_t index);

/**
 * @brief Inicializa los botones, no recibe parámetros.
 * 
 */
void initializeButtons(void);

/**
 * @brief Función que llamarán los botones cuando se cumpla la combianción en ela tabla de la FSM
 * 
 * @param param  Tipo de dato (void *)  en este caso se "casteará" a uint8_t 
 */
void onButtonEvent(void *param);


BusIn buttonArray(PB_6,PB_7,PB_8,PB_9);


BusOut lesdArray(PB_12,PB_13,PB_14,PB_15);


Timer myTimer;

uint8_t indexAux;


int main()
{
    hearbeatLed=0;
    myTimer.start();
    initializeButtons();
    uint16_t hearbeat=0;
    while(true)
    {
        if((myTimer.read_ms()-hearbeat)>= HEARBEATTIME)
        {
            hearbeat=myTimer.read_ms();
            hearbeatLed=!hearbeatLed;
        }
        for (uint8_t index=0; index < NUMBUTT ; index++)
        {
            updateDebounceFsm(index);
        }
    }

    return EXIT_SUCCESS;
}


void updateDebounceFsm(uint8_t index)
{
    static uint16_t timeToDebounce=0;
    // Reviso el valor del botón cada 40 ms (DEBOUNCETIME)
    if((myTimer.read_ms()-timeToDebounce)>=DEBOUNCETIME)
    {
        timeToDebounce=myTimer.read_ms();
        myButtons[index].event = (buttonArray & myButtons[index].mask) ? EV_NOT_PRESSED : EV_PRESSED;
	}
	// Si no hay eventos, retornamos para evitar pasar por la FSM
	if(myButtons[index].event == EV_NONE) {
		return;
	}
   
	//... de otro modo, recorremos la tabla de la FSM 
	for( indexAux=0; indexAux < sizeof(fsmTable)/sizeof(_sFsmEntry); indexAux++) {
		// Si el botón coincide con una combinación de Estado y Evento de la tabla ....
		if(fsmTable[indexAux].currentState == myButtons[index].currentState && fsmTable[indexAux].event == myButtons[index].event) {
            if(indexAux==BUTTON_RISING) // ...Si la combinacion pertenece a Flanco Ascendente 
                (*(myButtons[index].action))(&index); // ... Ejecutamos la acción ...
			myButtons[index].currentState = fsmTable[indexAux].nextState; // movemos el estado del botón al próximo estado de la FSM
		}
	}
	// Limpiamos el evento para evitar pasar una y otra vez por la tabla.
	myButtons[index].event = EV_NONE;
}

void initializeButtons(void)
{
    for(indexAux=0; indexAux<NUMBUTT; indexAux++)
    {
        myButtons[indexAux].currentState=BUTTON_UP;
        myButtons[indexAux].event = EV_NONE;
        myButtons[indexAux].mask =0;
        myButtons[indexAux].mask |= 1<<indexAux;
        myButtons[indexAux].action = onButtonEvent;
    }
}

void onButtonEvent(void *param)
{
    uint8_t *index = (uint8_t *) param;
    uint16_t ledAux=lesdArray;

    if(lesdArray & myButtons[*index].mask)
        ledAux &= ~(1<<(*index));
    else
        ledAux |= 1<<(*index);
    lesdArray=ledAux;
}
