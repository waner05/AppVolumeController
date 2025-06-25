import serial
import serial.tools.list_ports
import threading
import tkinter as tk
from tkinter import messagebox, ttk
from pycaw.pycaw import AudioUtilities, ISimpleAudioVolume
from PIL import Image, ImageDraw
import pystray
import sys

selected_app = None
apps = {}
ser = None
tray_icon = None
serial_thread = None
connected = False

# Get audio sessions
def get_audio_sessions():
    found = {}
    for session in AudioUtilities.GetAllSessions():
        if session.Process:
            name = session.Process.name()
            if name not in found:
                found[name] = session._ctl.QueryInterface(ISimpleAudioVolume)
    return found

# Send current volume to ESP
def send_volume():
    if not selected_app or not ser:
        return
    ctl = apps[selected_app]
    vol = ctl.GetMasterVolume()
    try:
        ser.write(f"VOL:{int(vol * 100)}\n".encode())
    except:
        pass

# Handle incoming serial data from ESP
def handle_serial():
    global connected
    while connected:
        try:
            line = ser.readline().decode().strip()
            if not line:
                continue

            if selected_app and line.isdigit():
                volume = int(line)
                volume = max(0, min(volume, 100))
                for session in AudioUtilities.GetAllSessions():
                    if session.Process and session.Process.name() == selected_app:
                        ctl = session._ctl.QueryInterface(ISimpleAudioVolume)
                        ctl.SetMasterVolume(volume / 100.0, None)
                # DO NOT send_volume() here — prevents loop
            elif not selected_app:
                ser.write("NOAPP\n".encode())
        except Exception as e:
            print(f"[Serial error]: {e}")
            break

# Update UI label
def update_selected_label():
    if selected_app:
        selected_label.config(text=f"Current: {selected_app}")
    else:
        selected_label.config(text="Current: None")

# Connect to serial and ESP
def connect():
    global ser, selected_app, apps, connected, serial_thread

    com_port = com_var.get()
    if not com_port or com_port == "No ports available":
        messagebox.showerror("Error", "Please select a COM port.")
        return

    selected_index = app_listbox.curselection()
    if not selected_index:
        messagebox.showerror("Error", "Please select one app.")
        return

    selected_app_name = app_listbox.get(selected_index[0])
    selected_app = selected_app_name

    try:
        ser = serial.Serial(com_port, 115200)
        connected = True
    except:
        messagebox.showerror("Error", f"Failed to open {com_port}")
        return

    send_volume()
    update_selected_label()
    serial_thread = threading.Thread(target=handle_serial, daemon=True)
    serial_thread.start()
    connect_btn.config(state=tk.DISABLED)
    disconnect_btn.config(state=tk.NORMAL)

# Disconnect serial and reset state
def disconnect():
    global connected, ser, selected_app
    connected = False
    if ser:
        try:
            ser.close()
        except:
            pass
        ser = None
    selected_app = None
    update_selected_label()
    connect_btn.config(state=tk.NORMAL)
    disconnect_btn.config(state=tk.DISABLED)

# Tray icon image
def create_image():
    return Image.open("ico.png")

def hide_window():
    root.withdraw()
    icon = pystray.Icon("VolumeCtrl", create_image(), "Rotary Volume Controller", menu=pystray.Menu(
        pystray.MenuItem("Manage", show_window),
        pystray.MenuItem("Quit", quit_app)
    ))
    global tray_icon
    tray_icon = icon
    threading.Thread(target=icon.run, daemon=True).start()

def show_window(icon=None, item=None):
    root.after(0, root.deiconify)
    if tray_icon:
        tray_icon.stop()

def quit_app(icon=None, item=None):
    disconnect()
    if tray_icon:
        tray_icon.stop()
    root.destroy()
    sys.exit()

# --- UI Setup ---
root = tk.Tk()
root.title("Rotary Volume Controller")

tk.Label(root, text="Select COM Port:").pack()
ports = [p.device for p in serial.tools.list_ports.comports()]
com_var = tk.StringVar()
com_dropdown = ttk.Combobox(root, textvariable=com_var, values=ports, state="readonly")
if ports:
    com_dropdown.current(0)
else:
    com_dropdown.set("No ports available")
com_dropdown.pack(pady=5)

tk.Label(root, text="Select an Application:").pack()
app_listbox = tk.Listbox(root, selectmode=tk.SINGLE, width=40, height=8)
app_listbox.pack()

apps = get_audio_sessions()
for name in apps.keys():
    app_listbox.insert(tk.END, name)

connect_btn = tk.Button(root, text="Connect", command=connect)
connect_btn.pack(pady=5)

disconnect_btn = tk.Button(root, text="Disconnect", command=disconnect, state=tk.DISABLED)
disconnect_btn.pack(pady=5)

selected_label = tk.Label(root, text="Current: None", font=("Arial", 12, "bold"))
selected_label.pack(pady=5)

root.protocol("WM_DELETE_WINDOW", hide_window)
root.geometry("270x330")
root.resizable(width=False, height=False)
root.mainloop()
