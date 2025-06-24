import serial
import serial.tools.list_ports
import threading
import tkinter as tk
from tkinter import messagebox, ttk
from pycaw.pycaw import AudioUtilities, ISimpleAudioVolume
from PIL import Image, ImageDraw
import pystray
import sys

selected_index = 0
selected_sessions = []
apps = {}
ser = None
tray_icon = None

# Get audio sessions
def get_audio_sessions():
    found = {}
    for session in AudioUtilities.GetAllSessions():
        if session.Process:
            name = session.Process.name()
            if name not in found:
                found[name] = session._ctl.QueryInterface(ISimpleAudioVolume)
    return found

# Send volume to serial
def send_volume():
    global selected_index
    if not selected_apps:
        return
    ctl = apps[selected_apps[selected_index]]
    vol = ctl.GetMasterVolume()
    ser.write(f"APP:{selected_apps[selected_index]}\n".encode())
    ser.write(f"VOL:{int(vol * 100)}\n".encode())

# Handle incoming serial data
def handle_serial():
    global selected_index
    while True:
        line = ser.readline().decode().strip()
        if not line or not selected_apps:
            continue

        ctl = apps[selected_apps[selected_index]]
        vol = ctl.GetMasterVolume()

        if line == "+1":
            vol = min(vol + 0.05, 1.0)
            ctl.SetMasterVolume(vol, None)
        elif line == "-1":
            vol = max(vol - 0.05, 0.0)
            ctl.SetMasterVolume(vol, None)
        elif line == "BTN":
            selected_index = (selected_index + 1) % len(selected_apps)
            print(f"Switched to: {selected_apps[selected_index]}")
            update_selected_label()

        send_volume()

# Update current app label
def update_selected_label():
    selected_label.config(text=f"Current: {selected_apps[selected_index]}")

# Connect to serial and start handler
def connect():
    global ser, selected_apps, apps

    com_port = com_var.get()
    if not com_port or com_port == "No ports available":
        messagebox.showerror("Error", "Please select a COM port.")
        return

    selected_indices = app_listbox.curselection()
    if not selected_indices:
        messagebox.showerror("Error", "Please select up to 3 apps.")
        return

    selected_apps = [app_listbox.get(i) for i in selected_indices[:3]]
    if len(selected_apps) == 0:
        messagebox.showerror("Error", "No apps selected.")
        return

    try:
        ser = serial.Serial(com_port, 115200)
    except:
        messagebox.showerror("Error", f"Failed to open {com_port}")
        return

    send_volume()
    update_selected_label()
    threading.Thread(target=handle_serial, daemon=True).start()
    connect_btn.config(state=tk.DISABLED)

# Create tray icon image
def create_image():
    img = Image.new("RGB", (64, 64), "black")
    draw = ImageDraw.Draw(img)
    draw.ellipse((16, 16, 48, 48), fill="white")
    return img

# Hide to tray
def hide_window():
    root.withdraw()
    icon = pystray.Icon("VolumeCtrl", create_image(), "Rotary Volume Controller", menu=pystray.Menu(
        pystray.MenuItem("Manage", show_window),
        pystray.MenuItem("Quit", quit_app)
    ))
    global tray_icon
    tray_icon = icon
    threading.Thread(target=icon.run, daemon=True).start()

# Show window from tray
def show_window(icon=None, item=None):
    root.after(0, root.deiconify)
    if tray_icon:
        tray_icon.stop()

# Quit app from tray
def quit_app(icon=None, item=None):
    if tray_icon:
        tray_icon.stop()
    root.destroy()
    sys.exit()

# --- UI Setup ---
root = tk.Tk()
root.title("Rotary Volume Controller")

# COM Port Selector
tk.Label(root, text="Select COM Port:").pack()
ports = [p.device for p in serial.tools.list_ports.comports()]

com_var = tk.StringVar()
com_dropdown = ttk.Combobox(root, textvariable=com_var, values=ports, state="readonly")

if ports:
    com_dropdown.current(0)  # Select first port
else:
    com_dropdown.set("No ports available")
   # com_dropdown.config(state="disabled")

com_dropdown.pack(pady=5)

# App listbox
tk.Label(root, text="Select up to 3 Applications:").pack()
app_listbox = tk.Listbox(root, selectmode=tk.MULTIPLE, width=40, height=8)
app_listbox.pack()

apps = get_audio_sessions()
for name in apps.keys():
    app_listbox.insert(tk.END, name)

# Connect button
connect_btn = tk.Button(root, text="Connect", command=connect)
connect_btn.pack(pady=10)

# Status label
selected_label = tk.Label(root, text="Current: None", font=("Arial", 12, "bold"))
selected_label.pack(pady=5)

# Handle window close
root.protocol("WM_DELETE_WINDOW", hide_window)
root.geometry("270x300")
root.resizable(width=False, height=False)
root.mainloop()
