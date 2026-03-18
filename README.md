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
./build/obould -t tests/sources/comprehensive_test.obl
```

Построить и вывести AST:

```bash
./build/obould -a tests/sources/comprehensive_test.obl
```

Сгенерировать и вывести LLVM IR:

```bash
./build/obould -l tests/sources/comprehensive_test.obl
```

Сгенерировать объектный файл

```bash
./build/obould tests/sources/comprehensive_test.obl
```

Компиляция и запуск демонстрационных программ

```bash
./tests/run_tests.sh
```
