
ROOT_DIR := ../..
COMPILER := $(ROOT_DIR)/build/obould
OUT_DIR := .obould
BIN := $(OUT_DIR)/fractal-flame

all: $(BIN)

$(BIN): $(OUT_DIR)/FlameTypes.o $(OUT_DIR)/FlameCli.o $(OUT_DIR)/FlameRenderer.o
	$(COMPILER) FractalFlame.obl --main --link --obj $(OUT_DIR)/FlameTypes.o --obj $(OUT_DIR)/FlameCli.o --obj $(OUT_DIR)/FlameRenderer.o -o $(BIN)

$(OUT_DIR)/FlameTypes.o: FlameTypes.obl
	$(COMPILER) $< -S $(OUT_DIR) -o $@

$(OUT_DIR)/FlameCli.o: FlameCli.obl $(OUT_DIR)/FlameTypes.o
	$(COMPILER) $< -S $(OUT_DIR) -o $@

$(OUT_DIR)/FlameRenderer.o: FlameRenderer.obl $(OUT_DIR)/FlameTypes.o
	$(COMPILER) $< -S $(OUT_DIR) -o $@

clean:
	rm -rf $(OUT_DIR)

rebuild: clean all

run: $(BIN)
	$(BIN) -w 800 -h 600 -i 10000000 -t 8 -o preview.png -ap 0.635,-0.932,-0.848,0.767,1.174,-0.533/1.002,0.499,-0.574,-0.248,-0.705,-0.036/-0.217,1.083,1.018,-1.131,-0.523,-0.428/-0.839,0.33,0.511,-0.46,-0.904,-0.97 -f spherical:0.74 --gamma 0.64 -g -s 2

.PHONY: all clean rebuild run
