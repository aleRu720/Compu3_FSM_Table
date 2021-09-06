#include "mbed.h"

#define TRUE    1

#define NROBOTONES  4

#define MAXLED      4

#define TIMETOSTART 1000

#define TIMEMAX 1501

#define BASETIME 500

#define ESPERAR         0
#define JUEGO           1
#define JUEGOTERMINADO  2
#define TECLAS          3

/**
 * @brief Defino el intervalo entre lecturas para "filtrar" el ruido del Botón
 * 
 */
#define INTERVAL    40

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


typedef struct{
    uint8_t estado;
    int32_t timeDown;
    int32_t timeDiff;
}_sTeclas;

_sTeclas ourButton[NROBOTONES];
// 0001 , 0010,  0100, 1000
uint16_t mask[]={0x0001,0x0002,0x0004,0x0008};

uint8_t estadoJuego=ESPERAR;


/**
 * @brief Dato del tipo Enum para asignar los estados de la MEF
 * 
 */
_eButtonState myButton;

/**
 * @brief Inicializa la MEF
 * Le dá un estado inicial a myButton
 */
void startMef(uint8_t indice);

/**
 * @brief Máquina de Estados Finitos(MEF)
 * Actualiza el estado del botón cada vez que se invoca.
 * 
 * @param buttonState Este parámetro le pasa a la MEF el estado del botón.
 */
void actuallizaMef(uint8_t indice );

/**
 * @brief Función para cambiar el estado del LED cada vez que sea llamada.
 * 
 */
void togleLed(uint8_t indice);

BusIn botones(PB_6,PB_7,PB_8,PB_9);
BusOut leds(PB_12,PB_13,PB_14,PB_15);


//DigitalIn boton(PB_9); //!< DEfino la entrada para el botón

//DigitalOut LED(PC_13); //!< Defino la salida del led

Timer miTimer; //!< Timer para hacer la espera de 40 milisegundos

int tiempoMs=0; //!< variable donde voy a almacenar el tiempo del timmer una vez cumplido


int main()
{
    miTimer.start();
    uint16_t ledAuxRandom=0;
    int tiempoRandom=0;
    int ledAuxRandomTime=0;
    int ledAuxJuegoStart=0;
    uint8_t indiceAux=0;
    for(uint8_t indice=0; indice<NROBOTONES;indice++){
        startMef(indice);
    }

    while(TRUE)
    {
        switch(estadoJuego){
            case ESPERAR:
                if ((miTimer.read_ms()-tiempoMs)>INTERVAL){
                    tiempoMs=miTimer.read_ms();
                    for(uint8_t indice=0; indice<NROBOTONES;indice++){
                        actuallizaMef(indice);
                        if(ourButton[indice].timeDiff >= TIMETOSTART){
                            srand(miTimer.read_us());
                            estadoJuego=TECLAS;   
                        }
                    }
                }
            break;
            case TECLAS:
                for( indiceAux=0; indiceAux<NROBOTONES;indiceAux++){
                    actuallizaMef(indiceAux);
                    if(ourButton[indiceAux].estado!=BUTTON_UP){
                        break;
                    }
                        
                }
                if(indiceAux==NROBOTONES){
                    estadoJuego=JUEGO;
                    leds=15;
                    ledAuxJuegoStart=miTimer.read_ms();
                }
            break;
            case JUEGO:
                if(leds==0){
                    ledAuxRandom = rand() % (MAXLED);
                    ledAuxRandomTime = (rand() % (TIMEMAX))+BASETIME;
                    tiempoRandom=miTimer.read_ms();
                    leds=mask[ledAuxRandom];
                }else{
                    if((miTimer.read_ms()- ledAuxJuegoStart)> TIMETOSTART) {
                        if(leds==15){
                            ledAuxJuegoStart=miTimer.read_ms();
                            leds=0;
                        }
                    }
                }
                
                if ((miTimer.read_ms()-tiempoRandom)>ledAuxRandomTime){
                    leds=0;
                }



                

            break;
            case JUEGOTERMINADO:
            break;
            default:
                estadoJuego=ESPERAR;
        }
        /*
        if ((miTimer.read_ms()-tiempoRandom)>ledAuxRandomTime){
            leds=0;
        }*/
       
       /*
       if ((miTimer.read_ms()-tiempoMs)>INTERVAL){
           tiempoMs=miTimer.read_ms();
           for(uint8_t indice=0; indice<NROBOTONES;indice++){
               actuallizaMef(indice);
               if(ourButton[indice].timeDiff >= TIMETOSTART){
                    ourButton[indice].timeDiff=0;
                    srand(miTimer.read_us());
                    ledAuxRandom = rand() % (MAXLED);
                    togleLed(ledAuxRandom);
                    ledAuxRandomTime = (rand() % (TIMEMAX))+BASETIME;
                    tiempoRandom=miTimer.read_ms();
               }
           }
        }
        */


    }
    return 0;
}



void startMef(uint8_t indice){
   ourButton[indice].estado=BUTTON_UP;
}


void actuallizaMef(uint8_t indice){

    switch (ourButton[indice].estado)
    {
    case BUTTON_DOWN:
        if(botones.read() & mask[indice] )
           ourButton[indice].estado=BUTTON_RISING;
    
    break;
    case BUTTON_UP:
        if(!(botones.read() & mask[indice]))
            ourButton[indice].estado=BUTTON_FALLING;
    
    break;
    case BUTTON_FALLING:
        if(!(botones.read() & mask[indice]))
        {
            ourButton[indice].timeDown=miTimer.read_ms();
            ourButton[indice].estado=BUTTON_DOWN;
            //Flanco de bajada
        }
        else
            ourButton[indice].estado=BUTTON_UP;    

    break;
    case BUTTON_RISING:
        if(botones.read() & mask[indice]){
            ourButton[indice].estado=BUTTON_UP;
            //Flanco de Subida
            ourButton[indice].timeDiff=miTimer.read_ms()-ourButton[indice].timeDown;
           /*
            if(ourButton[indice].timeDiff >= TIMETOSTART)
                togleLed(indice);
                */
        }

        else
            ourButton[indice].estado=BUTTON_DOWN;
    
    break;
    
    default:
    startMef(indice);
        break;
    }
}

void togleLed(uint8_t indice){
   leds=mask[indice];
}