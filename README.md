<div align="center">

# 🔐 Reto 2 — Sistema de Acceso por Contraseña

### STM32F407 · Teclado matricial 4×4 · Matriz LED 8×8

*Ingreso de una clave de 4 dígitos con validación y retroalimentación visual, en C bare-metal.*

<br>

![MCU](https://img.shields.io/badge/MCU-STM32F407-03234B?style=for-the-badge&logo=stmicroelectronics&logoColor=white)
![Lenguaje](https://img.shields.io/badge/C-bare--metal-00599C?style=for-the-badge&logo=c&logoColor=white)
![IDE](https://img.shields.io/badge/IDE-STM32CubeIDE-03234B?style=for-the-badge)
![Curso](https://img.shields.io/badge/Microprocesadores-UPB-1E88E5?style=for-the-badge)

</div>

---

## 📑 Tabla de contenido

- [📖 Descripción general](#-descripción-general)
- [✨ Características](#-características)
- [🎬 ¿Cómo funciona?](#-cómo-funciona)
- [🔌 Hardware y conexiones](#-hardware-y-conexiones)
- [🧠 Arquitectura del software](#-arquitectura-del-software)
- [⏱️ Temporización (SysTick)](#️-temporización-systick)
- [🖥️ Visualización y figuras](#️-visualización-y-figuras)
- [🗺️ Diagramas](#️-diagramas)
- [📂 Estructura del repositorio](#-estructura-del-repositorio)
- [⚙️ Compilación y carga](#️-compilación-y-carga)
- [📚 Documentación](#-documentación)
- [🛠️ Herramientas](#️-herramientas)
- [👤 Autor](#-autor)

---

## 📖 Descripción general

Sistema de **acceso por contraseña** sobre **STM32F407**. El usuario ingresa una clave de cuatro dígitos mediante un **teclado matricial 4×4** y el sistema la compara contra una clave almacenada. El resultado se muestra en una **matriz de LEDs 8×8**: una **cara feliz** si la clave es correcta o una **cara triste** si es incorrecta.

Todo el programa está escrito en **lenguaje C bare-metal**, accediendo directamente a los registros de los periféricos mediante las definiciones CMSIS (`stm32f4xx.h`), y se apoya en dos **máquinas de estado finito**, **antirrebote** por software y **multiplexación** de la matriz por interrupción.

> [!NOTE]
> La clave por defecto es **`8 3 1 4`** (constante `CLAVE_CORRECTA` en el código).

---

## ✨ Características

- 🔑 **Clave de 4 dígitos** con validación automática al completar el ingreso.
- ⌨️ **Teclado 4×4** con **antirrebote por máquina de estados** (depura presión y liberación).
- 🟩 **Matriz LED 8×8** multiplexada por **SysTick** a ≈ 125 Hz (sin parpadeo).
- 😀 / ☹️ **Retroalimentación visual** con cara feliz / triste.
- 🔁 **Reinicio automático** tras 2 s, listo para un nuevo intento.
- ⭐ Tecla **`*`** para **borrar / cancelar** en cualquier momento del ingreso.
- 🧩 Arquitectura **por capas, no bloqueante y dirigida por eventos**.
- ⚙️ **C bare-metal** con acceso directo a registros (sin HAL).

---

## 🎬 ¿Cómo funciona?

```
   Ingresas un dígito (0–9)
            │
            ▼
   ┌──────────────────┐     cada dígito muestra
   │  Aparece el BOX  │◄─── un cuadro 300 ms como
   └──────────────────┘     confirmación
            │
     ¿ya van 4 dígitos?
            │ sí
            ▼
     ┌─────────────┐        ✔ correcta → 😀  (2 s)
     │  VALIDACIÓN │───────►
     └─────────────┘        ✘ incorrecta → ☹️ (2 s)
            │
            ▼
     Reinicio automático → listo para otro intento
```

1. Ingresa los **4 dígitos**; cada uno se confirma con un **cuadro** en la matriz.
2. Al cuarto dígito, el sistema **valida** la clave.
3. Muestra **😀** (acceso) o **☹️** (error) durante **2 s**.
4. La tecla **`*`** borra el ingreso y reinicia en cualquier momento.

---

## 🔌 Hardware y conexiones

### Matriz de LEDs 8×8

| Señal | Pines | Puerto | Configuración |
| :--- | :--- | :--- | :--- |
| Filas (8) | `PA0 – PA7` | GPIOA | Salida · **activa en BAJO** |
| Columnas (8) | `PE8 – PE15` | GPIOE | Salida · **activa en ALTO** |

### Teclado matricial 4×4

| Señal | Pines | Puerto | Configuración |
| :--- | :--- | :--- | :--- |
| Filas (4) | `PD0 – PD3` | GPIOD | Salida |
| Columnas (4) | `PD4 – PD7` | GPIOD | Entrada · **pull-up interno** |

### Distribución del teclado

```
┌─────┬─────┬─────┬─────┐
│  1  │  2  │  3  │  A  │
├─────┼─────┼─────┼─────┤
│  4  │  5  │  6  │  B  │
├─────┼─────┼─────┼─────┤
│  7  │  8  │  9  │  C  │
├─────┼─────┼─────┼─────┤
│  *  │  0  │  #  │  D  │
└─────┴─────┴─────┴─────┘
```

> [!TIP]
> El orden lógico de los bits **no coincide** con el cableado físico. Toda esa dependencia se aísla en dos lugares: la función `columnas()` (matriz) y la tabla `keys[][]` (teclado). Si recableas el montaje, solo ajustas esos dos.

---

## 🧠 Arquitectura del software

El diseño está organizado en **capas independientes**, cada una con una única responsabilidad:

```
Adquisición      →  Antirrebote      →  Eventos        →  Lógica de       →  Presentación
Keypad_Scan()       (FSM teclado)       KeyEvent_t         aplicación         frame[] + ISR
lee el hardware     limpia rebotes      evento limpio      Sistema_Task()     multiplexa
```

El programa usa **dos máquinas de estado finito**:

### 🔘 Máquina de estados del teclado (antirrebote)

Convierte pulsaciones ruidosas en **eventos limpios**, sin bloquear el resto del sistema:

`KEY_IDLE → KEY_DEBOUNCE_PRESS → KEY_PRESSED → KEY_DEBOUNCE_RELEASE`

Genera `KEY_PRESSED_EVENT` y `KEY_RELEASED_EVENT`, garantizando **un solo evento por pulsación**.

### 🔘 Máquina de estados de la contraseña

`ESPERA → INGRESANDO → VALIDANDO → ACCESO / ESTADO_ERROR`

| Estado | Qué hace |
| :--- | :--- |
| `ESPERA` | Espera el primer dígito. `*` limpia. |
| `INGRESANDO` | Guarda dígitos y muestra el cuadro. `*` cancela. Con 4 dígitos → valida. |
| `VALIDANDO` | Compara la clave ingresada contra `8 3 1 4`. |
| `ACCESO` | Cara feliz 😀 durante 2 s, luego reinicia. |
| `ESTADO_ERROR` | Cara triste ☹️ durante 2 s, luego reinicia. |

---

## ⏱️ Temporización (SysTick)

Una **única base de tiempo** de 1 ms rige todo el sistema:

```c
SysTick_Config(16000);   // 16 000 / 16 MHz = 1 ms por tick
```

Los tiempos se miden de forma **no bloqueante** comparando contra el contador global `ms`:

| Constante | Valor | Uso |
| :--- | :---: | :--- |
| `DEBOUNCE_MS` | 20 ms | Antirrebote del teclado |
| `BOX_MS` | 300 ms | Duración del cuadro por dígito |
| `RESULT_MS` | 2000 ms | Duración de la cara feliz / triste |
| *tick SysTick* | 1 ms | Base de tiempo + refresco de la matriz |

> Con 8 filas refrescadas a 1 ms cada una, el cuadro completo se repite cada **8 ms (≈ 125 Hz)**, muy por encima del umbral de parpadeo perceptible.

---

## 🖥️ Visualización y figuras

La matriz no puede encender los 64 LEDs a la vez, así que se **multiplexa por filas** dentro de la **interrupción de SysTick** (refresco constante e inmune a la carga del programa). La lógica solo escribe en el buffer `frame[]` mediante `mostrar()` y `limpiar()`; nunca toca los GPIO directamente.

<details>
<summary><b>🎨 Ver las tres figuras (bitmaps 8×8)</b></summary>

<br>

**Cuadro — `BOX`** (ingreso de dígitos)

```
█ █ █ █ █ █ █ █
█ · · · · · · █
█ · · · · · · █
█ · · · · · · █
█ · · · · · · █
█ · · · · · · █
█ · · · · · · █
█ █ █ █ █ █ █ █
```

**Cara feliz — `HAPPY`** (acceso concedido)

```
· █ · · · · █ ·
· █ · · · · █ ·
· █ · · · · █ ·
· · · · · · · ·
· █ · · · · █ ·
· · █ · · █ · ·
· · · █ █ · · ·
· · · · · · · ·
```

**Cara triste — `SAD`** (acceso denegado)

```
· █ · · · · █ ·
· █ · · · · █ ·
· █ · · · · █ ·
· · · · · · · ·
· · █ █ █ █ · ·
· █ · · · · █ ·
█ · · · · · · █
· · · · · · · ·
```

</details>

---

## 🗺️ Diagramas

<div align="center">

**Flujo del programa (main + interrupción SysTick)**

<img src="3.%20Diagramas/diagrama_flujo_programa.png" width="820" alt="Diagrama de flujo del programa">

<br><br>

**Máquina de estados del teclado (antirrebote)**

<img src="3.%20Diagramas/maquina_estados_teclado.png" width="820" alt="Máquina de estados del teclado">

<br><br>

**Máquina de estados de la contraseña**

<img src="3.%20Diagramas/maquina_estados_contrasena.png" width="760" alt="Máquina de estados de la contraseña">

</div>

---

## 📂 Estructura del repositorio

```
Reto2Micro/
│
├── 0. Referencias/      → Datasheets y manuales (RM0090, PM0214) y material del reto
├── 1. Codigo/reto2/     → Código fuente en C y proyecto STM32CubeIDE
├── 2. documentación/    → Estrategias de desarrollo, cálculos y evidencias
├── 3. Diagramas/        → Diagrama de flujo y máquinas de estado
├── 4. Hardware/         → Fotos del montaje y conexiones
│
└── README.md
```

---

## ⚙️ Compilación y carga

1. Clona el repositorio:
   ```bash
   git clone https://github.com/ManuSeQuiereRendir/Reto2Micro.git
   ```
2. Abre **STM32CubeIDE** e importa el proyecto desde `1. Codigo/reto2`.
3. Compila el proyecto (**Project → Build**).
4. Conecta la tarjeta por **ST-Link** y carga el binario (**Run → Debug/Run**).

> [!IMPORTANT]
> El proyecto asume el reloj interno **HSI a 16 MHz** (por eso `SysTick_Config(16000)` da 1 ms). Si cambias la frecuencia del reloj, ajusta ese valor.

---

## 📚 Documentación

En la carpeta [`2. documentación`](2.%20documentaci%C3%B3n) encontrarás:

- 📄 **Estrategias de desarrollo** — arquitectura, decisiones de diseño y compensaciones.
- ⏱️ **Cálculos de temporización** — SysTick, antirrebote, refresco.
- 🔧 **Configuración de periféricos** — GPIO y mapa de registros.

---

## 🛠️ Herramientas

- 🔩 Tarjeta basada en **STM32F407**
- 🟩 Matriz de LEDs **8×8** (tipo 1088AS)
- ⌨️ Teclado matricial **4×4**
- 💻 **STM32CubeIDE** (CMSIS · `stm32f4xx.h`)
- 📘 Documentación **RM0090** y **PM0214**

---

## 👤 Autor

**Manu** — Estudiante de Ingeniería Eléctrica-Electrónica · UPB
🔗 GitHub: [@ManuSeQuiereRendir](https://github.com/ManuSeQuiereRendir)

<div align="center">

<br>
</div>
