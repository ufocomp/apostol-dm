# deal-module

**Deal Module** - Модуль сделок, исходные коды на C++.

СТРУКТУРА КАТАЛОГОВ
-

    auto/               содержит файлы со скриптами
    cmake-modules/      содержит файлы с модулями CMake
    conf/               содержит файлы с настройками
    doc/                содержит файлы с документацией
    ├─www/              содержит файлы с документацией в формате html
    src/                содержит файлы с исходным кодом
    ├─bps-dm/           содержит файлы с исходным кодом: Bitcoin Payment Service (Deal Module)
    ├─core/             содержит файлы с исходным кодом: Apostol Core
    ├─lib/              содержит файлы с исходным кодом библиотек
    | └─delphi/         содержит файлы с исходным кодом библиотеки*: Delphi classes for C++
    └─modules/          содержит файлы с исходным кодом дополнений (модулей)
      └─WebService/     содержит файлы с исходным кодом дополнения: Web-сервис

ОПИСАНИЕ
-

**Bitcoin Payment Service (Deal Module)** (bps-dm) - Модуль сделок сервиса обработки и учёта платежей Bitcoin. Построен на базе [Апостол](https://github.com/ufocomp/apostol).

СБОРКА И УСТАНОВКА
-
Для установки **Модуля сделок** Вам потребуется:

1. Компилятор C++;
1. [CMake](https://cmake.org) или интегрированная среда разработки (IDE) с поддержкой [CMake](https://cmake.org);
1. Библиотека [libdelphi](https://github.com/ufocomp/libdelphi/) (Delphi classes for C++);

###### **ВНИМАНИЕ**: Устанавливать `libdelphi` не нужно, достаточно скачать и разместить в каталоге `src/lib` проекта.

Для того чтобы установить компилятор C++ и необходимые библиотеки на Ubuntu выполните:
~~~
sudo apt-get install build-essential libssl-dev libcurl4-openssl-dev make cmake gcc g++
~~~

###### Подробное описание установки C++, CMake, IDE и иных компонентов необходимых для сборки проекта не входит в данное руководство. 

Для установки **Модуля сделок** (без Git) необходимо:

1. Скачать **Модуль сделок** по [ссылке](https://github.com/ufocomp/deal-module/archive/master.zip);
1. Распаковать;
1. Скачать **libdelphi** по [ссылке](https://github.com/ufocomp/libdelphi/archive/master.zip);
1. Распаковать в `src/lib/delphi`;
1. Настроить `CMakeLists.txt` (по необходимости);
1. Собрать и скомпилировать (см. ниже).

Для установки **Модуля сделок** с помощью Git выполните:
~~~
git clone https://github.com/ufocomp/deal-module.git
~~~

Чтобы добавить **libdelphi** в проект с помощью Git выполните:
~~~
cd deal-module/src/lib
git clone https://github.com/ufocomp/libdelphi.git delphi
cd ../../../
~~~

###### Сборка:
~~~
cd deal-module
cmake -DCMAKE_BUILD_TYPE=Release . -B cmake-build-release
~~~

###### Компиляция и установка:
~~~
cd cmake-build-release
make
sudo make install
~~~

По умолчанию **Модуль сделок** будет установлен в:
~~~
/usr/sbin
~~~

Файл конфигурации и необходимые для работы файлы будут расположены в: 
~~~
/etc/bps-dm
~~~

ЗАПУСК
-

**`bps-dm`** - это системная служба (демон) Linux. 
Для управления **`bps-dm`** используйте стандартные команды управления службами.

Для запуска Апостол выполните:
~~~
sudo service bps-dm start
~~~

Для проверки статуса выполните:
~~~
sudo service bps-dm status
~~~

Результат должен быть **примерно** таким:
~~~
● bps-dm.service - LSB: starts the Deal Module
   Loaded: loaded (/etc/init.d/bps-dm; generated; vendor preset: enabled)
   Active: active (running) since Thu 2019-08-15 14:11:34 BST; 1h 1min ago
     Docs: man:systemd-sysv-generator(8)
  Process: 16465 ExecStop=/etc/init.d/bps-dm stop (code=exited, status=0/SUCCESS)
  Process: 16509 ExecStart=/etc/init.d/bps-dm start (code=exited, status=0/SUCCESS)
    Tasks: 3 (limit: 4915)
   CGroup: /system.slice/bps-dm.service
           ├─16520 bps-dm: master process /usr/sbin/abc
           └─16521 bps-dm: worker process
~~~

### **Управление bps-dm**.

Управлять **`bps-dm`** можно с помощью сигналов.
Номер главного процесса по умолчанию записывается в файл `/usr/local/bps-dm/logs/bps-dm.pid`. 
Изменить имя этого файла можно при конфигурации сборки или же в `bps-dm.conf` секция `[daemon]` ключ `pid`. 

Главный процесс поддерживает следующие сигналы:

|Сигнал   |Действие          |
|---------|------------------|
|TERM, INT|быстрое завершение|
|QUIT     |плавное завершение|
|HUP	  |изменение конфигурации, запуск новых рабочих процессов с новой конфигурацией, плавное завершение старых рабочих процессов|
|WINCH    |плавное завершение рабочих процессов|	

Управлять рабочими процессами по отдельности не нужно. Тем не менее, они тоже поддерживают некоторые сигналы:

|Сигнал   |Действие          |
|---------|------------------|
|TERM, INT|быстрое завершение|
|QUIT	  |плавное завершение|
