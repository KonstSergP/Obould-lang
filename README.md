# Obould-lang

Этот репозиторий содержит курсовой проект студентов 3 курса ПИ Петрова Константина и Чубенко Семёна -  компилятор языка
программирования Обольд, состоящий из лексера, парсера, абстрактного синтаксического дерева, семантического анализа, а также генерации кода через LLVM IR и через язык Си.
Обольд — статически типизированный модульный язык, повторяющий семантику языка Оберон-07, но имеющий C-подобный синтаксис. Основная идея проекта связана с проверкой восприятия данного языка современными программистами.

### [Описание языка](docs/spec/obould.pdf)

## Требования

- CMake 3.14+
- Компилятор с поддержкой C++17
- LLVM 15+

## Сборка

```bash
cmake -S . -B build
cmake --build build -j
```

Если путь до llvm другой:

```bash
cmake -DLLVM_DIR="/home/user/путь/к/llvm-x/lib/cmake/llvm" -S . -B build
cmake --build build -j
```


## Запуск

```bash
./build/obould --help
```

Вывести токены:

```bash
./build/obould -t tests/test.obl
```

Построить и вывести AST:

```bash
./build/obould -a tests/test.obl
```

Сгенерировать и вывести LLVM IR:

```bash
./build/obould -l tests/test.obl
```

## Транспилятор в Си

Компилятор может транспилировать исходники Обольда в пару `.h` / `.c` файлов.

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

### Компиляция и запуск транспилированного кода

```bash
# 1. Транспилировать
./build/obould -c my_program.obl -o out/ --main

# 2. Скопировать stdlib (если модуль использует import Out)
cp stdlib/Out.h stdlib/Out.c out/

# 3. Скомпилировать и запустить
cc -o out/my_program out/MyProgram.c out/Out.c
./out/my_program
```

### Именование символов

Транспилятор использует манглирование имён формата `ob_{длина_модуля}{Модуль}_{Имя}`. Например, для модуля `Out` функция `WriteLn` получает имя `ob_3Out_WriteLn`.

## Тесты

### Сборка тестов

```bash
cmake -DO_TESTS=ON -S . -B build
cmake --build build
```

### Варианты запуск тестов

```bash
# 1. Все тесты через ctest
ctest --test-dir build

# 2. Только blackbox-тесты транспилятора
ctest --test-dir build -R blackbox

# 3. Напрямую
./build/blackbox_ccodegen_tests
