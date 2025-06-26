import serial
import serial.tools.list_ports
import threading
import tkinter as tk
from tkinter import messagebox, ttk
from pycaw.pycaw import AudioUtilities, ISimpleAudioVolume
from PIL import Image
import pystray
import sys

apps = {}
selected_apps = []
ser = None
tray_icon = None
serial_thread = None
connected = False
active_app_index = 0

def get_audio_sessions():
    found = {}
    for session in AudioUtilities.GetAllSessions():
        if session.Process:
            name = session.Process.name()
            if name not in found:
                found[name] = session._ctl.QueryInterface(ISimpleAudioVolume)
    return found

def send_app_list():
    for i in range(3):
        if i < len(selected_apps):
            ser.write(f"APP{i}:{selected_apps[i]}\n".encode())
        else:
            ser.write(f"APP{i}:\n".encode())  # Clear unused slots

def send_volume():
    if not ser or not selected_apps:
        return
    app = selected_apps[active_app_index]
    ctl = apps.get(app)
    if ctl:
        vol = ctl.GetMasterVolume()
        ser.write(f"APP:{app}\n".encode())
        ser.write(f"VOL:{int(vol * 100)}\n".encode())
        
def handle_serial():
    global connected, active_app_index
    while connected:
        try:
            line = ser.readline().decode().strip()
            if not line:
                continue

            if line.startswith("APP:"):
                app_name = line[4:]
                if app_name in selected_apps:
                    active_app_index = selected_apps.index(app_name)
                    update_selected_label()
                    send_volume()  # ← ensure volume is resent for selected app

            elif line.startswith("VOL:") and selected_apps:
                volume = int(line[4:])
                app = selected_apps[active_app_index]
                for session in AudioUtilities.GetAllSessions():
                    if session.Process and session.Process.name() == app:
                        ctl = session._ctl.QueryInterface(ISimpleAudioVolume)
                        ctl.SetMasterVolume(volume / 100.0, None)
                        break

            elif not selected_apps:
                ser.write("NOAPP\n".encode())

        except Exception as e:
            print(f"[Serial error]: {e}")
            break


def update_selected_label():
    if selected_apps:
        selected_label.config(text=f"Current: {selected_apps[active_app_index]}")
    else:
        selected_label.config(text="Current: None")

def connect():
    global ser, apps, connected, serial_thread, selected_apps, active_app_index

    com_port = com_var.get()
    if not com_port or com_port == "No ports available":
        messagebox.showerror("Error", "Please select a COM port.")
        return

    selections = app_listbox.curselection()
    if not selections or len(selections) > 3:
        messagebox.showerror("Error", "Please select 1–3 apps.")
        return

    selected_apps = [app_listbox.get(i) for i in selections]
    active_app_index = 0

    try:
        ser = serial.Serial(com_port, 115200)
        connected = True
    except:
        messagebox.showerror("Error", f"Failed to open {com_port}")
        return

    apps = get_audio_sessions()
    send_app_list()
    send_volume()
    update_selected_label()

    serial_thread = threading.Thread(target=handle_serial, daemon=True)
    serial_thread.start()
    connect_btn.config(state=tk.DISABLED)
    disconnect_btn.config(state=tk.NORMAL)

def disconnect():
    global connected, ser, selected_apps
    connected = False
    if ser:
        try:
            ser.close()
        except:
            pass
        ser = None
    selected_apps = []
    update_selected_label()
    connect_btn.config(state=tk.NORMAL)
    disconnect_btn.config(state=tk.DISABLED)

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

# UI Setup
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

tk.Label(root, text="Select up to 3 Applications:").pack()
app_listbox = tk.Listbox(root, selectmode=tk.MULTIPLE, width=40, height=8)
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
root.geometry("290x370")
root.resizable(width=False, height=False)
root.mainloop()
