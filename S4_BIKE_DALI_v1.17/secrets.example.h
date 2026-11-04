#pragma once

// Skopiuj ten plik jako secrets.h (w tym samym folderze) i wpisz własne dane.
// secrets.h jest w .gitignore – nie trafia do repozytorium.
#define SENDER_EMAIL        "adres@example.com"
#define SENDER_PASSWORD     "haslo-aplikacji"

// Opcjonalnie: jedno hasło sieci serwisowej dla wszystkich sterowników (8–63 znaki).
// Bez niego hasło to 8 cyfr wyliczonych z numeru układu ESP (i soli SERWIS_SOL).
// #define SERWIS_HASLO     "wpisz-haslo"
// #define SERWIS_SOL       "S-4 Dali serwis"
