<div align="center">

# cubiboot-new-ui

**Un reemplazo del IPL de GameCube que arranca tus juegos desde una cuadrícula de banners.**

<img width="320" height="240" alt="menú de cubiboot" src="https://github.com/user-attachments/assets/eb1d6fc9-f0eb-4a38-8f93-20daa4a0af19" />

Un fork de [makeo/cubiboot](https://github.com/makeo/cubiboot) — que a su vez es un fork de
[cubeboot](https://github.com/OffBroadway/cubeboot) de [TeamOffBroadway](https://github.com/OffBroadway) —
con soporte para SD2SP2, SD Gecko y adaptadores SD similares.

[English](../README.md) · **Español**

</div>

---

## Contenido

- [Novedades de este fork](#novedades-de-este-fork)
- [Antes de empezar](#antes-de-empezar)
- [Descargas](#descargas)
- [Instalación](#instalación)
  - [Método 1: PicoBoot o PicoLoader con gekkoboot](#método-1-picoboot-o-picoloader-con-gekkoboot)
  - [Método 2: PicoLoader con cubiboot integrado](#método-2-picoloader-con-cubiboot-integrado)
  - [Método 3: GC Loader y otros ODE](#método-3-gc-loader-y-otros-ode)
  - [Reinicio en el juego](#reinicio-en-el-juego)
- [Configuración](#configuración)
  - [Todas las opciones](#todas-las-opciones)
  - [Diseño del menú](#diseño-del-menú)
  - [Carpeta de inicio](#carpeta-de-inicio)
  - [Recordar el último jugado](#recordar-el-último-jugado)
  - [Apps homebrew](#apps-homebrew)
  - [De dónde se leen los juegos](#de-dónde-se-leen-los-juegos)
  - [Iniciar Swiss desde el menú](#iniciar-swiss-desde-el-menú)
  - [Colores](#colores)
- [Carpetas grandes y el pool de banners](#carpetas-grandes-y-el-pool-de-banners)
- [Limitaciones conocidas](#limitaciones-conocidas)
- [Compilación](#compilación)
- [Créditos](#créditos)

---

## Novedades de este fork

Lo que este fork añade sobre [makeo/cubiboot](https://github.com/makeo/cubiboot):

| | |
|---|---|
| **Menú en cuadrícula con banners** | Portado desde cubeboot. Tres diseños, seleccionables con [`menu_grid_type`](#diseño-del-menú); usa `small_banners` por defecto incluso sin `config.ini`. |
| **Nombres de archivo reales** | La lista muestra el **nombre del archivo** `.iso` en lugar del nombre interno del juego, y carga el banner correcto de cada disco en juegos multidisco (por ejemplo Resident Evil 0 Disco 1 / Disco 2). |
| **Apps homebrew con banner** | Una carpeta con `default.dol` junto a `opening.bnr` aparece como una aplicación lanzable con su propio banner, en vez de una carpeta que hay que abrir. Mira [Apps homebrew](#apps-homebrew). |
| **Recordar el último jugado** | [`remember_last_game = 1`](#recordar-el-último-jugado) abre el menú en la carpeta de tu último juego, ya resaltado — pulsas **A** y listo. |
| **Juegos desde la SD del ODE** | [`device_order`](#de-dónde-se-leen-los-juegos) puede apuntar cubiboot a la tarjeta SD que está dentro de un ODE tipo GC Loader, así el menú lista lo que ya hay en ella sin un segundo lector. |
| **Arreglo de banners en arranque en frío** | Los pools de banners viven en memoria baja que PicoBoot no limpia en arranque en frío, así que flags de "en uso" obsoletos solapaban búferes (corrupción) o los dejaban sin ninguno (en blanco) — peor cuanto más fría la consola. Ahora los pools se ponen a cero al inicio y los banners quedan residentes en MRAM. |
| **Nombre de la carpeta en el encabezado** | El encabezado del menú nombra la carpeta que estás navegando; en la raíz de la tarjeta dice "CUBIBOOT New UI". |
| **Marca Cubiboot** | El banner de Cubiboot en el loader y en la intro de la BIOS del `.iso`, reemplazando el "Game Play" de gc-linux. |
| **Releases automatizadas** | La CI recompila `apploader.img` (para que el reinicio en el juego vuelva a *esta* versión del loader, no a una vieja) y un `cubiboot_picoloader.uf2` flasheable. |

Lista completa de cambios frente al upstream: [docs/FORK_CHANGES.md](FORK_CHANGES.md).
Cómo encaja todo (en inglés): [docs/ARCHITECTURE.md](ARCHITECTURE.md).

## Antes de empezar

> [!IMPORTANT]
> - Formatea la tarjeta SD como **exFAT**, no FAT32. En FAT32 la carga es muy lenta.
> - Mantén los nombres de `.iso` y `.dol` por debajo de **28 caracteres**, o se recortan en la lista.
> - Ni `ipl.dol` ni `cubiboot.iso` funcionan en **Dolphin Emulator**, ni siquiera configurando un IPL.bin.

También necesitas [Swiss](https://github.com/emukidid/swiss-gc/releases/latest): cubiboot lo
encadena para arrancar los juegos. Renombra su `.dol` a **`swiss-gc.dol`** y ponlo en la
**raíz** de la tarjeta que cubiboot va a leer.

Estos archivos siempre van en la **raíz** de esa tarjeta:

```
/ipl.dol                        (solo para el método de instalación 1)
/config.ini
/swiss-gc.dol
/swiss/patches/apploader.img    (solo si quieres reinicio en el juego)
```

Tus juegos pueden estar donde quieras, incluso en subcarpetas — mira
[`default_folder`](#carpeta-de-inicio).

## Descargas

Cada release etiquetada (`v*`) publica:

| Archivo | Qué es |
|---------|--------|
| **`EXTRACT_TO_ROOT.zip`** | Todo lo que va en la tarjeta SD (`ipl.dol`, `config.ini`, `swiss/patches/apploader.img`). Extráelo en la raíz de la tarjeta — el punto de partida más fácil. |
| `ipl.dol` | El loader cubiboot (un reemplazo del IPL de GameCube). Se arranca vía PicoBoot/PicoLoader + gekkoboot. |
| `cubiboot_picoloader.uf2` | Firmware de PicoLoader con cubiboot **integrado** — flashéalo en la RP2040 Pico; no hace falta ningún archivo del loader en la tarjeta. |
| `cubiboot.iso` | Imagen de disco GameCube arrancable para **GC Loader** y otros ODE, con la marca Cubiboot. |
| `apploader.img` | El redirector de **reinicio en el juego** de Swiss. Incrusta el loader de *esta* compilación, así la combinación de reinicio vuelve a este menú. Va en `SD:/swiss/patches/`. |
| `config.ini` | Configuración de ejemplo mínima (`menu_grid_type = small_banners`). Va en la raíz de la tarjeta. |

[**→ Última release**](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest)

## Instalación

Elige el que corresponda a tu consola:

| Tu equipo | Usa |
|---|---|
| Modchip PicoBoot o PicoLoader | [Método 1](#método-1-picoboot-o-picoloader-con-gekkoboot) — recomendado, se actualiza cambiando un archivo en la SD |
| PicoLoader, y no quieres archivos del loader en la tarjeta | [Método 2](#método-2-picoloader-con-cubiboot-integrado) |
| GC Loader u otro ODE, sin modchip | [Método 3](#método-3-gc-loader-y-otros-ode) |

### Método 1: PicoBoot o PicoLoader con gekkoboot

**Recomendado.** Actualizar cubiboot después es solo reemplazar un archivo en la tarjeta SD
— sin desarmar nada.

1. Flashea tu Pico con el `.uf2` de [PicoBoot](https://github.com/webhdx/PicoBoot) o
   [PicoLoader](https://github.com/makeo/PicoLoader).
2. Descarga [`ipl.dol`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/ipl.dol)
   y cópialo a la **raíz** de tu tarjeta SD.
3. Pon [Swiss](https://github.com/emukidid/swiss-gc/releases/latest) en la tarjeta como
   `swiss-gc.dol`, más un [`config.ini`](#configuración) y tus juegos.

### Método 2: PicoLoader con cubiboot integrado

Cubiboot vive en el firmware de la Pico, así que en la tarjeta solo hacen falta los juegos y
`swiss-gc.dol`.

1. Flashea tu Pico con el `.uf2` de [PicoLoader](https://github.com/makeo/PicoLoader).
2. Descarga [`cubiboot_picoloader.uf2`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/cubiboot_picoloader.uf2).
3. Mantén pulsado el botón de la RP2040 Pico mientras la conectas al PC.
4. Copia el `.uf2` a la unidad USB que aparece; la Pico se reinicia ejecutando cubiboot.
5. Pon Swiss en tu tarjeta SD2SP2 / SD Gecko como `swiss-gc.dol`, junto con un
   [`config.ini`](#configuración) y tus juegos.

> [!WARNING]
> Con este método, cada actualización de cubiboot implica abrir la consola y volver a
> flashear la Pico. El método 1 es más cómodo de mantener.

### Método 3: GC Loader y otros ODE

`cubiboot.iso` es una imagen de disco GameCube arrancable que sencillamente *es* el loader
cubiboot — sin modchip.

1. Descarga [`cubiboot.iso`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/cubiboot.iso)
   y cópialo al almacenamiento de tu [GC Loader](https://gcloaderhq.com/), en la carpeta
   desde la que arrancas imágenes.
2. Arranca `cubiboot.iso` desde el menú del GC Loader — cae directo en el menú de cubiboot.
3. Elige de dónde salen los juegos:
   - **De la propia tarjeta SD del ODE** (sin segundo lector): pon `swiss-gc.dol` y un
     `config.ini` con `device_order = gcldr` en la **raíz de esa misma tarjeta**. Mira
     [De dónde se leen los juegos](#de-dónde-se-leen-los-juegos).
   - **De un adaptador SD** (SD2SP2 / SD Gecko): no hay nada que configurar — los lectores de
     tarjeta van primero por defecto. Prepara la tarjeta del adaptador como siempre.

### Reinicio en el juego

Opcional, funciona con todos los métodos anteriores.

1. Descarga [`EXTRACT_TO_ROOT.zip`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/EXTRACT_TO_ROOT.zip).
2. Extráelo en la **raíz** de la tarjeta SD — esto deja `apploader.img` dentro de
   `swiss/patches/`.
3. Pulsa **Z + A + START** dentro de un juego para volver al menú de cubiboot.

## Configuración

Pon un `config.ini` en la raíz de la tarjeta que cubiboot lee. Es opcional: sin él obtienes
el diseño `small_banners` y la raíz de la tarjeta como carpeta de inicio.

Las releases incluyen una plantilla comentada con todas las opciones — es
[`.ci/config.ini`](../.ci/config.ini) en este repositorio, y `config.ini` en el zip de la
release. El bloque de abajo es la versión corta.

```ini
[cubeboot]

; Diseño de la cuadrícula del menú de selección:
;   small_banners = banners pequeños, 4 columnas  (por defecto)
;   banners       = banners grandes, 3 columnas
;   square_icons  = iconos cuadrados, 8 columnas
menu_grid_type = small_banners

; Un solo color para toda la interfaz -- logo de arranque, cubos del menú, la caja de info
; al pie de la lista de juegos y el PRESS START grande (hex, ejemplo naranja):
; theme_color = ff9801

; Color solo del logo del cubo en el arranque (hex, tiene prioridad sobre theme_color):
; cube_color = ff9801

; Carpeta en la que se abre el menú al arrancar. Coméntala para la raíz de la tarjeta.
; default_folder = /games

; Preseleccionar el último juego arrancado al abrir el menú (1 = sí, 0 = no).
remember_last_game = 0

; De qué almacenamiento leer los juegos, el preferido primero: sd2sp2, slot_b, slot_a,
; ode. Los nombres de volumen de FatFs (sdc, sdb, sda, gcldr) también funcionan.
; Deja comentado para usar el valor por defecto.
; device_order = sd2sp2, slot_b, slot_a, ode
```

### Todas las opciones

| Clave | Valores | Por defecto | Qué hace |
|-------|---------|-------------|----------|
| [`menu_grid_type`](#diseño-del-menú) | `small_banners` · `banners` · `square_icons` | `small_banners` | Diseño de la cuadrícula |
| [`default_folder`](#carpeta-de-inicio) | ruta | raíz de la tarjeta | Carpeta en la que abre el menú |
| [`remember_last_game`](#recordar-el-último-jugado) | `1` · `0` | `0` | Preselecciona el último juego arrancado |
| [`device_order`](#de-dónde-se-leen-los-juegos) | nombres de dispositivo | `sd2sp2, slot_b, slot_a, ode` | De qué almacenamiento leer los juegos |
| [`theme_color`](#colores) | RGB hex · `random` | original | Un color para toda la interfaz |
| [`cube_color`](#colores) | RGB hex · `random` | `theme_color` | Color del logo de arranque |
| [`menu_cube_color`](#colores) | RGB hex · `random` · nombre de paleta | `theme_color` | Cubos / banners de la cuadrícula |
| [`menu_box_color`](#colores) | RGB hex · `random` | `theme_color` | Panel de info bajo la lista de juegos |
| [`menu_start_color`](#colores) | RGB hex · `random` | `theme_color` | El "PRESS START" grande de bloques |
| `force_progressive` | `1` · `0` | `0` | Fuerza el escaneo progresivo |

Otras claves heredadas del upstream se leen en
[`cubeboot/source/settings.c`](../cubeboot/source/settings.c); mira también
[docs/settings.md](settings.md). Ten en cuenta que `cube_logo` y `button_*` **no funcionan**
(mira [Limitaciones conocidas](#limitaciones-conocidas)).

### Diseño del menú

| Valor | Diseño |
|-------|--------|
| `small_banners` | banners pequeños, 4 columnas (**por defecto**) |
| `banners` | banners grandes, 3 columnas |
| `square_icons` | iconos cuadrados, 8 columnas |

`square_icons` es el que conviene para carpetas muy grandes — mira
[Carpetas grandes y el pool de banners](#carpetas-grandes-y-el-pool-de-banners).

### Carpeta de inicio

`default_folder` define el directorio en el que abre el menú. Déjala sin definir (o
comentada) para abrir en la raíz de la tarjeta. Se añade una `/` inicial automáticamente si
la omites, y si la carpeta no se puede abrir cubiboot vuelve a la raíz.

> [!NOTE]
> `default_folder` solo cambia dónde busca el menú **juegos y homebrew**
> (`.dol` / `.dol.gz` / `.iso` / …). Los archivos de sistema siguen teniendo que estar en la
> raíz de la tarjeta: `ipl.dol`, `config.ini`, `swiss-gc.dol` y `swiss/patches/apploader.img`.

### Recordar el último jugado

`remember_last_game = 1` hace que el menú abra **en la carpeta del último juego que
arrancaste**, con ese juego ya resaltado — así en el siguiente arranque solo pulsas **A**.
Desactivado por defecto.

> [!IMPORTANT]
> Esto lee la lista de recientes de Swiss, así que Swiss tiene que estar manteniéndola. En
> Swiss, abre **Settings** y pon **Recent List** en **On** (escribe `RecentListLevel=On` en
> `/swiss/settings/global.ini`). Si está en **Off** no hay ningún `recent.ini` que leer y
> cubiboot recae en [`default_folder`](#carpeta-de-inicio).

<details>
<summary><b>Cómo funciona, y cómo interactúa con <code>default_folder</code></b></summary>

<br/>

- Cubiboot arranca los juegos encadenando **Swiss** con autoload, así que Swiss registra cada
  lanzamiento en su propia lista de recientes (`/swiss/settings/recent.ini`). Cubiboot solo
  **lee** esa lista — no hay ningún archivo extra que escribir.
- En el siguiente arranque en frío el menú abre directamente en la carpeta que contiene el
  juego más reciente — incluida una subcarpeta por letra o género, no solo `default_folder` —
  y lo resalta. Navega normalmente (**B** sube un nivel).
- **Sin bloqueos:** para esa primera carpeta cubiboot *no* espera a todos los banners antes de
  mostrar la lista. Escanea la carpeta (rápido — solo cabeceras), pone el cursor sobre tu
  último juego y un **hilo en segundo plano** rellena los banners por prioridad: primero la
  ventana visible alrededor de tu juego, para que aparezca casi al instante sin importar el
  tamaño de la carpeta, y luego el resto mientras el menú ya se puede usar. Pulsar **A**
  funciona aunque los banners aún se estén cargando.

**`remember_last_game` tiene prioridad sobre `default_folder`.** Con esta opción activada el
menú **siempre** abre en la carpeta del último juego. `default_folder` (o la raíz de la
tarjeta, si no está definida) solo se usa como respaldo: en el primer arranque antes de haber
jugado a nada, o si la carpeta del último juego ya no existe.

**Carpetas grandes:** si la carpeta del último juego tiene más juegos de los que caben en el
pool de banners (>128), pasa a la ventana deslizante — los banners se leen de la tarjeta a
medida que aparecen. En cualquier caso tu juego resaltado se muestra primero. Mira
[Carpetas grandes y el pool de banners](#carpetas-grandes-y-el-pool-de-banners).

</details>

### De dónde se leen los juegos

`device_order` lista el almacenamiento que cubiboot debe usar, el preferido primero. La
primera entrada que monte se convierte en el volumen del que sale todo: el volcado del IPL,
`swiss-gc.dol`, los banners y los juegos que el menú lista.

| Nombre | Dónde está |
|--------|------------|
| `sd2sp2` (o `sdc`) | Puerto serie 2 — un **SD2SP2** |
| `slot_b` (o `sdb`) | **Ranura B** de memory card — un SD Gecko |
| `slot_a` (o `sda`) | **Ranura A** de memory card — un SD Gecko |
| `ode`, `gcloader` (o `gcldr`) | La tarjeta SD **dentro del ODE** — un [GC Loader](https://gcloaderhq.com/) o cualquiera que responda a los mismos comandos de unidad |

El valor por defecto, cuando la clave no está:

```ini
device_order = sd2sp2, slot_b, slot_a, ode
```

Dejar un dispositivo fuera de la lista es como mantienes a cubiboot lejos de él — no hay un
interruptor de encendido/apagado aparte. Así que una consola con SD2SP2 y GC Loader, cuyos
juegos viven en el ODE, escribe:

```ini
device_order = ode
```

Dos cosas a tener en cuenta:

- **Un volumen a la vez.** cubiboot no mezcla tarjetas. La entrada que gane lo tiene todo:
  `swiss-gc.dol`, los juegos y el `ipl.bin` si usas uno.
- **El `config.ini` puede estar en cualquiera de ellas.** cubiboot lo busca en cada
  dispositivo que pueda montar, en el orden por defecto de arriba, y lee el primero que
  realmente tenga el archivo — tiene que hacerlo así, porque `device_order` vive *dentro* de
  ese archivo. Si dos tarjetas llevan un `config.ini`, el orden por defecto desempata.

La tarjeta del ODE se monta en solo lectura. Una consola sin ODE paga una consulta a la unidad
por arranque por la entrada `gcldr`, que se rinde en cuanto responde una unidad óptica real.

### Apps homebrew

Una carpeta que tiene **`default.dol`** y **`opening.bnr`** juntos se trata como una
aplicación, no como una carpeta. Aparece en la cuadrícula con el banner de su `opening.bnr`,
y pulsar **A** ejecuta el `.dol` directamente en vez de abrir la carpeta.

```
/apps/
  mi-app/
    default.dol     <- lo que se lanza
    opening.bnr     <- nombre, descripción y arte del banner
  otra-app/
    default.dol
    opening.bnr
```

Los dos nombres de archivo son fijos. El banner usa el mismo formato que los discos
originales, así que el título, la descripción y la imagen de 96x32 salen todos de ese archivo.

Una carpeta a la que le falte cualquiera de los dos se comporta como siempre — entras y
navegas. La comprobación cuesta un sondeo de archivo por carpeta mientras se construye la
lista, y las carpetas sin `opening.bnr` se detienen ahí, así que una biblioteca de carpetas de
juegos no se ve afectada.

**Crear el banner.** [`tools/banner-converter/run.py`](../tools/banner-converter) convierte
cualquier imagen en un `opening.bnr`. Descárgalo de este repositorio, pon tu arte junto a él y
ejecuta:

```sh
pip install Pillow
python run.py
```

Elige la opción **2**, responde a las preguntas de título/autor/descripción, y escribe
`output/<nombre>/opening.bnr`. Deja tu `default.dol` junto a ese archivo y la carpeta está
lista para la tarjeta. Mira [su README](../tools/banner-converter) para las reglas de tamaño:
el hueco es de 96×32, así que un logotipo que se ve delgado necesita una fuente más alta, no
más ancha.

### Iniciar Swiss desde el menú

Deja el `.dol` *o* el `.iso` de Swiss en cualquier carpeta con un nombre que empiece por
`swiss` (por ejemplo `swiss-gc.dol`, `Swiss v0.6r2073.iso`). Cubiboot arranca una imagen
llamada `swiss…` **directamente a través de su propio apploader** en lugar de pasársela a
Swiss — sin ese prefijo, una imagen de disco de Swiss simplemente reinicia al IPL original.

Esto es independiente del motor `swiss-gc.dol` que debe estar en la **raíz** de la tarjeta.

### Colores

Todas las claves de color aceptan un código RGB hexadecimal
([selector de color](https://www.w3schools.com/colors/colors_hexadecimal.asp)) o `random`.
`theme_color` es el atajo; el resto son ajustes por elemento que tienen prioridad sobre él.

```ini
theme_color = ff9801   ; naranja en todo
```

| Clave | Qué pinta |
|-------|-----------|
| `theme_color` | Todo lo de abajo, salvo lo que se defina explícitamente |
| `cube_color` | Solo el cubo del logo de arranque |
| `menu_cube_color` | Los cubos / banners de la cuadrícula de juegos |
| `menu_box_color` | El panel de info bajo la lista (nombre, descripción, miniatura) |
| `menu_start_color` | El "PRESS START" grande de la pantalla previa al arranque |

Sin ninguna clave de color obtienes el aspecto original, intacto.

**Los cubos conservan su sombreado.** El IPL trae cuatro tonos por cubo — brillante, atenuado
y una variante seleccionada de cada uno — y `menu_cube_color` mueve todo ese conjunto a tu
color en vez de aplanarlo, así que el cubo seleccionado sigue destacando. También puedes
nombrar una de las seis paletas que el IPL ya tiene, usando los tonos de Nintendo tal cual:

```ini
menu_cube_color = green   ; blue | green | yellow | orange | red | purple (por defecto)
```

Nombrar una paleta *y* definir `theme_color` elige esa paleta y luego la tiñe.

**El panel de info es un degradado, a partir de un solo color.** Es más oscuro abajo y se
aclara hacia arriba hasta el color que elijas, así que `menu_box_color` es ese extremo claro
y el extremo oscuro es tu color al ~20% de luminosidad. El tono y la saturación no cambian,
así que eliges un color y el sombreado se resuelve solo. El extremo oscuro baja mucho más que
el del panel original, porque el original también gira su tono de morado a magenta; aplicar
ese giro a un color arbitrario convierte el degradado en un choque, así que la luminosidad
carga sola con el efecto y tiene que esforzarse más.

**El "PRESS START" grande** lo dibuja el BIOS original, que no ofrece ningún parámetro de
color, así que cubiboot recolorea la paleta de bloques que este lee. Solo se toca el RGB — la
intensidad por bloque que impulsa la entrada y el fundido se deja intacta, así que la
animación no cambia. La línea pequeña `Press START to begin!` de arriba es un dibujo aparte y
siempre queda en blanco.

> [!NOTE]
> Esta opción depende de la revisión del IPL, y están cubiertas las siete revisiones en las
> que cubiboot puede arrancar: NTSC 1.0-001, 1.1, 1.2-001, 1.2-101, PAL 1.0-001, PAL 1.2-101 y
> MPAL. Los demás IPL que el menú reconoce son BIOS NPDP / de kits de desarrollo, que el
> loader se niega a arrancar de todos modos.

> [!NOTE]
> `000000` funciona como negro real, y `random` elige un color nuevo en cada arranque.

## Carpetas grandes y el pool de banners

Los banners viven en un pool de memoria baja fijo, limitado a **128 imágenes** (se descartó
el streaming por ARAM — corrompía los banners al cargar). Ese límite define dos modos:

| Archivos en la carpeta | Comportamiento |
|---|---|
| **≤ 128** | Todos los banners quedan residentes. El desplazamiento es instantáneo. El mejor caso para los diseños con banners. |
| **> 128** | El pool se llena y pasa a una **ventana deslizante bajo demanda** — los banners fuera de pantalla se liberan y se releen de la tarjeta al desplazarte. Los nombres siguen apareciendo al instante; solo las imágenes se cargan bajo demanda. |

> [!WARNING]
> En modo bajo demanda el diseño con banners se vuelve lento y **puede fallar** al
> desplazarse. Para una lista muy grande en una sola carpeta, cambia a
> `menu_grid_type = square_icons`. Los nombres de archivo, el último jugado y la carpeta de
> inicio siguen funcionando en ese diseño.

**Consejos**

- Mantén las carpetas por debajo de 128 archivos para un desplazamiento instantáneo.
- Divide bibliotecas grandes en subcarpetas (género, favoritos, próximos a jugar).
- Para una biblioteca grande que quieras mantener en una sola carpeta, usa el diseño de cubos.

El límite de 128 es una salvaguarda. Se puede subir en el código, pero arriesga errores de
falta de memoria.

## Limitaciones conocidas

- La carga de archivos es lenta en FAT32 — usa **exFAT**.
- Ni `ipl.dol` ni `cubiboot.iso` funcionan en **Dolphin**, ni con un IPL.bin configurado.
- Heredado del upstream: `cube_logo` y `button_*` no funcionan (usa gekkoboot para programas
  asociados a botones).
- Los diseños con banners pueden fallar en carpetas de más de 128 archivos — mira
  [Carpetas grandes y el pool de banners](#carpetas-grandes-y-el-pool-de-banners).

## Compilación

### CI (recomendado)

Cada push compila `ipl.dol` + `apploader.img` + `cubiboot.iso` + `config.ini` +
`cubiboot_picoloader.uf2` y los sube como artefactos. Empujar una etiqueta `v*` publica una
Release de GitHub con esos archivos más `EXTRACT_TO_ROOT.zip`. Mira
[.github/workflows/ci.yml](../.github/workflows/ci.yml).

### Local

La compilación corre en una imagen Docker reproducible (devkitPPC + libogc2/libfat fijados,
solo GameCube) definida en [.ci/Dockerfile](../.ci/Dockerfile):

```sh
docker build -t cubiboot-dev - < .ci/Dockerfile
docker run --rm -v "$PWD":/work cubiboot-dev bash -lc 'cd entry && make clean && make'    # -> cubeboot/cubeboot.dol (ipl.dol)
docker run --rm -v "$PWD":/work cubiboot-dev bash -lc 'bash /work/.ci/build_apploader.sh' # -> apploader.img
docker run --rm -v "$PWD":/work cubiboot-dev bash -lc 'bash /work/.ci/build_iso.sh'       # -> cubiboot.iso (con la marca)
```

<details>
<summary><b>Cómo se produce cada artefacto</b></summary>

<br/>

- **`apploader.img`** — `cubeboot.elf` empaquetado con el
  [packer de swiss-gc](https://github.com/emukidid/swiss-gc/tree/master/cube/packer)
  (variante reboot) y envuelto en una cabecera de apploader de GameCube. Mira
  [.ci/build_apploader.sh](../.ci/build_apploader.sh).
- **`cubiboot.iso`** — una imagen ISO9660 El-Torito de GameCube construida con `genisoimage`
  a partir del `gbi.hdr` de [cubeboot-tools](https://github.com/makeo/cubeboot-tools)
  (rebrandeado al banner de Cubiboot), con el `.dol` del loader como imagen de arranque. Mira
  [.ci/build_iso.sh](../.ci/build_iso.sh).
- **`cubiboot_picoloader.uf2`** — el firmware de
  [PicoLoader](https://github.com/makeo/PicoLoader) con `cubiboot.iso` incrustado como
  payload, replicando el conversor de PicoLoader de makeo. Mira
  [.ci/make_picoloader_uf2.py](../.ci/make_picoloader_uf2.py).

</details>

## Créditos

Este proyecto se apoya en el trabajo de otros — lo siguiente **no** es original de este fork:

- [cubeboot](https://github.com/OffBroadway/cubeboot) de [TeamOffBroadway](https://github.com/OffBroadway) — el loader IPL de GameCube original. (GPL-2.0)
- [cubiboot](https://github.com/makeo/cubiboot) de [makeo](https://github.com/makeo) — el fork para SD2SP2 / SD Gecko en el que este se basa. (GPL-2.0)
- El **menú en cuadrícula con banners** (`custom-loader-menu`) de [Ben Hetherington](https://github.com/BenHetherington), portado desde cubeboot. (GPL-2.0)
- [Swiss](https://github.com/emukidid/swiss-gc) de [Extrems](https://github.com/Extrems), [emukidid](https://github.com/emukidid) y colaboradores — el cargador de juegos/apps que cubiboot encadena. (GPL-2.0)
- [PicoLoader](https://github.com/makeo/PicoLoader) de [makeo](https://github.com/makeo) — el ODE RP2040 al que apunta el `.uf2`. (GPL-2.0)
- [apploader / cubeboot-tools](https://github.com/makeo/cubeboot-tools) (GPL-2.0)
- [packer](https://github.com/emukidid/swiss-gc/tree/master/cube/packer) (de Swiss) — usado para construir `apploader.img`. (GPL-2.0)
- La opción de configuración **`default_folder`** de [wins1ey](https://github.com/wins1ey), vía el fork [Hazado/cubiboot](https://github.com/Hazado/cubiboot) ([merge](https://github.com/Hazado/cubiboot/commit/c91066b4889346fec288393f6a9fe41304652e49)) — portada a este fork. (GPL-2.0)
- El driver de bloques de la **SD del ODE GC Loader**, obtenido por ingeniería inversa de una compilación `cubiboot-gcldr.iso` y contrastado con `DVD_LowGcodeRead` de libogc2. (GPL-2.0)
- Para el desglose completo, mira el [CREDIT.md](https://github.com/makeo/cubiboot/blob/main/CREDIT.md) del upstream y el [CREDIT.md](../CREDIT.md) de este fork.
