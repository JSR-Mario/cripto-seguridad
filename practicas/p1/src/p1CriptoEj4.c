#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

typedef struct {
    const char *extension;
    const uint8_t signature[8];
    size_t length;
    const char *description;
} MagicNumber;

MagicNumber magic_numbers[] = {
    {"png", {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A}, 8, "PNG image"},
    {"pdf", {0x25, 0x50, 0x44, 0x46, 0x2D}, 5, "PDF document"},
    {"mp3", {0x49, 0x44, 0x33}, 3, "MP3 audio"},
    {"mp4", {0x66, 0x74, 0x79, 0x70}, 4, "MP4 video"},
};

typedef struct {
    int method; // 0=César, 1=Afín, 2=Base64
    int key1;   // shift para César, a para Afín
    int key2;   // b para Afín
    const char *format;
} DecryptionResult;

// Función para leer archivo
uint8_t* read_file(const char *filename, size_t *length) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error: No se pudo abrir %s\n", filename);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    *length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    uint8_t *data = malloc(*length);
    if (!data) {
        fclose(file);
        return NULL;
    }
    
    fread(data, 1, *length, file);
    fclose(file);
    
    return data;
}

// Función para escribir archivo
int write_file(const char *filename, const uint8_t *data, size_t length) {
    FILE *file = fopen(filename, "wb");
    if (!file) return 0;
    
    fwrite(data, 1, length, file);
    fclose(file);
    return 1;
}

// Función para calcular el MCD
int gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

// Función para verificar si es Base64
int is_base64(const uint8_t *data, size_t length) {
    if (length < 4) return 0;
    
    for (size_t i = 0; i < (length > 100 ? 100 : length); i++) {
        if (!(data[i] >= 'A' && data[i] <= 'Z') &&
            !(data[i] >= 'a' && data[i] <= 'z') &&
            !(data[i] >= '0' && data[i] <= '9') &&
            data[i] != '+' && data[i] != '/' && data[i] != '=' &&
            data[i] != '\n' && data[i] != '\r') {
            return 0;
        }
    }
    return 1;
}

// Función Base64 decoding 
void base64_decode(const char *data, uint8_t *output, size_t *out_length) {
    *out_length = 0;
}

// Función para analizar el cifrado automáticamente
DecryptionResult analyze_encryption(const uint8_t *encrypted_data, size_t length) {
    DecryptionResult result = {-1, -1, -1, NULL};
    
    if (length < 8) {
        printf("Archivo demasiado corto para análisis\n");
        return result;
    }
    
    printf("Analizando header cifrado: ");
    for (int i = 0; i < 8; i++) printf("%02X ", encrypted_data[i]);
    printf("\n");
    
    // 1. Primero verificar si es Base64
    if (is_base64(encrypted_data, length)) {
        printf("✓ Detectado: Base64 encoding\n");
        result.method = 2;
        result.format = "mp4"; 
        return result;
    }
    
    // 2. Analizar para César y Afín
    for (int magic_idx = 0; magic_idx < sizeof(magic_numbers)/sizeof(magic_numbers[0]); magic_idx++) {
        const uint8_t *original_magic = magic_numbers[magic_idx].signature;
        size_t magic_len = magic_numbers[magic_idx].length;
        
        if (length < magic_len) continue;
        
        printf("Probando con %s...\n", magic_numbers[magic_idx].description);
        
        // Análisis para César
        int shifts[8];
        int consistent_shift = 1;
        int first_shift = (encrypted_data[0] - original_magic[0] + 256) % 256;
        
        for (int i = 0; i < magic_len; i++) {
            shifts[i] = (encrypted_data[i] - original_magic[i] + 256) % 256;
            if (shifts[i] != first_shift) {
                consistent_shift = 0;
            }
        }
        
        if (consistent_shift) {
            printf("✓ Detectado: César cipher (shift=%d) -> %s\n", first_shift, magic_numbers[magic_idx].extension);
            result.method = 0;
            result.key1 = first_shift;
            result.format = magic_numbers[magic_idx].extension;
            return result;
        }
        
        // Análisis para Afín (solo si el magic length es suficiente)
        if (magic_len >= 2) {
            // a*orig0 + b ≡ enc0 mod 256
            // a*orig1 + b ≡ enc1 mod 256
            
            int orig0 = original_magic[0], orig1 = original_magic[1];
            int enc0 = encrypted_data[0], enc1 = encrypted_data[1];
            
            // Restar ecuaciones: a*(orig1 - orig0) ≡ (enc1 - enc0) mod 256
            int diff_orig = (orig1 - orig0 + 256) % 256;
            int diff_enc = (enc1 - enc0 + 256) % 256;
            
            if (diff_orig != 0 && gcd(diff_orig, 256) == 1) {
                // Buscar a tal que a*diff_orig ≡ diff_enc mod 256
                for (int a = 1; a < 256; a++) {
                    if ((a * diff_orig) % 256 == diff_enc) {
                        // Calcular b 
                        int b = (enc0 - a * orig0 + 256) % 256;
                        
                        // Verificar con más bytes
                        int valid = 1;
                        for (int i = 2; i < magic_len && i < 4; i++) {
                            int expected = (a * original_magic[i] + b) % 256;
                            if (expected != encrypted_data[i]) {
                                valid = 0;
                                break;
                            }
                        }
                        
                        if (valid) {
                            printf("✓ Detectado: Afín cipher (a=%d, b=%d) -> %s\n", a, b, magic_numbers[magic_idx].extension);
                            result.method = 1;
                            result.key1 = a;
                            result.key2 = b;
                            result.format = magic_numbers[magic_idx].extension;
                            return result;
                        }
                    }
                }
            }
        }
    }
    
    printf("✗ No se pudo determinar el método de cifrado\n");
    return result;
}

// Función principal de descifrado 
void auto_decrypt(const char *filename) {
    printf("\n=== ANALIZANDO %s ===\n", filename);
    
    size_t length;
    uint8_t *data = read_file(filename, &length);
    if (!data) return;
    
    DecryptionResult result = analyze_encryption(data, length);
    
    if (result.method != -1) {
        printf("Aplicando descifrado...\n");
        
        uint8_t *decrypted_data = malloc(length);
        memcpy(decrypted_data, data, length);
        
        switch (result.method) {
            case 0: // César
                for (size_t i = 0; i < length; i++) {
                    decrypted_data[i] = (decrypted_data[i] - result.key1 + 256) % 256;
                }
                break;
                
            case 1: // Afín
                {
                    // Encontrar inverso modular de a
                    int a_inv = 0;
                    for (int i = 0; i < 256; i++) {
                        if ((result.key1 * i) % 256 == 1) {
                            a_inv = i;
                            break;
                        }
                    }
                    for (size_t i = 0; i < length; i++) {
                        decrypted_data[i] = (a_inv * (decrypted_data[i] - result.key2 + 256)) % 256;
                    }
                }
                break;
                
            case 2: 
                printf("Base64 decoding necesario (implementar)\n");
                free(decrypted_data);
                free(data);
                return;
        }
        
        // Verificar magic number resultante
        const char *detected_format = NULL;
        for (int i = 0; i < sizeof(magic_numbers)/sizeof(magic_numbers[0]); i++) {
            if (memcmp(decrypted_data, magic_numbers[i].signature, magic_numbers[i].length) == 0) {
                detected_format = magic_numbers[i].extension;
                break;
            }
        }
        
        // Extraer carpeta y nombre base del archivo original
        const char *last_slash = strrchr(filename, '/');
        char folder[256] = "";
        char base[256] = "";
        if (last_slash) {
            size_t folder_len = last_slash - filename + 1;
            strncpy(folder, filename, folder_len);
            folder[folder_len] = '\0';
            strcpy(base, last_slash + 1);
        } else {
            strcpy(base, filename);
        }

        char output_file[512];
        if (detected_format) {
            snprintf(output_file, sizeof(output_file), "%sdecrypted_%s.%s", folder, base, detected_format);
            printf("✓ Descifrado exitoso: %s\n", output_file);
        } else {
            snprintf(output_file, sizeof(output_file), "%sdecrypted_%s.bin", folder, base);
            printf("✓ Descifrado completo pero formato no identificado: %s\n", output_file);
        }

        write_file(output_file, decrypted_data, length);
        free(decrypted_data);
    }
    
    free(data);
}


int main() {
    printf("=== ANALIZADOR INTELIGENTE DE CIFRADO ===\n");
    auto_decrypt("../file4.lol");
    printf("\n=== ANÁLISIS COMPLETADO ===\n");
    return 0;
}
