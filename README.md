# Obould-lang

Компилятор языка программирования Обольд — курсовой проект студентов 3-го курса ПИ Петрова Константина и Чубенко Семёна.

Обольд — статически типизированный модульный язык, повторяющий семантику языка Оберон-07, но имеющий C-подобный синтаксис.

### [Описание языка](docs/spec/obould.pdf)

## Требования

- CMake 3.14+
- LLVM 15+
- Компилятор с поддержкой C++17

## Сборка

```bash
cmake -S . -B build && cmake --build build -j
```

Если LLVM установлен нестандартно:

```bash
cmake -DLLVM_DIR="/path/to/llvm-x/lib/cmake/llvm" -S . -B build && cmake --build build -j
```

## Быстрый старт

Скомпилировать `.obl` файл в исполняемый и запустить:

```bash
./build/obould my_program.obl -m --link -o my_program
./my_program
```

Флаг `-m`/`--main` — главный модуль (с функцией `init`).

## Многомодульный проект

Если программа состоит из нескольких модулей, нужно собрать каждый модуль отдельно, затем скомпоновать.

```bash
./build/obould Lib.obl -o out/Lib.o
./build/obould Main.obl --main --link -S out --obj out/Lib.o -o app
./app
```

## Другие режимы

```bash
./build/obould --help           # справка
./build/obould prog.obl -t      # вывести токены
./build/obould prog.obl -a      # вывести AST
./build/obould prog.obl -l      # вывести LLVM IR
```

## Тесты

```bash
# Сборка с тестами
cmake -DO_TESTS=ON -S . -B build && cmake --build build -j
ctest --test-dir build
```

## Транспиляция в Си

```bash
./build/obould prog.obl -c -o out/ --main   # .h / .c с main()
cc -o prog out/Prog.c out/Out.c && ./prog   # компиляция и запуск
```

Компилятор может генерировать пару `.h` / `.c` вместо машинного кода.


### Генерация C-кода

```bash
# Вывести заголовок и исходник в stdout
./build/obould -c tests/sources/test.obl

# Сгенерировать файлы в указанную директорию
./build/obould -c tests/sources/test.obl -o out/

# Сгенерировать с точкой входа main() (для главного модуля)
./build/obould -c tests/sources/test.obl -o out/ --main
```

Флаг `--main` (`-m`) добавляет в `.c` файл функцию `main()`, которая вызывает `init()` модуля.

### Компиляция и запуск одномодульной программы

```bash
# 1. Транспилировать
./build/obould -c my_program.obl -o out/ --main

# 2. Скопировать stdlib (если модуль использует import Out)
cp stdlib/Out.h stdlib/Out.c out/

# 3. Скомпилировать и запустить
cc -o out/my_program out/MyProgram.c out/Out.c
./out/my_program
```

### Многомодульный проект

Допустим, есть два файла: библиотечный модуль `MathLib.obl` и главный модуль `Main.obl`, который его импортирует.

**Пошаговая сборка:**

```bash
# 1. Создать выходную директорию
mkdir -p out

# 2. Скопировать стандартную библиотеку на примере модуля Out
cp stdlib/Out.h stdlib/Out.c stdlib/Out.json out/
mkdir -p out/.obould
cp stdlib/Out.json out/.obould/

# 3. Сгенерировать символьный файл для библиотечного модуля.
cd out
../build/obould ../MathLib.obl --emit-symbols

# 4. Транспилировать библиотечный модуль (без --main)
../build/obould ../MathLib.obl --emit-c -o .

# 5. Транспилировать главный модуль (с --main)
../build/obould ../Main.obl --emit-c -o . --main

# 6. Скомпилировать все .c файлы вместе
cc -o main Main.c MathLib.c Out.c

# 7. Запустить
./main
```
