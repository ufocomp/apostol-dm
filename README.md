# BitDeals Payment Service: Deal Module

**Модуль сделок** системы учёта Bitcoin платежей, исходные коды на C++.

Построено на базе [Апостол](https://github.com/ufocomp/apostol).

СТРУКТУРА КАТАЛОГОВ
-
    auto/               содержит файлы со скриптами
    cmake-modules/      содержит файлы с модулями CMake
    conf/               содержит файлы с настройками
    doc/                содержит файлы с документацией
    ├─www/              содержит файлы с документацией в формате html
    src/                содержит файлы с исходным кодом
    ├─app/              содержит файлы с исходным кодом: BitDeals Payment Service (Deal Module)
    ├─core/             содержит файлы с исходным кодом: Apostol Core
    ├─lib/              содержит файлы с исходным кодом библиотек
    | └─delphi/         содержит файлы с исходным кодом библиотеки*: Delphi classes for C++
    └─modules/          содержит файлы с исходным кодом дополнений (модулей)
      └─WebService/     содержит файлы с исходным кодом дополнения: Web-сервис

ОПИСАНИЕ
-

**Модуля сделок** (dm) предоставляет интерфейсы для создания и изменения учётных записей пользователей и сделок системы учёта Bitcoin платежей.

REST API
-

[Документация по REST API](./doc/REST-API-ru.md)

Протестировать API **Модуля сделок** можно с помощью встроенного Web-сервера доступного по адресу: [localhost:4999](http://localhost:4999)

##### Авторизация:
    username: module
    password: <address> 

Где `<address>` - Bitcoin адрес модуля сделок.

СБОРКА И УСТАНОВКА
-
Для установки **Модуля сделок** Вам потребуется:

1. Компилятор C++;
1. [CMake](https://cmake.org) или интегрированная среда разработки (IDE) с поддержкой [CMake](https://cmake.org);
1. Библиотека [libbitcoin-explorer](https://github.com/libbitcoin/libbitcoin-explorer/tree/version3) (Bitcoin Cross-Platform C++ Development Toolkit);
1. Библиотека [OpenPGP](https://github.com/calccrypto/OpenPGP) (OpenPGP in C++)
1. Библиотека [yaml-cpp](https://github.com/jbeder/yaml-cpp) (YAML parser and emitter in C++)

### Linux (Debian/Ubuntu)

Для того чтобы установить компилятор C++ и необходимые библиотеки на Ubuntu выполните:
~~~
$ sudo apt-get install build-essential libssl-dev libcurl4-openssl-dev make cmake gcc g++
~~~

###### Подробное описание установки C++, CMake, IDE и иных компонентов необходимых для сборки проекта не входит в данное руководство. 

### libbitcoin

Для того чтобы установить libbitcoin выполните:
~~~
$ sudo apt-get install autoconf automake libtool pkg-config git
~~~
~~~
$ wget https://raw.githubusercontent.com/libbitcoin/libbitcoin-explorer/version3/install.sh
$ chmod +x install.sh
~~~
~~~
$ ./install.sh --build-boost --build-zmq --disable-shared
~~~

#### OpenPGP

Для сборки **OpenPGP** обратитесь к документации на сайте [OpenPGP](https://github.com/calccrypto/OpenPGP).

##### CMake Configuration
~~~
GPG_COMPATIBLE=ON
USE_OPENSSL=ON
~~~

###### Обратите внимание на зависимости OpenPGP от других библиотек (GMP, bzip2, zlib, OpenSSL).

#### yaml-cpp

Для сборки **yaml-cpp** обратитесь к документации на сайте [yaml-cpp](https://github.com/jbeder/yaml-cpp).

#### BitDeals Payment Service (Deal Module)

Для того чтобы установить **Модуля сделок** с помощью Git выполните:
~~~
$ git clone git@github.com:ufocomp/apostol-dm.git dm
~~~
Далее:
1. Настроить `CMakeLists.txt` (по необходимости);
1. Собрать и скомпилировать (см. ниже).

Для того чтобы установить **Модуля сделок** (без Git) необходимо:

1. Скачать **BPS (DM)** по [ссылке](https://github.com/ufocomp/apostol-dm/archive/master.zip);
1. Распаковать;
1. Настроить `CMakeLists.txt` (по необходимости);
1. Собрать и скомпилировать (см. ниже).

###### Сборка:
~~~
$ cd dm
$ ./configure
~~~

###### Компиляция и установка:
~~~
$ cd cmake-build-release
$ make
$ sudo make install
~~~

По умолчанию **Модуль сделок** будет установлен в:
~~~
/usr/sbin
~~~

Файл конфигурации и необходимые для работы файлы, в зависимости от варианта установки, будут расположены в: 
~~~
/etc/dm
или
~/dm
~~~

ЗАПУСК 
-
###### Если `INSTALL_AS_ROOT` установлено в `ON`.

**`dm`** - это системная служба (демон) Linux. 
Для управления **`dm`** используйте стандартные команды управления службами.

Для запуска Апостол выполните:
~~~
$ sudo service dm start
~~~

Для проверки статуса выполните:
~~~
$ sudo service dm status
~~~

Результат должен быть **примерно** таким:
~~~
● dm.service - LSB: starts the Deal Module
   Loaded: loaded (/etc/init.d/dm; generated; vendor preset: enabled)
   Active: active (running) since Thu 2019-08-15 14:11:34 BST; 1h 1min ago
     Docs: man:systemd-sysv-generator(8)
  Process: 16465 ExecStop=/etc/init.d/dm stop (code=exited, status=0/SUCCESS)
  Process: 16509 ExecStart=/etc/init.d/dm start (code=exited, status=0/SUCCESS)
    Tasks: 3 (limit: 4915)
   CGroup: /system.slice/dm.service
           ├─16520 dm: master process /usr/sbin/abc
           └─16521 dm: worker process
~~~

### **Управление dm**.

Управлять **`dm`** можно с помощью сигналов.
Номер главного процесса по умолчанию записывается в файл `/run/dm.pid`. 
Изменить имя этого файла можно при конфигурации сборки или же в `dm.conf` секция `[daemon]` ключ `pid`. 

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
