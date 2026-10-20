#pragma once

// Skopiuj ten plik jako secrets.h (w tym samym folderze) i wpisz własne dane.
// secrets.h jest w .gitignore – nie trafia do repozytorium.
#define SENDER_EMAIL        "adres@example.com"
#define SENDER_PASSWORD     "haslo-aplikacji"

// eaCloud: adres odbioru danych i klucz urządzenia (portal eaCloud → urządzenie → klucz).
// Adres: IP komputera z eaCloud w sieci lokalnej (ipconfig → Adres IPv4), port API 8198.
// Puste "" = adres i klucz z portalu Wi-Fi sterownika (S2 przy starcie).
#define EACLOUD_URL         "http://192.168.1.50:8198/update"
#define EACLOUD_KEY         "wklej-tu-klucz-urzadzenia"
