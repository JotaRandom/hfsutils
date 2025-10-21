## Objetivo rápido
Ayuda a implementar y mantener las utilidades HFS (hfsutils). El repo genera un único binario `hfsutil` que actúa como envoltorio para subcomandos (hls, hcopy, hmount, ...). Prioriza cambios en `src/` y en las carpetas de librerías `libhfs/` y `librsrc/`.

## Entradas / Salidas y contrato mínimo
- Input: código C en `src/`, `libhfs/`, `librsrc/`.
- Output: ejecutable `hfsutil` (root del repo) y opcionalmente symlinks tradicionales.
- Error modes: fallos frecuentes en configure/make; usar `./build.sh` y `make clean` para reproducir.

## Estructura clave (qué editar y dónde)
- Binarios y symlinks: `Makefile` en la raíz compila `hfsutil` y crea symlinks en `EXECUTABLES`.
- Bibliotecas: `libhfs/` y `librsrc/` contienen su propio `Makefile` y `configure` — muchos cambios de API requieren recompilar esas carpetas primero.
- Código de utilidades: `src/hfsutil/*.c` implementa cada subcomando (ej. `hcopy.c`, `hls.c`, `hmount.c`). Cambios en la interfaz de subcomando deben reflejarse en `hfsutil.c`.

## Flujo de desarrollo y comandos útiles
- IMPORTANTE: Antes de ejecutar cualquier `make`, ejecutar `make clean` en la raíz y en las subcarpetas relevantes (`libhfs/`, `librsrc/`, `hfsck/`) para asegurarse de un build reproducible.

- Build rápido (usa los scripts del repo):
  - `./build.sh` — script cómodo que ejecuta `./configure` y `make` en `libhfs/` y `librsrc/`, luego `make` en la raíz. Nota: el script asume que ha ejecutado `make clean` previamente si desea un build desde cero.
  - `make` — compila `hfsutil` (requiere que `libhfs` y `librsrc` ya estén configuradas/compiladas). Recomendado: `make clean; make`.
  - `make symlinks` o `make install-symlinks` — crea los nombres tradicionales (por ejemplo `hcopy`) apuntando a `hfsutil`.
  - `make test` — ejecuta `test/generate_test_data.sh` y `test/run_tests.sh`. Recomendado: `make clean; ./build.sh; make symlinks; make test`.

## Patrones y convenciones específicas
- Un único binario multipropósito: la raíz contiene `hfsutil` que despacha subcomandos; los archivos en `src/hfsutil/` implementan cada comando. Para agregar un subcomando, añada el .c en `src/hfsutil/`, registrelo en `Makefile` (`OBJDIR`/objetos) y extienda `hfsutil.c` si es necesario.
- Prefer compilación por separado de librerías: no modifique `libhfs/` o `librsrc/` sin actualizar sus `configure`/`Makefile`; use `./build.sh` para garantizar el orden correcto.
- Objetos en `build/obj`: el Makefile coloca objetos en `build/obj`; respetar esa convención al añadir nuevos .o.

## Integraciones y dependencias externas
- Dependencias estándar: compilador C (gcc/clang), `make`, utilidades POSIX. El proyecto asume un entorno Unix-like.
- No hay dependencias de paquetes de terceros en tiempo de ejecución; las integraciones críticas son las llamadas a funciones dentro de `libhfs` y `librsrc`.

## Ejemplos concretos (copiar/pegar para el agente)
- Para reproducir un fallo de compilación en CI: ejecutar
  - `./build.sh` (ver salida) luego `make` en la raíz.
- Añadir un nuevo subcomando `hfoo`:
  1. Añadir `src/hfsutil/hfoo.c` implementando `main` o las funciones necesarias.
  2. Añadir `$(OBJDIR)/hfoo.o` en el `Makefile` y en `UTIL_OBJS`.
  3. Ejecutar `make` y validar `./hfsutil hfoo`.

## Qué evitar / límites del agente
- No cambiar permisos de setuid ni recomendar instalación setuid; el README prohíbe setuid por razones de seguridad.
- No asumir herramientas no listadas en README; si hace falta una dependencia nueva, documentarla en el PR.

## Archivos de referencia rápido
- `build.sh` — script de construcción principal
- `Makefile` — reglas de build y symlink
- `src/hfsutil/*.c` — implementaciones de subcomandos
- `libhfs/`, `librsrc/` — librerías de bajo nivel
- `hfsck/` — herramientas de verificación de HFS
- `test/` — scripts de pruebas y generación de datos

Si algo falta o deseas que el archivo incluya ejemplos más específicos (por ejemplo, fragmentos de `hfsutil.c` o cómo ejecutar pruebas en Windows vs WSL), dímelo y lo itero.
