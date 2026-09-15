<div align="center">

# Reto 2 — Sistema de Acceso por Contraseña

**STM32F407 · Teclado matricial 4×4 · Matriz LED 8×8**

![STM32F407](https://img.shields.io/badge/STM32F407-6D28D9?style=flat-square&logo=stmicroelectronics&logoColor=white)
![C](https://img.shields.io/badge/C-bare--metal-7C3AED?style=flat-square&logo=c&logoColor=white)
![Microprocesadores](https://img.shields.io/badge/Microprocesadores-UPB-8B5CF6?style=flat-square)

</div>

---

## Descripción

Sistema de acceso por contraseña sobre STM32F407. El usuario ingresa una clave de cuatro dígitos mediante un teclado matricial 4×4 y el sistema la compara contra una clave almacenada. El resultado se muestra en una matriz de LEDs 8×8: una cara feliz si la clave es correcta o una cara triste si es incorrecta.

El programa está escrito en C bare-metal, accediendo directamente a los registros mediante CMSIS (`stm32f4xx.h`). La clave por defecto es **8314**.

## Características

- Clave de 4 dígitos con validación automática al completar el ingreso.
- Teclado 4×4 con antirrebote por máquina de estados (depura presión y liberación).
- Matriz LED 8×8 multiplexada por SysTick a ≈ 125 Hz, sin parpadeo.
- Retroalimentación visual con cara feliz / triste.
- Reinicio automático tras 2 s, listo para un nuevo intento.
- Tecla `*` para borrar o cancelar en cualquier momento del ingreso.
- Arquitectura por capas, no bloqueante y dirigida por eventos.

## Cómo funciona

1. Ingresa los 4 dígitos; cada uno se confirma con un cuadro en la matriz.
2. Al cuarto dígito, el sistema valida la clave.
3. Muestra la cara feliz (acceso) o la cara triste (error) durante 2 s.
4. La tecla `*` borra el ingreso y reinicia en cualquier momento.

## Hardware y conexiones

**Matriz de LEDs 8×8**

| Señal | Pines | Puerto | Configuración |
| :--- | :--- | :--- | :--- |
| Filas (8) | `PA0 – PA7` | GPIOA | Salida, activa en BAJO |
| Columnas (8) | `PE8 – PE15` | GPIOE | Salida, activa en ALTO |

**Teclado matricial 4×4**

| Señal | Pines | Puerto | Configuración |
| :--- | :--- | :--- | :--- |
| Filas (4) | `PD0 – PD3` | GPIOD | Salida |
| Columnas (4) | `PD4 – PD7` | GPIOD | Entrada con pull-up interno |

Distribución del teclado:

```
1  2  3  A
4  5  6  B
7  8  9  C
*  0  #  D
```

## Arquitectura del software

El diseño se organiza en capas independientes, cada una con una única responsabilidad:

```
Keypad_Scan()  →  Antirrebote (FSM)  →  Eventos  →  Sistema_Task()  →  frame[] + ISR
lee el teclado     limpia rebotes        limpios     lógica de clave    multiplexa la matriz
```

**Máquina de estados del teclado (antirrebote)**

`KEY_IDLE → KEY_DEBOUNCE_PRESS → KEY_PRESSED → KEY_DEBOUNCE_RELEASE`

Convierte pulsaciones ruidosas en eventos limpios (`KEY_PRESSED_EVENT`, `KEY_RELEASED_EVENT`), garantizando un solo evento por pulsación.

**Máquina de estados de la contraseña**

`ESPERA → INGRESANDO → VALIDANDO → ACCESO / ESTADO_ERROR`

| Estado | Qué hace |
| :--- | :--- |
| `ESPERA` | Espera el primer dígito. `*` limpia. |
| `INGRESANDO` | Guarda dígitos y muestra el cuadro. `*` cancela. Con 4 dígitos, valida. |
| `VALIDANDO` | Compara la clave ingresada contra 8314. |
| `ACCESO` | Cara feliz durante 2 s, luego reinicia. |
| `ESTADO_ERROR` | Cara triste durante 2 s, luego reinicia. |

## Temporización (SysTick)

Una única base de tiempo de 1 ms rige todo el sistema:

```c
SysTick_Config(16000);   // 16 000 / 16 MHz = 1 ms por tick
```

Los tiempos se miden de forma no bloqueante comparando contra el contador global `ms`:

| Constante | Valor | Uso |
| :--- | :---: | :--- |
| `DEBOUNCE_MS` | 20 ms | Antirrebote del teclado |
| `BOX_MS` | 300 ms | Duración del cuadro por dígito |
| `RESULT_MS` | 2000 ms | Duración de la cara feliz / triste |
| tick SysTick | 1 ms | Base de tiempo y refresco de la matriz |

Con 8 filas refrescadas a 1 ms cada una, el cuadro completo se repite cada 8 ms (≈ 125 Hz), por encima del umbral de parpadeo perceptible.

## Diagramas

<div align="center">

<img src="3.%20Diagramas/diagrama_flujo_programa.png" width="780" alt="Diagrama de flujo del programa">

<img src="3.%20Diagramas/maquina_estados_teclado.png" width="780" alt="Máquina de estados del teclado">

<img src="3.%20Diagramas/maquina_estados_contrasena.png" width="720" alt="Máquina de estados de la contraseña">

</div>

## Estructura del repositorio

```
Reto2Micro/
├── 0. Referencias/      Datasheets y manuales (RM0090, PM0214)
├── 1. Codigo/reto2/     Código fuente en C y proyecto STM32CubeIDE
├── 2. documentación/    Estrategias de desarrollo, cálculos y evidencias
├── 3. Diagramas/        Diagrama de flujo y máquinas de estado
├── 4. Hardware/         Fotos del montaje y conexiones
└── README.md
```

## Compilación y carga:

1. Clona el repositorio:
   ```bash
   git clone https://github.com/ManuSeQuiereRendir/Reto2Micro.git
   ```
2. Abre STM32CubeIDE e importa el proyecto desde `1. Codigo/reto2`.
3. Compila (Project → Build).
4. Conecta la tarjeta por ST-Link y carga el binario (Run).

> El proyecto asume el reloj interno HSI a 16 MHz (por eso `SysTick_Config(16000)` da 1 ms). Si cambias la frecuencia del reloj, ajusta ese valor.

## Documentación: 

En la carpeta `2. documentación` se encuentran las estrategias de desarrollo.

---

## Autora: 

**Manuela Sánchez Quintero** — Estudiante de Ingeniería Eléctrica-Electrónica · UPB
🔗 GitHub: [@ManuSeQuiereRendir](https://github.com/ManuSeQuiereRendir)

<div align="center">

<br>
</div>
