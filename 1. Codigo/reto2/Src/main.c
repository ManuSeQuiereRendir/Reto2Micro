#ifndef STM32F407xx
#define STM32F407xx
#endif

#include "stm32f4xx.h"
#include <stdint.h>

//CONFIGURACIÓN-----------------------------------
#define DEBOUNCE_MS 20
#define BOX_MS      300
#define RESULT_MS   2000

//VARIABLES--------------------------------------
volatile uint32_t ms = 0;
volatile uint8_t frame[8] = {0};

const char CLAVE_CORRECTA[4] = {'8','3','1','4'};

char entrada[4];
uint8_t n = 0;

//COLUMNAS DE LA MATRIZ----------------------------------------
	//filas estan en orden columnas se organizan a continuación:

uint8_t columnas(uint8_t dato)
{
    uint8_t r = 0;

    if(dato & 0x01) r |= 0x10;
    if(dato & 0x02) r |= 0x08;
    if(dato & 0x04) r |= 0x40;
    if(dato & 0x08) r |= 0x01;
    if(dato & 0x10) r |= 0x20;
    if(dato & 0x20) r |= 0x02;
    if(dato & 0x40) r |= 0x80;
    if(dato & 0x80) r |= 0x04;

    return r;
}

//MULTIPLEXACIÓN---------------------------------

void SysTick_Handler(void)
{
    static uint8_t fila = 0;

    ms++;   //tyme increase

    //matriz inicia apagada
    GPIOA->BSRR = 0x000000FF;
    GPIOE->BSRR = 0xFF000000;

    //Activación columna
    GPIOE->BSRR =
        ((uint32_t)columnas(frame[fila])) << 8;

    //Activación fila
    GPIOA->BSRR =
        1u << (fila + 16);

    fila = (fila + 1) & 7;
}

//FIGURAS---------------------------------------------
void limpiar(void)
{
    for(int i = 0; i < 8; i++)
        frame[i] = 0;
}

void mostrar(const uint8_t dibujo[8])
{
    for(int i = 0; i < 8; i++)
        frame[i] = dibujo[i];
}

//cuadro para el ingreso de numeros
const uint8_t BOX[8] =
{
    0xFF,
    0x81,
    0x81,
    0x81,
    0x81,
    0x81,
    0x81,
    0xFF
};

// Carita triste para cuando esta malo
const uint8_t SAD[8] =
{
    0x42,
    0x42,
    0x42,
    0x00,
    0x3C,
    0x42,
    0x81,
    0x00
};

// carita feliz para cuando esta bueno

const uint8_t HAPPY[8] =
{
    0x42,
    0x42,
    0x42,
    0x00,
    0x42,
    0x24,
    0x18,
    0x00
};

//configuración del teclado

void Keypad_Init(void)
{
	//filas (outs)
    RCC->AHB1ENR |= (1 << 3);

    /* PD0-PD3 = salidas */
    GPIOD->MODER &= ~0x0000FFFF;
    GPIOD->MODER |=  0x00000055;

    //columnas (ins)
    /* PD4-PD7 = entradas */
    GPIOD->MODER &= ~0x0000FF00;

    /* Pull-up en PD4-PD7 */
    GPIOD->PUPDR &= ~0x0000FF00;
    GPIOD->PUPDR |=  0x00005500;

    /* Filas HIGH */
    GPIOD->BSRR = 0x0000000F;
}

char Keypad_Scan(void)
{
    const char keys[4][4] =
    {
    		//compensación para que si coincida con el tecclado
        {'1','4','7','A'},
        {'2','5','8','B'},
        {'3','6','9','C'},
        {'*','0','#','D'}
    };

    for(int fila = 0; fila < 4; fila++)
    {
        /* Todas las filas HIGH */
        GPIOD->BSRR = 0x0000000F;

        /* Fila actual LOW */
        GPIOD->BSRR = 1u << (fila + 16);

        /* Estabilizacion */
        for(volatile int i = 0; i < 50; i++);

        uint16_t cols =
            (GPIOD->IDR >> 4) & 0x0F;

        if(!(cols & 0x01))
            return keys[fila][0];

        if(!(cols & 0x02))
            return keys[fila][1];

        if(!(cols & 0x04))
            return keys[fila][2];

        if(!(cols & 0x08))
            return keys[fila][3];
    }

    return 0;
}

// MAQUINA DE ESTAODS DEL DEBOUNCE----------------------------------
typedef enum
{
    KEY_IDLE,         // espera que se presione una tecla
    KEY_DEBOUNCE_PRESS, //espera 20 milisegundos y si sigue presionada la tecla la toma como presionada
    KEY_PRESSED,        // acepta la presión y genera el "evento"
    KEY_DEBOUNCE_RELEASE //confirma que la tecla se haya soltado

} KeyState_t;

KeyState_t key_state = KEY_IDLE;

char tecla_guardada = 0;
uint32_t debounce_timer = 0;

typedef enum
{
    KEY_NONE,
    KEY_PRESSED_EVENT,
    KEY_RELEASED_EVENT

} KeyEventType_t;

typedef struct
{
    KeyEventType_t type;
    char key;

} KeyEvent_t;

/* =========================================================
   TAREA DEL TECLADO
   ========================================================= */

KeyEvent_t Keypad_Task(void)
{
    KeyEvent_t evento =
    {
        KEY_NONE,
        0
    };

    char actual = Keypad_Scan();

    switch(key_state)
    {
        case KEY_IDLE:

            if(actual != 0)
            {
                tecla_guardada = actual;
                debounce_timer = ms;
                key_state = KEY_DEBOUNCE_PRESS;
            }

            break;

        case KEY_DEBOUNCE_PRESS:

            if(actual == 0)
            {
                key_state = KEY_IDLE;
            }
            else if((ms - debounce_timer) >= DEBOUNCE_MS)
            {
                evento.type = KEY_PRESSED_EVENT;
                evento.key = tecla_guardada;

                key_state = KEY_PRESSED;
            }

            break;

        case KEY_PRESSED:

            if(actual == 0)
            {
                debounce_timer = ms;
                key_state = KEY_DEBOUNCE_RELEASE;
            }

            break;

        case KEY_DEBOUNCE_RELEASE:

            if(actual != 0)
            {
                key_state = KEY_PRESSED;
            }
            else if((ms - debounce_timer) >= DEBOUNCE_MS)
            {
                evento.type = KEY_RELEASED_EVENT;
                evento.key = tecla_guardada;

                key_state = KEY_IDLE;
            }

            break;
    }

    return evento;
}

//maquina de estados de la contraseña

typedef enum
{
    ESPERA,   //espera a que entre algún #
    INGRESANDO, //va guardando los datos y me muestra el cuadrado
    VALIDANDO,  //revisa si la contraseña es correcta (entra a acceso) o incorrecta (entra a estado_error)
    ACCESO,   //muestra la carita feliz 2 secs limpia el n para reiniciar
    ESTADO_ERROR //muestra la carita triste 2 secs y limpia el n para reiniciar

} SistemaState_t;

SistemaState_t sistema = ESPERA;

uint32_t box_timer = 0;
uint32_t resultado_timer = 0;

/* =========================================================
   LOGICA DE CONTRASEÑA
   ========================================================= */

void Sistema_Task(KeyEvent_t evento)
{
    switch(sistema)
    {
        //espera

        case ESPERA:

            if(evento.type == KEY_PRESSED_EVENT)
            {
                if(evento.key >= '0' &&
                   evento.key <= '9')           //revisa que si sea un número
                {
                    entrada[0] = evento.key;
                    n = 1;

                    mostrar(BOX);
                    box_timer = ms;

                    sistema = INGRESANDO;
                }
                else if(evento.key == '*')
                {
                    n = 0;
                    limpiar();
                }
            }

            break;


        //ingresando

        case INGRESANDO:

            /* Apagar cuadro despues de 300 ms */
            if((ms - box_timer) >= BOX_MS)
            {
                limpiar();    //lo que me ayuda a ver el cuadro con cada ingreso
            }

            if(evento.type == KEY_PRESSED_EVENT)
            {
                /* BORRAR */
                if(evento.key == '*')
                {
                    n = 0;
                    limpiar();
                    sistema = ESPERA;
                }

                /* NUMERO */
                else if(evento.key >= '0' &&
                        evento.key <= '9')
                {
                    if(n < 4)
                    {
                        entrada[n] = evento.key;
                        n++;

                        mostrar(BOX);
                        box_timer = ms;
                    }

                    /* Validar solamente con 4 numeros */
                    if(n == 4)
                    {
                        sistema = VALIDANDO;
                    }
                }
            }

            break;


        //validando

        case VALIDANDO:

            if((ms - box_timer) >= BOX_MS)
            {
                uint8_t correcto = 1;

                for(int i = 0; i < 4; i++)
                {
                    if(entrada[i] != CLAVE_CORRECTA[i])
                    {
                        correcto = 0;
                    }
                }

                if(correcto)
                {
                    mostrar(HAPPY);
                    sistema = ACCESO;
                }
                else
                {
                    mostrar(SAD);
                    sistema = ESTADO_ERROR;
                }

                resultado_timer = ms;
            }

            break;


        //acceso directo

        case ACCESO:

            if((ms - resultado_timer) >= RESULT_MS)
            {
                limpiar();

                n = 0;
                sistema = ESPERA;
            }

            break;

            //estado error
        case ESTADO_ERROR:

            if((ms - resultado_timer) >= RESULT_MS)
            {
                limpiar();

                n = 0;
                sistema = ESPERA;
            }

            break;
    }
}

// MAIN ----------------------------------------------

int main(void)
{
    /* GPIOA, GPIOD, GPIOE */
    RCC->AHB1ENR |=
        (1 << 0) |
        (1 << 3) |
        (1 << 4);

    // Mascaras de la matriz

    /* PA0-PA7 = filas */
    GPIOA->MODER =
        (GPIOA->MODER & 0xFFFF0000) |
        0x00005555;

    GPIOA->BSRR = 0x000000FF;

    /* PE8-PE15 = columnas */
    GPIOE->MODER =
        (GPIOE->MODER & 0x0000FFFF) |
        0x55550000;

    GPIOE->BSRR = 0xFF000000;

    limpiar();
    Keypad_Init();
    SysTick_Config(16000);

    //Loop-----------------------------------

    while(1)
    {
        KeyEvent_t evento = Keypad_Task();

        Sistema_Task(evento);
    }
}
