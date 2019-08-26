Apostol Bitcoin Module
=

**Apostol Bitcoin Module** - Web модуль сервиса обработки **Bitcoin** платежей, исходные коды на C++.

СТРУКТУРА КАТАЛОГОВ
-

    auto/               содержит файлы со скриптами
    cmake-modules/      содержит файлы с модулями CMake
    conf/               содержит файлы с настройками
    doc/                содержит файлы с документацией
    ├─www/              содержит файлы с документацией в формате html
    src/                содержит файлы с исходным кодом
    ├─abm/              содержит файлы с исходным кодом: Apostol Bitcoin Module
    ├─core/             содержит файлы с исходным кодом: Apostol Core
    ├─lib/              содержит файлы с исходным кодом библиотек
    | └─delphi/         содержит файлы с исходным кодом библиотеки: Delphi classes for C++
    └─modules/          содержит файлы с исходным кодом дополнений (модулей)
      └─BitTrade/       содержит файлы с исходным кодом дополнения: Модуль сделок

ОПИСАНИЕ
-

**Apostol Bitcoin Module** (abm) - Web модуль сервиса обработки **Bitcoin** платежей построен на базе [Апостол](https://github.com/ufocomp/apostol).

СБОРКА И УСТАНОВКА
-
Для сборки проекта Вам потребуется:

1. Компилятор C++;
1. [CMake](https://cmake.org) или интегрированная среда разработки (IDE) с поддержкой [CMake](https://cmake.org);

Для того чтобы установить компилятор C++ и необходимые библиотеки на Ubuntu выполните:
~~~
sudo apt-get install build-essential libssl-dev libcurl4-openssl-dev make cmake gcc g++
~~~

###### Подробное описание установки C++, CMake, IDE и иных компонентов необходимых для сборки проекта не входит в данное руководство. 

Для сборки **Апостол Bitcoin Module**, необходимо:

1. Скачать **Апостол Bitcoin Module** по [ссылке](https://github.com/ufocomp/abc-module/archive/master.zip);
1. Распаковать;
1. Скомпилировать (см. ниже).

Для сборки **Апостол Bitcoin Module**, с помощью Git выполните:
~~~
git clone https://github.com/ufocomp/abc-module.git
~~~

###### Сборка:
~~~
cd abc-module
cmake -DCMAKE_BUILD_TYPE=Release . -B cmake-build-release
~~~

###### Компиляция и установка:
~~~
cd cmake-build-release
make
sudo make install
~~~

По умолчанию **Apostol Bitcoin Module** будет установлен в:
~~~
/usr/sbin
~~~

Файл конфигурации и необходимые для работы файлы будут расположены в: 
~~~
/etc/abm
~~~

ЗАПУСК
-

Apostol Bitcoin Module - это системная служба (демон) Linux. 
Для управления Апостол Bitcoin Module используйте стандартные команды управления службами.

Для запуска Апостол выполните:
~~~
sudo service abm start
~~~

Для проверки статуса выполните:
~~~
sudo service abm status
~~~

Результат должен быть **примерно** таким:
~~~
● abm.service - LSB: starts the abc-module
   Loaded: loaded (/etc/init.d/abm; generated; vendor preset: enabled)
   Active: active (running) since Thu 2019-08-15 14:11:34 BST; 1h 1min ago
     Docs: man:systemd-sysv-generator(8)
  Process: 16465 ExecStop=/etc/init.d/abm stop (code=exited, status=0/SUCCESS)
  Process: 16509 ExecStart=/etc/init.d/abm start (code=exited, status=0/SUCCESS)
    Tasks: 3 (limit: 4915)
   CGroup: /system.slice/abm.service
           ├─16520 abm: master process /usr/sbin/abc
           └─16521 abm: worker process
~~~

### **Управление abm**.

Управлять **`abm`** можно с помощью сигналов.
Номер главного процесса по умолчанию записывается в файл `/usr/local/abm/logs/abm.pid`. 
Изменить имя этого файла можно при конфигурации сборки или же в `abm.conf` секция `[daemon]` ключ `pid`. 

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
