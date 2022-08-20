# HELOBA

Das hier ist das Repository für HELOBA, unseren Heap-basierten Load-Balancer für die Radiokommunikation.

## Installation

Wir benutzen `make` als Buildsystem. Hier findet ihr eine Liste unserer `make` targets und was sie machen:

- `make debug` baut das Projekt mit Debugsymbolen, Debugoutput und [ASAN](https://clang.llvm.org/docs/AddressSanitizer.html).
- `make release` baut das Projekt ohne Debugsymbole und Debugoutput.
- `make simulation` baut das Projekt ohne Debugsymbole, aber mit Debugoutput, um Statistiken sammeln zu können.
- `make run` führt das kompilierte Programm aus.
- `make test` führt unsere Testsuite aus. Dafür muss vorher [criterion](https://github.com/Snaipe/Criterion) installiert werden.
- `make clean` löscht alle generierten Builddateien.
- `make deploy` kopiert den gesamten Source Code auf den Beaglebone nach `~/ppl_deployment`. Der Beaglebone muss unter `debian@beaglebone.local` erreichbar sein.

Falls ihr den Code auf dem Beaglebone testet, muss erst das `-DVIRTUAL` aus `Makefile:3` entfernt werden.

## Suchsimulation

Eine Suchsimulation kann mit `./simulate_search.sh` gestartet werden. Anpassbare Parameter sind die Anzahl der Nodes und die Laufzeit. Feinere Stellschrauben befinden sich in `include/src/config.h`, wir empfehlen aber dort nichts zu ändern.