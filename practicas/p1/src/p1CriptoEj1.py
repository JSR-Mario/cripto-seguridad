# Magic bytes de formatos conocidos
MAGIC_BYTES = {
    b"%PDF": "pdf",
    b"\x89PNG\r\n\x1a\n": "png",
    b"ID3": "mp3",
    b"\xff\xfb": "mp3",
    b"ftyp": "mp4",
}

def read_file(filename):
    with open(filename, "rb") as f:
        return bytearray(f.read())

def write_file(filename, data):
    with open(filename, "wb") as f:
        f.write(data)

def detect_format(data):
    for magic, ext in MAGIC_BYTES.items():
        if data.startswith(magic):
            return ext
    return None

def affine_decrypt(data, a, b):
    print(f'probando cifrado afín con a: {a}, b: {b}')
    try:
        inv = pow(a, -1, 256)
    except ValueError:
        return None
    return bytearray(((b_ - b) * inv) % 256 for b_ in data)

# metodo para intentar parametros a y b para el cifrado afín
def try_values(data:bytearray, a:int, b:int):
    dec = affine_decrypt(data, a, b)
    if dec:
        ext = detect_format(dec)
        if ext:
            out = f"file1_affine_{a}_{b}.{ext}"
            write_file(out, dec)
            print(f"[+] Encontrado Afín con claves a={a}, b={b}, guardado como {out}")
            return True

def brute_force(filename, a:int=None, b:int=None):
    data = read_file(filename)

    if a and b: # intentando con parametros manuales
        if try_values(data, a, b):
            return

    """ esta parte la skipeo por el tiempo, hice la ejecución completa
    para obtener los parametros a y b correctos"""
    # Probando Afín (fuerza bruta) 
    for a in range(1, 256, 2):  # solo valores coprimos con 256
        for b in range(256):
            if try_values(data, a, b): return

if __name__ == "__main__":
    from pathlib import Path
    FILE = Path("file1.lol")
    # pongo los parametros encontrados ya que hice la ejecución por fuerza bruta
    brute_force(str(FILE), a=143, b=157)  