import tkinter as tk
from tkinter import messagebox
from PIL import Image, ImageTk
import subprocess
import random
import os
import time


EXECUTABLE = ".obould/fractal-flame"
OUTPUT_IMAGE = "preview.png"
PARAMS_FILE = "saved_params.txt"

WIDTH = 800
HEIGHT = 600
DEFAULT_ITERATIONS = 10_000_000
THREADS = os.cpu_count()

VARIATIONS = [
    "linear", "sinusoidal", "spherical", "horseshoe", "swirl", "polar",
    "handkerchief", "heart", "hyperbolic", "disk", "spiral", "diamond",
    "ex", "eyefish", "tangent"
]

class FractalGenerator:
    def __init__(self, root):
        self.root = root
        self.root.title("Fractal Flame - Obould")

        self.current_affine = ""
        self.current_funcs = ""
        self.current_symmetry = 1
        self.current_seed = 0

        self.last_executed_cmd = ""


        self.img_label = tk.Label(root, bg="#222", fg="#fff")
        self.img_label.pack(pady=10, fill=tk.BOTH, expand=True)

        settings_frame = tk.Frame(root)
        settings_frame.pack(pady=5)

        tk.Label(settings_frame, text="Gamma:").pack(side=tk.LEFT, padx=5)
        self.entry_gamma = tk.Entry(settings_frame, width=10)
        self.entry_gamma.pack(side=tk.LEFT, padx=5)
        self.entry_gamma.insert(0, "1.5")

        tk.Label(settings_frame, text="Iterations:").pack(side=tk.LEFT, padx=5)
        self.entry_iters = tk.Entry(settings_frame, width=15)
        self.entry_iters.pack(side=tk.LEFT, padx=5)
        self.entry_iters.insert(0, str(DEFAULT_ITERATIONS))


        btn_frame = tk.Frame(root)
        btn_frame.pack(pady=10)

        self.btn_gen = tk.Button(btn_frame, text="New Random (Space)", command=self.generate_new_random, bg="#dddddd")
        self.btn_gen.pack(side=tk.LEFT, padx=10)

        self.btn_upd = tk.Button(btn_frame, text="Re-render (R)", command=self.rerender_current, bg="#ffffaa")
        self.btn_upd.pack(side=tk.LEFT, padx=10)

        self.btn_save = tk.Button(btn_frame, text="Save Params (Enter)", command=self.save_current_params, bg="#aaffaa")
        self.btn_save.pack(side=tk.LEFT, padx=10)

        self.lbl_info = tk.Label(root, text="Ready", wraplength=800, justify="center")
        self.lbl_info.pack(pady=10)


        root.bind('<space>', lambda e: self.generate_new_random())
        root.bind('r', lambda e: self.rerender_current())
        root.bind('R', lambda e: self.rerender_current())
        root.bind('<Return>', lambda e: self.save_current_params())


    def get_random_affine(self, count):
        affines = []
        for _ in range(count):
            coeffs = [round(random.uniform(-1.2, 1.2), 3) for _ in range(6)]
            affines.append(",".join(map(str, coeffs)))
        return "/".join(affines)


    def get_random_functions(self):
        count = random.randint(1, 4)
        chosen = random.sample(VARIATIONS, count)
        funcs = []
        for name in chosen:
            weight = round(random.uniform(0.2, 1.0), 2)
            funcs.append(f"{name}:{weight}")
        return ",".join(funcs)


    def generate_new_random(self):
        num_transforms = random.randint(2, 5)
        self.current_affine = self.get_random_affine(num_transforms)
        self.current_funcs = self.get_random_functions()

        self.current_symmetry = 1
        if random.random() > 0.7:
            self.current_symmetry = random.randint(2, 6)

        random_gamma = round(random.uniform(0.5, 2.5), 2)
        self.entry_gamma.delete(0, tk.END)
        self.entry_gamma.insert(0, str(random_gamma))
        self.current_seed = random.randint(0, 2**31)

        self.run_render_process()


    def rerender_current(self):
        if not self.current_affine:
            messagebox.showwarning("Warning", "Сначала сгенерируйте фрактал (Space)")
            return
        self.run_render_process()

    def run_render_process(self):
        self.lbl_info.config(text="Generating... Please wait.")
        self.root.update()

        try:
            gamma_val = float(self.entry_gamma.get())
            iter_count = int(self.entry_iters.get())
        except ValueError:
            messagebox.showerror("Error", "Gamma должна быть числом, Iterations - целым числом")
            return

        cmd = [
            EXECUTABLE,
            "-w", str(WIDTH),
            "-h", str(HEIGHT),
            "-i", str(iter_count),
            "-t", str(THREADS),
            "-o", OUTPUT_IMAGE,
            "-ap", self.current_affine,
            "-f", self.current_funcs,
            "--gamma", str(gamma_val),
            "-s", str(self.current_symmetry),
            "-g",
            "--seed", str(self.current_seed)
        ]
        self.last_executed_cmd = " ".join(cmd)

        desc_str = (
            f"Funcs: {self.current_funcs}\n"
            f"Sym: {self.current_symmetry} | Gamma: {gamma_val} | Iters: {iter_count}"
        )

        try:
            start_time = time.time()

            result = subprocess.run(
                cmd,
                check=True,
                capture_output=True,
                text=True,
                timeout=300
            )

            elapsed = time.time() - start_time

            if os.path.exists(OUTPUT_IMAGE):
                load = Image.open(OUTPUT_IMAGE)
                load.thumbnail((WIDTH, HEIGHT))
                render = ImageTk.PhotoImage(load)

                self.img_label.config(image=render, text="")
                self.img_label.image = render

                self.lbl_info.config(text=f"Done in {elapsed:.2f}s\n{desc_str}")
            else:
                self.lbl_info.config(text="Error: Image file not created!")
                print("STDOUT:", result.stdout)
                print("STDERR:", result.stderr)

        except subprocess.TimeoutExpired:
            self.lbl_info.config(text="Timeout!")
        except subprocess.CalledProcessError as e:
            print(e.stderr)
            self.lbl_info.config(text=f"Error")
        except Exception as e:
            print(f"Python Error: {e}")
            self.lbl_info.config(text=f"Error: {e}")


    def save_current_params(self):
        if not self.last_executed_cmd:
            return

        with open(PARAMS_FILE, "a", encoding="utf-8") as f:
            f.write("-" * 50 + "\n")
            f.write(f"\n{self.last_executed_cmd}\n")

        messagebox.showinfo("Saved", f"Parameters saved to {PARAMS_FILE}")


if __name__ == "__main__":
    if not os.path.exists(EXECUTABLE):
        print(f"Error: Executable not found at {EXECUTABLE}")
        exit(1)

    root = tk.Tk()
    app = FractalGenerator(root)
    root.mainloop()