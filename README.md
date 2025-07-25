# logger-lib

Универсальная библиотека для логирования на C++: поддерживает запись в файл и отправку по TCP-сокету, гарантирует потокобезопасность и предоставляет три уровня логирования.

## Особенности

- **Два режима вывода**:
    - В файл
    - По TCP-сокету
- **Три уровня логирования**: `DEBUG`, `INFO`, `ERROR`
- **Временные метки в формате**: `YYYY-MM-DD HH:MM:SS`
- **Потокобезопасность**: все методы защищены мьютексами
- **Удобная настройка** уровня логирования в рантайме

## Установка и использование

### Установка в систему

1. Склонируйте данный репозиторий при помощи `git clone`.
2. Создайте папку `build` и войдите в неё: `mkdir build && cd build`.
3. Введите команду сборки для cmake. Флаги сборки:
    - `LOGGERLIB_SHARED_LIBS` определяет статическую/динамическую сборку библиотеки (по умолчанию не определён).
    - `LOGGERLIB_BUILD_TESTS` включает/выключает сборку тестов (тестирование происходит с помощью собственной библиотеки `mytest`), по умолчанию `OFF`.
    - `LOGGERLIB_BUILD_EXAMPLES` включает/выключает сборку примеров (см. Примеры), по умолчанию `OFF`.
    - `LOGGERLIB_INSTALL` включает/выключает установку библиотеки в систему, по умолчанию `OFF`.
4. Введите команду `cmake --build .`. Она выполнит установку и сборку необходимых компонентов.

### Использование как подпроекта

1. Склонируйте данный репозиторий при помощи `git clone` в ваш проект.
2. В `CMakeLists.txt` вашего проекта:
    ```cmake
    add_subdirectory(path/to/logger-lib)
    target_link_libraries(your-app PRIVATE loggerlib)
    ```
3. Подключите заголовочный файл:
    ```cpp
    #include <loggerlib/logger.hpp>
    ```

## Тестирование

При сборке установите флаг `LOGGERLIB_BUILD_TESTS` в положение `ON`, затем запустите файл `./tests/loggerlib-tests/`. Перед использованием библиотеки настоятельно рекомендуется проверить, что все тесты запускаются и проходят на вашем устройстве.

## Примеры использования

При сборке установите флаг `LOGGERLIB_BUILD_EXAMPLES` в положение `ON`.

### logger-test-app

1. Запустите `./examples/logger-test-app/logger-test-app <filename> [default level]`, где `<filename>` - имя файла, `[default level]` - уровень логирования по умолчанию (необязательный параметр).
2. Следуйте инструкциям для ввода логируемых сообщений
3. Введённые сообщения с соответствующими уровнями отобразятся в корректном формате в файле `filename`.

### logger-stats-app

1. Запустите `./examples/logger-stats-app/logger-stats-app <host> <port> <T> <N>`, где `<host>:<port>` - сокет, из которого приходят логи, `<T>` - промежуток времени, через который будет выводиться статистика (в случае изменений), `<N>` - количество сообщений, по достижении которого будет выводиться статистика.
2. Попробуйте отправить несколько залогированных строчек с помощью `netcat`, пример:
    ```bash
    nc <host> <port> < "[2025-07-23 14:51:49] INFO:  info message"
    ```
3. В консоли, где запущено `logger-stats-app` отобразится сообщение, а также по достижении `N` сообщений либо `T` секунд выведется статистика.

## API

### enum class LogLevel
 - `DEBUG` - подробные логи
 - `INFO` - информационные логи
 - `ERROR` - ошибки

### Класс Logger

#### Конструкторы
- Файловый
    ```cpp
    Logger(const std::string& filename, Loglevel level);
    ```
    - Открывает `filename` в режиме `append`.
    - Бросает `std::runtime_error` если открыть файл не удалось.
- Сетевой
    ```cppp
    Logger(const std::string& host, int port, LogLevel level);
    ```
    - Создаёт TCP-сокет и подключается.
    - Бросает `std::runtime_error` в случае неудачи при разрешении адреса, создании сокета или подключении.

### Метод log()
```cpp
void log(const std::string& message, LogLevel level);
```
- Игнорирует вызовы, если уровень сообщения ниже текущего уровня логирования по умолчанию
- Формирует строку:
    ```
    [YYYY-MM-DD HH:MM:SS] LEVEL: message\n
    ```
- Записывает в файл или шлёт по сокету.
### get_level/set_level
```cpp
void set_level(LogLevel level);
LogLevel get_level() const;
```
- В рантайме меняет порог минимального уровня логов.
### get_current_timestamp
```cpp
std::string get_current_timestamp();
```
- Возвращает строку с текущим локальным временем в формате `YYYY-MM-DD HH:MM:SS`.

## Заключение

Весь код вышеописанной библиотеки, а также данную краткую документацию написал Михаловский Михаил Михайлович, студент программы бакалавриата "Прикладная математика и информатика" Школы Физики, Информатики и Технологий НИУ ВШЭ (Санкт-Петербург) в качестве тестового задания для компании Инфотекс.
