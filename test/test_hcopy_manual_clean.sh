#!/bin/bash
set -e

echo "========================================="
echo "  Test manual de hcopy -R (limpio)"
echo "========================================="

# Limpiar y crear entorno
rm -rf /tmp/test_rec
mkdir -p /tmp/test_rec
cd /tmp/test_rec

# Crear estructura de directorios
echo "[1] Creando estructura de directorios de prueba..."
mkdir -p source/nivel1/nivel2
echo 'archivo raiz' > source/file_root.txt
echo 'archivo nivel1' > source/nivel1/file1.txt  
echo 'archivo nivel2' > source/nivel1/nivel2/file2.txt

echo "Estructura creada:"
find source -type f -o -type d | sort

# Crear volumen HFS
echo ""
echo "[2] Creando volumen HFS..."
dd if=/dev/zero of=test.hfs bs=1M count=5 2>/dev/null
/mnt/c/Users/Usuario/source/repos/hfsutils/hfsutil hformat -l 'TestRec' test.hfs 2>&1 | head -n 3

# Montar volumen
echo ""
echo "[3] Montando volumen..."
/mnt/c/Users/Usuario/source/repos/hfsutils/hfsutil hmount test.hfs

# Verificar que está vacío
echo ""
echo "[4] Contenido inicial (debe estar vacío):"
/mnt/c/Users/Usuario/source/repos/hfsutils/hfsutil hls || echo "(vacío)"

# Test 1: Intentar copiar SIN -R (debe fallar)
echo ""
echo "[5] Test 1: Copiar directorio SIN -R (debe fallar con EISDIR)..."
if /mnt/c/Users/Usuario/source/repos/hfsutils/hfsutil hcopy source :source 2>&1; then
    echo "❌ FALLÓ: Debería rechazar el directorio sin -R"
    exit 1
else
    echo "✓ Correctamente rechazó el directorio sin -R"
fi

# Test 2: Copiar CON -R (debe funcionar)
echo ""
echo "[6] Test 2: Copiar directorio CON -R (debe funcionar)..."
if /mnt/c/Users/Usuario/source/repos/hfsutils/hfsutil hcopy -R source :source 2>&1; then
    echo "✓ hcopy -R ejecutado exitosamente"
else
    echo "❌ FALLÓ: hcopy -R retornó error"
    exit 1
fi

# Test 3: Verificar estructura de directorios
echo ""
echo "[7] Test 3: Verificando estructura de directorios..."
echo "Directorios en raíz:"
/mnt/c/Users/Usuario/source/repos/hfsutils/hfsutil hls

echo ""
echo "Contenido de :source:"
/mnt/c/Users/Usuario/source/repos/hfsutils/hfsutil hls :source

echo ""
echo "Contenido de :source:nivel1:"
/mnt/c/Users/Usuario/source/repos/hfsutils/hfsutil hls :source:nivel1

echo ""
echo "Contenido de :source:nivel1:nivel2:"
/mnt/c/Users/Usuario/source/repos/hfsutils/hfsutil hls :source:nivel1:nivel2

# Test 4: Verificar archivos
echo ""
echo "[8] Test 4: Verificando contenido de archivos..."

# Copiar de vuelta y verificar
/mnt/c/Users/Usuario/source/repos/hfsutils/hfsutil hcopy :source:file_root.txt /tmp/test_rec/verify_root.txt
/mnt/c/Users/Usuario/source/repos/hfsutils/hfsutil hcopy :source:nivel1:file1.txt /tmp/test_rec/verify_nivel1.txt
/mnt/c/Users/Usuario/source/repos/hfsutils/hfsutil hcopy :source:nivel1:nivel2:file2.txt /tmp/test_rec/verify_nivel2.txt

echo "Contenido de file_root.txt:"
cat /tmp/test_rec/verify_root.txt

echo "Contenido de file1.txt:"
cat /tmp/test_rec/verify_nivel1.txt

echo "Contenido de file2.txt:"
cat /tmp/test_rec/verify_nivel2.txt

# Limpiar
echo ""
echo "[9] Desmontando..."
/mnt/c/Users/Usuario/source/repos/hfsutils/hfsutil humount

echo ""
echo "========================================="
echo "  ✓ TODOS LOS TESTS PASARON"
echo "========================================="
