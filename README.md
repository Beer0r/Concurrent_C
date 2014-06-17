## Concurrent C - File Server
Dierser File Server unterstütz folgende Operationen nach Protokolldefinition von "https://github.com/telmich/zhaw_seminar_concurrent_c_programming".
* LIST
* CREATE
* READ
* UPDATE
* DELETE

## Ausführen
Zunächst muss das Programm kompilliert werden:

`$ make`

Danach kann der File Server gestartet werden.

`$ ./run`

## Tests
Um die vorgefertigten Tests auszuführen, einfach das test (bash) Script ausführen:

`$ ./test`

Um die Semaphoren zu testen, einfach das test Script mehrfach parallel ausführen.
Manuelle Tests können sehr einfach auf der Konsole ausgeführt werden.
Das nachfolgende Beispiel zeigt vier manuelle Test-Ausführungen.

`$ echo -e "CREATE file1 5\ntest" | netcat 127.0.0.1 8080 2>&1`

`$ echo -e "UPDATE file1 13\nUPDATED:test" | netcat 127.0.0.1 8080 2>&1`

`$ echo "LIST" | netcat 127.0.0.1 8080 2>&1`

`$ echo "READ file1" | netcat 127.0.0.1 8080 2>&1`

## Anpassungen an Konstanten
Im File "server.c" können nachfolgende Konstanten angepasst werden.
* define NUMBER_OF_FILES 10
* define MAX_FILE_LENGHT 20
* define MAX_CONTENT_LENGHT 256
* define PORT 8080
Die IP Adresse kann nicht angepasst werden, da das Programm sowieso immer auf allen IP Adressen (also 0.0.0.0) des Servers hört.
Anpassungen im Test-Script sind hier möglich:
* PORT="8080"
* IP="127.0.0.1"

## Dokumentation
Die Dokumentation zu diesem Projekt liegt im File "doc.pdf"
