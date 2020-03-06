# API "Модуля сделок" системы учёта Bitcoin платежей.

## Общая информация
 * Базовая конечная точка (endpoint): [localhost:40977](http://localhost:40977)
 * Все конечные точки возвращают `JSON-объект`
 * Все поля, относящиеся ко времени и меткам времени, указаны в **миллисекундах**. 

## HTTP коды возврата
 * HTTP `4XX` коды возврата применимы для некорректных запросов - проблема на стороне клиента.
 * HTTP `5XX` коды возврата используются для внутренних ошибок - проблема на стороне сервера. Важно **НЕ** рассматривать это как операцию сбоя. Статус выполнения **НЕИЗВЕСТЕН** и может быть успешным.
 
## Коды ошибок
 * Любая конечная точка может вернуть ошибку.
  
**Пример ответа:**
```json
{
  "error": {
    "code": 404,
    "message": "Not Found"
  }
}
```

## Общая информация о конечных точках
 * Для `GET` конечных точек параметры должны быть отправлены в виде `строки запроса (query string)` .
 * Для `POST` конечных точек, некоторые параметры могут быть отправлены в виде `строки запроса (query string)`, а некоторые в виде `тела запроса (request body)`:
 * При отправке параметров в виде `тела запроса` допустимы следующие типы контента:
    * `application/x-www-form-urlencoded` для `query string`;
    * `multipart/form-data` для `HTML-форм`;
    * `application/json` для `JSON`
 * Параметры могут быть отправлены в любом порядке.
 
## Общие конечные точки
### Тест подключения
 
```http request
GET /api/v1/ping
```
Проверить подключение к REST API.
 
**Параметры:**
НЕТ
 
**Пример ответа:**
```json
{}
```
 
### Проверить время сервера
```http request
GET /api/v1/time
```
Проверить подключение к REST API и получить текущее время сервера.
 
**Параметры:**
 НЕТ
  
**Пример ответа:**
```json
{
  "serverTime": 1583495795455
}
```
 
## Конечные точки
 
Конечные точки разделены на две категории: `Пользователи (user)` и `Сделки (deal)`. 

Обработка запросов выполняется на стороне `Дополнительного сервера (ДС)`. 
Поэтому `Модуль сделок` формирует **транспортный пакет** в виде `JSON-объекта` и отправляет его на `ДС`.

##### ВАЖНО: Сетевой адрес `Дополнительного сервера (ДС)` можно указать в параметрах запроса в виде `строки запроса`. По умолчанию: `localhost`

### Формат транспортного пакета:
**Запрос:**
```json
{
  "id": "<string>",
  "address": "<string>"
} 
```
**Ответ:**
```json
{
  "id": "<string>",
  "action": "<string>",
  "result": {
    "success": "<boolean>",
    "message": "<string>"
  },
  "payload": "<base64>"
} 
```

**Описание:**

Ключ | Тип | Значение | Описание
------------ | ------------ | ------------ | ------------
id | STRING | | Уникальный идентификатор запроса 
address | STRING | | Bitcoin адрес пользователя (учётной записи) `ДС`. По умолчанию: Адрес модуля
action | STRING | help, status, new, add, update, delete | Действие 
result | JSON | | Результат выполнения запроса
success | BOOLEAN | true, false | Успешно: Да/Нет 
message | STRING | | Сообщение (информация об ошибке) 
payload | BASE64 | | Полезная нагрузка (строка в формате Base64) 

###### ВНИМАНИЕ: Данные в поле `payload` зашифрованы по алгоритму Base64. 

После расшифровки `полезной нагрузки` конечным значением может быть: 
 * `Текст в произвольном формате`; 
 * `HTML-документ`; 
 * `JSON-объект`.  

### Общие параметры

#### Для последующих конечных точек можно указать **параметры** в виде `строки запроса (query string)`:

Имя | Тип | Значение | Описание
------------ | ------------ | ------------ | ------------
payload | STRING | text, html, json, xml | Получить ответ в виде содержимого поля `payload` с указанием формата 
server | STRING | localhost | IP-адрес или Host имя сервера (ДС). По умолчанию: localhost 
pgp | BOOLEAN | false, off | Отключить PGP подпись модуля сделок 
address | STRING |  | Bitcoin адрес пользователя (учётной записи) `ДС`. По умолчанию: Адрес модуля

## Параметры пользователя

### Параметры в виде `тела запроса (request body)`:

Наименование | Тип | Действие | Описание
------------ | ------------ | ------------ | ------------
address | STRING | * | Bitcoin адрес     
bitmessage | STRING | New | Bitmessage адрес    
key | STRING | New, Add | Bitcoin ключ (публичный)
pgp | STRING | New, Add | PGP ключ (публичный)
url | STRING | New, Update | Список URL   
date | STRING | Update, Delete | Дата в формате: YYYY-MM-DD     
sign | STRING | Update, Delete | Bitcoin подпись 
flags | STRING | Delete | Флаги

## Параметры сделки

### Параметры в виде `тела запроса (request body)`:
#### Для типов контента `application/x-www-form-urlencoded` и `multipart/form-data`:

Наименование | Тип | Действие | Описание
------------ | ------------ | ------------ | ------------
at | URL | * | URL сайта
date | DATETIME | * | Дата сделки     
seller_address | STRING | * | Продавец: Bitcoin адрес     
seller_rating | STRING  | Pay, Complete | Продавец: Рейтинг
customer_address | STRING | * | Покупатель: Bitcoin адрес
customer_rating | STRING | Pay, Complete | Покупатель: Рейтинг    
payment_address | STRING | Pay, Complete | Оплата: Bitcoin адрес
payment_until | DATETIME | Pay, Complete | Оплата: Срок оплаты
payment_sum | DOUBLE | * | Оплата: Сумма
feedback_leave_before | DATETIME | Pay, Complete | Обратная связь: Срок до... 
feedback_status | STRING | Complete | Обратная связь: Статус
feedback_comments | STRING | Complete | Обратная связь: Комментарий

#### Для типа контента `application/json`:

```json
{
  "at": "<url>",
  "date": "<datetime>",
  "seller": {
    "address": "<string>",
    "rating": "<string>"
  },
  "customer": {
    "address": "<string>",
    "rating": "<string>"
  },
  "payment": {
    "address": "<string>",
    "until": "<datetime>",
    "sum": "<double>"
  },
  "feedback": {
    "leave-before": "<datetime>",
    "status": "<string>",
    "comments": "<string>"
  }
} 
```

#### Для типа контента `text/plain`:

```yaml
deal:
  order: <string>
  at: <url>
  date: <datetime>
  seller:
    address: <string>
    rating: <string>
  customer:
    address: <string>
    rating: <string>
  payment:
    sum: <double>
    address: <string>
    until: <datetime>
  feedback:
    leave-before: <datetime>
} 
```

### Пользователь

#### Помощь  
```http request
GET /api/v1/user/help
```
```http request
POST /api/v1/user/help
```
Получить справочную информацию по командам регистрации и обновления данных учётной записи.
 
**Параметры:**
[Общие параметры](#общие-параметры)
  
**Пример:**

Запрос:
```http request
GET /api/v1/user/help?payload=html
```

Ответ (содержимое `payload`):
```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="author" content="Bitcoin Payment Service">
</head>
<body>
<pre>
Bitcoin Payment Service.

Usage: Send a message in accordance with the format.

Available actions: 

  <b>help</b>         : send this help
  <b>status</b>       : show user account data
  <b>new</b>          : create user account
  <b>add</b>          : adding data to a user account (adding missing data)
  <b>update</b>       : updating user account data
  <b>delete</b>       : delete user account

The system has an AI and can determine the action itself.
But you can help the AI and indicate the action explicitly in the message subject.

Subject: [help|status|new|add|update|delete] | any text
Body:
Action         : Message format
  <b>help</b>         : &lt;ignored&gt;
  <b>status</b>       : &lt;ignored&gt;
  <b>new</b>          : &lt;bitcoin&gt;&lt;LF&gt;
                 [&lt;key&gt;&lt;LF&gt;] |
                 [&lt;PGP&gt;&lt;LF&gt;] |
                 [{account:|trusted:}&lt;LF&gt;]
                 [&lt;url&gt;&lt;LF&gt;]
  <b>add</b>          : [&lt;key&gt;&lt;LF&gt;] |
                 [&lt;PGP&gt;&lt;LF&gt;]
  <b>update</b>       : &lt;yyyy-mm-dd&gt;&lt;LF&gt;
                 [&lt;PGP&gt;&lt;LF&gt;] |
                 [{account:|trusted:}&lt;LF&gt;]
                 [&lt;url&gt;&lt;LF&gt;] |
                 &lt;signature&gt;
  <b>delete</b>       : &lt;yyyy-mm-dd&gt;&lt;LF&gt;
                 -account&lt;LF&gt;
                 &lt;signature&gt;
Arguments:
  account      : account URL list (default if not set "trusted")
  trusted      : trusted URL list
  -account     : delete account

Templates:
  &lt;ignored&gt;    : any text will be ignored
  &lt;LF&gt;         : line feed; format: 0x0a
  &lt;string&gt;     : single line text
  &lt;bitcoin&gt;    : bitcoin address; format: [bitcoin:]&lt;string&gt;
  &lt;key&gt;        : bitcoin public key
  &lt;PGP&gt;        : PGP public key
  &lt;yyyy-mm-dd&gt; : current date
  [+|-]&lt;url&gt;   : add(+) or delete(-) URL; format: http[s]://&lt;string&gt;
  &lt;signature&gt;  : bitcoin signature for message text (use private key)</pre>

</body>
</html>
```
#### Статус
```http request
GET /api/v1/user/status
```
```http request
POST /api/v1/user/status
```
Получить статус учётной записи пользователя.
 
**Параметры:**
[Общие параметры](#общие-параметры)

**Пример:**

Запрос:
```http request
GET /api/v1/user/status?address=null
```

Ответ:
```json
{
"id": "A4406-PDF10-OE2BC-SF8AA-T918F-OC9BC-L9EB00",
"action": "Status",
"result": {
"success": false,
"message": "Invalid Bitcoin address: null"
},
"payload": "PCFET0NUWVBFIGh0bWw+CjxodG1sIGxhbmc9ImVuIj4KPGhlYWQ+CiAgICA8bWV0YSBjaGFyc2V0PSJVVEYtOCI+CiAgICA8bWV0YSBuYW1lPSJhdXRob3IiIGNvbnRlbnQ9IkJpdGNvaW4gUGF5bWVudCBTZXJ2aWNlIj4KPC9oZWFkPgo8Ym9keT4KPHByZT5IZWxsbyEKClNvcnJ5LCBzb21ldGhpbmcgd2VudCB3cm9uZyBhbmQgbGVkIHRvIGFuIGVycm9yOgoKPGZvbnQgY29sb3I9IiNDMDM5MkIiPjxiPkludmFsaWQgQml0Y29pbiBhZGRyZXNzOiBudWxsPC9iPjwvZm9udD4KCi0tLS0tClRoYW5rIHlvdSwKUGF5bWVudHMubmV0PC9wcmU+CjwvYm9keT4KPC9odG1sPgo="
}
```

#### Новый
```http request
POST /api/v1/user/new
```
Зарегистрировать нового пользователя.
 
**Параметры:**
[Общие параметры](#общие-параметры),
[Тело запроса](#параметры-пользователя)

#### Добавить
```http request
POST /api/v1/user/add
```
Добавить данные в учётную запись пользователя.
 
**Параметры:**
[Общие параметры](#общие-параметры),
[Тело запроса](#параметры-пользователя)

#### Обновить
```http request
POST /api/v1/user/update
```
Обновить данные в учётной записи пользователя.
 
**Параметры:**
[Общие параметры](#общие-параметры),
[Тело запроса](#параметры-пользователя)

#### Удалить
```http request
POST /api/v1/user/delete
```
Удалить учётную запись пользователя.

**Параметры:**
[Общие параметры](#общие-параметры),
[Тело запроса](#параметры-пользователя)

### Сделка

#### Создать
```http request
POST /api/v1/deal/create
```
Создать сделку.
 
**Параметры:**
[Общие параметры](#общие-параметры),
[Тело запроса](#параметры-сделки)

#### Оплатить
```http request
POST /api/v1/deal/pay
```
Оплатить сделку.
 
**Параметры:**
[Общие параметры](#общие-параметры),
[Тело запроса](#параметры-сделки)

#### Завершить
```http request
POST /api/v1/deal/complete
```
Завершить сделку.
 
**Параметры:**
[Общие параметры](#общие-параметры),
[Тело запроса](#параметры-сделки)

## Примеры:

* **HTTP Request:**
```http request
POST /api/v1/user/new HTTP/1.1
Host: localhost:40977
Content-Type: application/x-www-form-urlencoded

address=mynFyJJkRhsbB6y1Q5kTgDGckVz2m9NKH8&key=02ef68c191984433c8a730b8fa48a47ec016a0727e6d26cc0982c8900b39354f61
```    

* **curl command:**
````curl
curl "http://localhost:40977/api/v1/user/new?payload=html" \
  -X POST \
  -d "address=mynFyJJkRhsbB6y1Q5kTgDGckVz2m9NKH8&key=02ef68c191984433c8a730b8fa48a47ec016a0727e6d26cc0982c8900b39354f61" \
  -H "Content-Type: application/x-www-form-urlencoded"
````

* **HTTP Request:**
```http request
POST /api/v1/user/new HTTP/1.1
Host: localhost:40977
Content-Type: application/json

{"address": "mynFyJJkRhsbB6y1Q5kTgDGckVz2m9NKH8", "key":  "02ef68c191984433c8a730b8fa48a47ec016a0727e6d26cc0982c8900b39354f61"}
```    

* **curl command:**
````curl
curl "http://localhost:40977/api/v1/user/new?payload=html" \
  -X POST \
  -d '{"address": "mynFyJJkRhsbB6y1Q5kTgDGckVz2m9NKH8", "key":  "02ef68c191984433c8a730b8fa48a47ec016a0727e6d26cc0982c8900b39354f61"}' \
  -H "Content-Type: application/json"
````

* **JavaScript:**
````javascript
let headers = new Headers();
headers.append('Content-Type', 'multipart/form-data');

let body = new FormData();
body.append("address", "mynFyJJkRhsbB6y1Q5kTgDGckVz2m9NKH8");
body.append("key", "02ef68c191984433c8a730b8fa48a47ec016a0727e6d26cc0982c8900b39354f61");

const init = {
    method: 'POST',
    headers: headers,
    body: body,
    mode: "cors"
};

fetch('http://localhost:40977/api/v1/user/new', init)
    .then((response) => {
        return response.json();
    })
    .then((json) => {
        console.log(json);
        console.log(window.atob(json['payload']));
    })
    .catch((e) => {
        console.log(e.message);
});
````