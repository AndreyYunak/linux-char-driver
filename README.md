# linux-char-driver

## Задание Linux kernel

Реализовать символьный драйвер предлагающий возможность передавать информацию двум независимым процессам через файл символьного устройства: 

 - взаимодействие должно осуществляться по принципу кольцевого буфера;  т.е. один процесс может писать в файл символьного устройства, а другой из него читать.
 - обеспечить возможность задавать размер кольцевого буфера через механизм параметров модулей ядра; 
 - по умолчанию вызовы чтения и записи должны быть блокирующими;
 - обеспечить поддержку нескольких вызовов IOCTL:
 - переключение режима работы вызовов read\write из блокирующих \ не блокирующих и обратно;
 - обеспечить возможность получения информации о времени последней операции чтения и записи в буфер, а так же идентификаторов и владельцев процессов совершивших эту операцию;
 - драйвер должен собираться вне дерева исходных кодов ядра при помощи Makefile;
 - исходные коды драйвера должны находиться под системой контроля версий git и содержать историю коммитов;
 - результат передаётся в виде ссылки на репозиторий размещённый на github или в виде архива содержащего репозиторий с исходными кодами.

