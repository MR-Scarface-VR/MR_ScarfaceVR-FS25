# SignPath vorbereiten und beantragen

Stand: 24.10.2026. Noch kein Antrag, keine Aufnahme und keine Signatur.
Dieses Paket ist für die Einrichtung gedacht, nicht zum Installieren im Spiel.

## Bereits lokal vorbereitet

- Eigene Quellen unter MIT; OpenXR-Lizenzen separat erhalten.
- Saubere öffentliche Dateiauswahl ohne private Engine-Abbilder und lokale Logs.
- DLL-Versionsinformationen und passende SignPath-Artefaktkonfiguration.
- GitHub-Build mit Tests; Signierschritt zunächst nicht freigeschaltet.
- Code signing policy, Datenschutz und englischer Bewerbungsentwurf.

Im Archiv enthält `repository/` die Dateien für das spätere Repository.
Die Quelldateien einschließlich `.github/` gehören direkt ins Repository-Hauptverzeichnis,
nicht in einen zusätzlichen Unterordner namens `repository`.
Bei Browser-Uploads sicherstellen, dass `.github/workflows/build-and-sign.yml`
und `.gitignore` mit übernommen werden. Alternativ mit Git kopieren/committen.
Das Spielpaket `MR_ScarfaceVR_FS25_Experimental.zip` ist ein separates Paket.
Es enthält den neuen unsignierten Stand 0.1.0.1; der erneute Headset-Test steht aus.

## 1. GitHub-Konto und Repository

1. Auf https://github.com/signup ein eigenes Konto einrichten und MFA aktivieren.
2. Ein öffentliches Repository `MR_ScarfaceVR-FS25` mit Standardbranch `main` anlegen.
3. Den Profil-Anzeigenamen auf **MR_Scarface** setzen. In CODE_SIGNING_POLICY.md
   und signing/APPLICATION_DRAFT.md `GITHUB_OWNER` durch den tatsächlichen
   Kontonamen ersetzen. Nur öffentliche Projekt-Kontaktdaten in veröffentlichte
   Dateien eintragen. Private Verifizierungsdaten gegebenenfalls direkt im
   Anbieterformular angeben. Keine Kennwörter oder API-Schlüssel in Dateien eintragen.
4. Nur den Inhalt von `repository/` übernehmen. `.analysis`, Spielabbilder,
   lokale Entwicklungsnotizen und Geschäftsplanung nicht hochladen.
5. Den Workflow unter „Actions“ zunächst ohne `request_signing` ausführen.
   Er muss erfolgreich bauen. Er benötigt weder FS25 noch Meta Link.

Die noch zu ersetzenden Kontaktfelder sind bewusst Platzhalter. Die MIT-Lizenz
für die eigenen Quellen wurde vom Projektinhaber freigegeben. Die Lizenz ist
keine Erlaubnis zur Weitergabe von Spiel- oder Meta-Dateien.

## 2. Erste öffentliche experimentelle Veröffentlichung

Eine nachvollziehbare öffentliche Pilot-Veröffentlichung und dokumentierte
Funktion sind Voraussetzungen für die Bewerbung. Ein bestehendes lokales
Headset-Ergebnis ist noch keine öffentliche Projektreputation.

Den veröffentlichten Build, seine Quellen und Prüfsummen eindeutig zuordnen.
Das vorhandene getestete Spielpaket ist noch unsigniert. Die neue lokale DLL
enthält zusätzliche Versionsmetadaten und ist ein neuer Build: vor Verwendung
als veröffentlichter Spiel-Build erneut mit FS25/Quest testen. Die Metadaten
ändern nicht absichtlich das VR-Verhalten, ersetzen aber keinen Praxistest.

Für das endgültige Paket müssen bei jeder anderen DLL die DLL-Prüfsumme im
Manifest, die Dateiprüfsummen und die ZIP-Prüfsumme erneuert werden. Keine neue
DLL unter alten Prüfsummen oder als bereits headset-getestetes Original ausgeben.
Die Quelle muss zur veröffentlichten Binärdatei passen; keine privaten Abbilder
oder Diagnosedaten in das Quellpaket aufnehmen.

## 3. Bewerbung

Den Entwurf APPLICATION_DRAFT.md mit echten Repository-, Release- und Build-URLs
ergänzen. Die Oberfläche auf https://signpath.org/apply.html ist maßgeblich;
der Entwurf bietet Antworten zum Übertragen, kein automatisch versendetes Formular.

Der Antrag beschreibt die Renderer-/IAT-/Present-Hooks und die proprietären
externen Voraussetzungen ausdrücklich. Für dieses neue Projekt ist eine
Ablehnung wegen geringer Reputation oder anderer Aufnahmebedingungen möglich.
Keine SignPath-Unterstützung behaupten, bevor die Aufnahme bestätigt ist.

## 4. Erst nach Aufnahme: SignPath und GitHub verbinden

Die von SignPath zugewiesenen IDs/Bezeichnungen verwenden; nichts erfinden.

- GitHub als Trusted Build System mit dem tatsächlichen Repository verknüpfen;
  die GitHub-App nach den SignPath-Anweisungen berechtigen.
- `signing/artifact-configuration.xml` als Standard-Artefaktkonfiguration
  übernehmen. Das GitHub-Artefakt ist ein ZIP mit `dinput8.dll` direkt darin.
- Für die Produktionsrichtlinie menschliche Freigabe jeder Signierung in
  SignPath verlangen. Ein Token mit Einreichungsrechten genügt.
- In GitHub die Umgebung `signpath` einrichten und deren Zugriff auf `main`
  begrenzen. Falls verfügbar, zusätzlich menschliche Umgebungsfreigabe aktivieren.
- Die folgenden GitHub-Repository-Variablen setzen:
  `SIGNPATH_ORGANIZATION_ID`, `SIGNPATH_PROJECT_SLUG`,
  `SIGNPATH_SIGNING_POLICY_SLUG`, `SIGNPATH_CERTIFICATE_THUMBPRINT`.
  Den erwarteten öffentlichen Zertifikat-Fingerabdruck aus SignPath übernehmen.
- Das Secret `SIGNPATH_API_TOKEN` in der Umgebung `signpath` hinterlegen.
- Erst nach vollständiger Einrichtung die Repository-Variable
  `SIGNPATH_ENABLED` auf den Textwert `true` setzen.
- Rollen und Status in CODE_SIGNING_POLICY.md aktualisieren und nach Aufnahme
  den dort vorbereiteten SignPath-Attributionssatz aktivieren.

## 5. Erste Signierung und Veröffentlichung

1. Auf `main` den Workflow manuell mit `request_signing=true` starten.
2. Er baut erneut, testet und lädt die unsignierte DLL zu GitHub hoch.
3. Die richtige Anfrage samt Commit/Artifact in SignPath kontrollieren und dort
   menschlich freigeben. Der Workflow wartet maximal eine Stunde auf Freigabe.
4. Nur nach erfolgreicher Signatur-/Zeitstempel-/Fingerabdruckprüfung wird das
   Artefakt `mr-scarfacevr-native-signed` mit DLL und SHA256SUMS bereitgestellt.
   Es ist noch kein vollständiges installierbares Mod-Paket.
5. Die signierte DLL mit dem passenden Lua-ZIP, Quellen und Anleitungen zum
   finalen Mod-Paket zusammenstellen; alle Prüfsummen neu erstellen.
6. FS25/Quest und einen PC mit aktiviertem Smart App Control testen.
   Erst danach als signierten experimentellen Build veröffentlichen.

Der Workflow publiziert keine Releases automatisch. Geheimnisse werden weder
für Pull Requests gebraucht noch bei deren Builds für Signierung verwendet.
Ein anderer Branch oder fehlende Freischaltung lehnt eine Signieranfrage ab.
Ein Zertifikatswechsel erfordert die bewusste Anpassung des Fingerabdrucks.

## Quellen

- [Aufnahmebedingungen](https://signpath.org/terms.html)
- [Bewerbung](https://signpath.org/apply.html)
- [GitHub-Anbindung und offizielle Action](https://docs.signpath.io/trusted-build-systems/github)
- [Artefakt-Syntax](https://docs.signpath.io/artifact-configuration/syntax)
- [Metadaten-Einschränkungen](https://docs.signpath.io/artifact-configuration/reference#file-metadata-restrictions)

Die drei Workflow-Actions sind auf am 24.10.2026 über GitHubs öffentliche API
aufgelöste Commits der Tags checkout v4, upload-artifact v7 und SignPath v3
festgelegt. Künftige Updates bewusst prüfen. Der echte Cloud-Ablauf lässt sich
ohne GitHub-Repository und SignPath-Freischaltung noch nicht ausführen.
