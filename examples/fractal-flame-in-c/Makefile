ROOT_DIR := ../..
SRC_DIR := ../fractal-flame
COMPILER := $(ROOT_DIR)/build/obould
STDLIB := $(ROOT_DIR)/stdlib
SYSTEM_DIR := $(ROOT_DIR)/build/system
OUT_DIR := .obould
BIN := $(OUT_DIR)/fractal-flame

CC := cc

STD_MODULES := Args Math Out Parse Random Strings Images Threads
APP_MODULES := FlameTypes FlameCli FlameRenderer FractalFlame

STD_SYMBOLS := $(STD_MODULES:%=$(OUT_DIR)/%.json)
APP_SYMBOLS := $(APP_MODULES:%=$(OUT_DIR)/%.json)

STD_OBJS := $(STD_MODULES:%=$(OUT_DIR)/stdlib/%.o)
APP_OBJS := $(APP_MODULES:%=$(OUT_DIR)/%.o)

PKG_GC_CFLAGS := $(shell pkg-config --silence-errors --cflags bdw-gc)
PKG_GC_LIBS := $(shell pkg-config --silence-errors --libs bdw-gc)

ifeq ($(strip $(PKG_GC_CFLAGS)),)
ifneq (,$(wildcard $(ROOT_DIR)/build/_deps/bdwgc-src/include/gc/gc.h))
GC_CFLAGS := -I$(ROOT_DIR)/build/_deps/bdwgc-src/include
else
GC_CFLAGS := -I/opt/homebrew/include -I/usr/local/include
endif
else
GC_CFLAGS := $(PKG_GC_CFLAGS)
endif

ifeq ($(strip $(PKG_GC_LIBS)),)
ifneq (,$(wildcard $(ROOT_DIR)/build/lib/libgc.a))
GC_LIBS := -L$(ROOT_DIR)/build/lib -lgc
else
GC_LIBS := -L/opt/homebrew/lib -L/usr/local/lib -lgc
endif
else
GC_LIBS := $(PKG_GC_LIBS)
endif

APP_CFLAGS := -I$(STDLIB) -I. -Wno-incompatible-pointer-types $(GC_CFLAGS) \
	-DMaxPath=ob_10FlameTypes_MaxPath \
	-DMaxAffine=ob_10FlameTypes_MaxAffine \
	-DMaxFunctions=ob_10FlameTypes_MaxFunctions \
	-DMaxWidth=ob_10FlameTypes_MaxWidth \
	-DMaxHeight=ob_10FlameTypes_MaxHeight \
	-Dob_4Math_PI=3.14159265358979323846

STD_CFLAGS := -Wno-incompatible-pointer-types $(GC_CFLAGS)
LDFLAGS := -lm $(GC_LIBS) -lpthread

all: $(BIN)

symbols: $(STD_SYMBOLS) $(APP_SYMBOLS)

transpile: $(APP_MODULES:%=$(OUT_DIR)/%.transpile.stamp)

$(BIN): $(APP_OBJS) $(STD_OBJS)
	$(CC) $(APP_OBJS) $(STD_OBJS) $(LDFLAGS) -o $@

$(OUT_DIR)/%.json: $(SYSTEM_DIR)/%.json | $(OUT_DIR)
	cp $< $@

$(OUT_DIR)/FlameTypes.json: $(SRC_DIR)/FlameTypes.obl $(STD_SYMBOLS) | $(OUT_DIR)
	$(COMPILER) $< --emit-symbols -S $(OUT_DIR)

$(OUT_DIR)/FlameCli.json: $(SRC_DIR)/FlameCli.obl $(OUT_DIR)/FlameTypes.json $(STD_SYMBOLS) | $(OUT_DIR)
	$(COMPILER) $< --emit-symbols -S $(OUT_DIR)

$(OUT_DIR)/FlameRenderer.json: $(SRC_DIR)/FlameRenderer.obl $(OUT_DIR)/FlameTypes.json $(STD_SYMBOLS) | $(OUT_DIR)
	$(COMPILER) $< --emit-symbols -S $(OUT_DIR)

$(OUT_DIR)/FractalFlame.json: $(SRC_DIR)/FractalFlame.obl $(OUT_DIR)/FlameCli.json $(OUT_DIR)/FlameRenderer.json $(OUT_DIR)/FlameTypes.json $(STD_SYMBOLS) | $(OUT_DIR)
	$(COMPILER) $< --emit-symbols -S $(OUT_DIR)

$(OUT_DIR)/FlameTypes.transpile.stamp: $(SRC_DIR)/FlameTypes.obl $(OUT_DIR)/FlameTypes.json
	$(COMPILER) $< --emit-c -S $(OUT_DIR) -o .
	touch $@

$(OUT_DIR)/FlameCli.transpile.stamp: $(SRC_DIR)/FlameCli.obl $(OUT_DIR)/FlameCli.json $(OUT_DIR)/FlameTypes.json
	$(COMPILER) $< --emit-c -S $(OUT_DIR) -o .
	touch $@

$(OUT_DIR)/FlameRenderer.transpile.stamp: $(SRC_DIR)/FlameRenderer.obl $(OUT_DIR)/FlameRenderer.json $(OUT_DIR)/FlameTypes.json
	$(COMPILER) $< --emit-c -S $(OUT_DIR) -o .
	touch $@

$(OUT_DIR)/FractalFlame.transpile.stamp: $(SRC_DIR)/FractalFlame.obl $(OUT_DIR)/FractalFlame.json $(OUT_DIR)/FlameCli.json $(OUT_DIR)/FlameRenderer.json $(OUT_DIR)/FlameTypes.json
	$(COMPILER) $< --emit-c --main -S $(OUT_DIR) -o .
	touch $@

FlameTypes.c FlameTypes.h: $(OUT_DIR)/FlameTypes.transpile.stamp
FlameCli.c FlameCli.h: $(OUT_DIR)/FlameCli.transpile.stamp
FlameRenderer.c FlameRenderer.h: $(OUT_DIR)/FlameRenderer.transpile.stamp
FractalFlame.c FractalFlame.h: $(OUT_DIR)/FractalFlame.transpile.stamp

$(OUT_DIR)/stdlib/%.o: $(STDLIB)/%.c $(STDLIB)/%.h | $(OUT_DIR)/stdlib
	$(CC) $(STD_CFLAGS) -c $< -o $@

$(OUT_DIR)/FlameTypes.o: FlameTypes.c FlameTypes.h | $(OUT_DIR)
	$(CC) $(APP_CFLAGS) -c $< -o $@

$(OUT_DIR)/FlameCli.o: FlameCli.c FlameCli.h FlameTypes.h | $(OUT_DIR)
	$(CC) $(APP_CFLAGS) -c $< -o $@

$(OUT_DIR)/FlameRenderer.o: FlameRenderer.c FlameRenderer.h FlameTypes.h | $(OUT_DIR)
	$(CC) $(APP_CFLAGS) -c $< -o $@

$(OUT_DIR)/FractalFlame.o: FractalFlame.c FractalFlame.h FlameCli.h FlameRenderer.h FlameTypes.h | $(OUT_DIR)
	$(CC) $(APP_CFLAGS) -c $< -o $@

$(OUT_DIR) $(OUT_DIR)/stdlib:
	mkdir -p $@

clean:
	rm -rf $(OUT_DIR)

rebuild: clean all

run: $(BIN)
	$(BIN) -w 800 -h 600 -i 10000000 -t 8 -o preview.png -ap 0.635,-0.932,-0.848,0.767,1.174,-0.533/1.002,0.499,-0.574,-0.248,-0.705,-0.036/-0.217,1.083,1.018,-1.131,-0.523,-0.428/-0.839,0.33,0.511,-0.46,-0.904,-0.97 -f spherical:0.74 --gamma 0.64 -g -s 2

.PHONY: all symbols transpile clean rebuild run
